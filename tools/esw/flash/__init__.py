import re
import shutil
import subprocess
from collections import defaultdict
from dataclasses import dataclass
from pathlib import Path

from elftools.elf.elffile import ELFFile
from elftools.elf.sections import SymbolTableSection


_FLASH_BASE = 0x08000000
_FLASH_LIMIT = 0x09000000

_WARN_FRACTION = 0.85

_REGION_RE = re.compile(
    r"^\s*FLASH\s*\([^)]*\)\s*:\s*ORIGIN\s*=\s*(0x[0-9a-fA-F]+|\d+)\s*,"
    r"\s*LENGTH\s*=\s*(\d+)\s*([KMkm]?)",
    re.MULTILINE,
)
_PAGE_SIZE_RE = re.compile(r"PAGE_SIZE\s*=\s*(\d+)")

_MAP_RE = re.compile(r"^\s+(0x[0-9a-fA-F]+)\s+(0x[0-9a-fA-F]+)\s+(\S.*)$")

_SCALE = {"": 1, "k": 1024, "m": 1024 * 1024}


@dataclass(frozen=True)
class Region:
    origin: int
    length: int


@dataclass(frozen=True)
class Usage:
    project: str
    extent: int
    top_addr: int
    region: Region
    config_bytes: int

    @property
    def usable(self) -> int:
        return self.region.length - self.config_bytes

    @property
    def percent(self) -> float:
        return 100.0 * self.extent / self.region.length

    @property
    def percent_usable(self) -> float:
        return 100.0 * self.extent / self.usable

    @property
    def config_origin(self) -> int:
        return self.region.origin + self.usable

    @property
    def status(self) -> str:
        if self.extent > self.region.length:
            return "OVERRUN"
        if self.config_bytes and self.extent > self.usable:
            return "CONFIG"
        if self.extent > _WARN_FRACTION * self.usable:
            return "TIGHT"
        return "OK"

    @property
    def ok(self) -> bool:
        return self.status in ("OK", "TIGHT")


def read_region(ld_path: Path) -> Region:
    match = _REGION_RE.search(ld_path.read_text())
    if not match:
        raise RuntimeError(f"no FLASH region found in {ld_path}")

    origin = int(match.group(1), 0)
    length = int(match.group(2)) * _SCALE[match.group(3).lower()]
    return Region(origin=origin, length=length)


def read_config_bytes(inc_dir: Path | None, project: str) -> int:
    if inc_dir is None:
        return 0

    header = inc_dir / f"{project}_config.hpp"
    if not header.is_file():
        return 0

    match = _PAGE_SIZE_RE.search(header.read_text())
    if not match:
        raise RuntimeError(f"{header} exists but declares no PAGE_SIZE")

    return int(match.group(1))


def read_usage(elf_path: Path, ld_path: Path, inc_dir: Path | None = None) -> Usage:
    region = read_region(ld_path)

    with elf_path.open("rb") as handle:
        elf = ELFFile(handle)
        tops = [
            segment["p_paddr"] + segment["p_filesz"]
            for segment in elf.iter_segments()
            if segment["p_type"] == "PT_LOAD"
            and segment["p_filesz"]
            and _FLASH_BASE <= segment["p_paddr"] < _FLASH_LIMIT
        ]

    if not tops:
        raise RuntimeError(f"{elf_path} has no loadable flash segments")

    top = max(tops)
    return Usage(
        project=elf_path.stem,
        extent=top - region.origin,
        top_addr=top,
        region=region,
        config_bytes=read_config_bytes(inc_dir, elf_path.stem),
    )


def format_usage(usage: Usage) -> str:
    reserved = f", {usage.config_bytes:,} B reserved for config" if usage.config_bytes else ""
    return (
        f"flash: {usage.extent:,} / {usage.usable:,} B usable ({usage.percent_usable:.1f}%{reserved}) [{usage.status}]"
    )


def explain_failure(usage: Usage) -> str:
    if usage.status == "OVERRUN":
        return (
            f"image overruns flash by {usage.extent - usage.region.length:,} B "
            f"({usage.extent:,} B into a {usage.region.length:,} B region)"
        )
    if usage.status == "CONFIG":
        return (
            f"image extends {usage.extent - usage.usable:,} B into the configuration page.\n"
            f"  image top      0x{usage.top_addr:08x}\n"
            f"  config page    0x{usage.config_origin:08x} - 0x{usage.region.origin + usage.region.length - 1:08x}\n"
            f"  flashing this would erase the board configuration."
        )
    return ""


def component_of(obj: str) -> str:
    if "STM32G4xx_HAL_Driver" in obj or "STM32_Drivers.dir" in obj:
        return "HAL/LL drivers"
    if "FreeRTOS" in obj or "freertos.dir" in obj:
        return "FreeRTOS kernel"
    if "fwrtos.dir" in obj:
        return "fwlib/rtos"
    if "fwlib/" in obj:
        return "fwlib (other)"
    if obj.endswith(".a") or ".a(" in obj:
        return "toolchain libs"
    if "CMakeFiles" in obj:
        return "application"
    return "other"


def object_sizes(map_path: Path) -> dict[str, int]:
    sizes: dict[str, int] = defaultdict(int)

    for line in map_path.read_text(errors="replace").splitlines():
        match = _MAP_RE.match(line)
        if not match:
            continue

        addr = int(match.group(1), 16)
        size = int(match.group(2), 16)
        obj = match.group(3).strip()

        if size <= 0 or not (_FLASH_BASE <= addr < _FLASH_LIMIT):
            continue
        if ".o" not in obj and ".a(" not in obj:
            continue

        sizes[obj] += size

    return dict(sizes)


def symbol_entries(elf_path: Path) -> list[tuple[str, int, int]]:
    entries: list[tuple[str, int, int]] = []

    with elf_path.open("rb") as handle:
        elf = ELFFile(handle)
        table = elf.get_section_by_name(".symtab")
        if not isinstance(table, SymbolTableSection):
            return []

        for symbol in table.iter_symbols():
            if symbol["st_info"]["type"] not in ("STT_FUNC", "STT_OBJECT"):
                continue
            size = symbol["st_size"]
            value = symbol["st_value"]
            if size <= 0 or not (_FLASH_BASE <= value < _FLASH_LIMIT):
                continue
            entries.append((symbol.name, value, size))

    return entries


def symbol_sizes(elf_path: Path) -> dict[str, int]:
    sizes: dict[str, int] = defaultdict(int)
    for name, _, size in symbol_entries(elf_path):
        sizes[name] += size
    return dict(sizes)


def _parse_components(rest: str) -> list[str]:
    out: list[str] = []
    i = 0
    while i < len(rest) and rest[i].isdigit():
        j = i
        while j < len(rest) and rest[j].isdigit():
            j += 1
        length = int(rest[i:j])
        out.append(rest[j : j + length])
        i = j + length
    return out


def owner_of(mangled: str) -> str:
    if not mangled.startswith("_Z"):
        return mangled  # plain C symbol

    rest = mangled[2:]
    if rest.startswith("N"):
        rest = rest[1:]
    rest = rest.lstrip("KVRO")  # cv- and ref-qualifiers

    if rest.startswith("St"):
        parts = _parse_components(rest[2:])
        if not parts:
            return "std::"
        # std::__detail::__variant, not std::__detail
        if parts[0].startswith("__") and len(parts) > 1:
            return f"std::{parts[0]}::{parts[1]}"
        return f"std::{parts[0]}"

    parts = _parse_components(rest)
    if not parts:
        return mangled
    return "::".join(parts[:2]) if len(parts) > 2 else parts[0]


def category_of(owner: str) -> str:
    if owner.startswith("std::"):
        leaf = owner.split("::", 1)[1]
        if "visit" in leaf or "variant" in leaf:
            return "std:: variant/visit machinery"
        if leaf in ("_Construct", "construct_at", "_Optional_payload", "_Optional_base", "optional"):
            return "std:: optional/construct"
        return "std:: other"
    if owner.startswith("HAL_") or owner.startswith("LL_"):
        return "HAL public API"
    if owner.startswith(("UART_", "ADC_", "TIM_", "FDCAN_", "RCC_", "DMA_", "I2C_", "SPI_")):
        return "HAL internals"
    if owner.startswith(("xTask", "vTask", "xQueue", "prv", "pxCurrent", "uxTask", "vPort", "xPort", "os")):
        return "FreeRTOS/CMSIS-RTOS"
    if owner.startswith("mrover::"):
        return f"mrover:: {owner.split('::')[1]}"
    if owner.startswith("mrover"):
        return "mrover:: (free functions)"
    return "other/libc"


def object_extents(map_path: Path) -> list[tuple[int, int, str]]:
    spans: list[tuple[int, int, str]] = []

    for line in map_path.read_text(errors="replace").splitlines():
        match = _MAP_RE.match(line)
        if not match:
            continue
        addr = int(match.group(1), 16)
        size = int(match.group(2), 16)
        obj = match.group(3).strip()
        if size <= 0 or not (_FLASH_BASE <= addr < _FLASH_LIMIT):
            continue
        if ".o" not in obj and ".a(" not in obj:
            continue
        spans.append((addr, addr + size, obj))

    return sorted(spans)


def demangle(names: list[str]) -> dict[str, str]:
    tool = shutil.which("arm-none-eabi-c++filt") or shutil.which("c++filt")
    if tool is None or not names:
        return {name: name for name in names}

    result = subprocess.run(tool, input="\n".join(names), capture_output=True, text=True, check=False)
    if result.returncode != 0:
        return {name: name for name in names}

    out = result.stdout.splitlines()
    if len(out) != len(names):
        return {name: name for name in names}

    return dict(zip(names, out))
