import os
from datetime import datetime
from pathlib import Path
from typing import Any

from jinja2 import Environment, FileSystemLoader

from esw import esw_logger


_C_STANDARD: int = 17
_CXX_STANDARD: int = 23


def _clean_project(path: Path) -> None:
    esw_logger.info(f"Cleaning Existing CMake Project {path.absolute().resolve()}")
    cmakelists = path / "CMakeLists.txt"
    cmakelists_stm = path / "cmake" / "stm32cubemx" / "CMakeLists.txt"

    if cmakelists.exists() and cmakelists.is_file():
        cmakelists.unlink()
    if cmakelists_stm.exists() and cmakelists_stm.is_file():
        cmakelists_stm.unlink()
    if cmakelists_stm.parent.exists() and cmakelists_stm.parent.is_dir():
        cmakelists_stm.parent.rmdir()

def _find_dir_case_exact(base: Path, rel: Path) -> Path | None:
    """
    Resolve a relative path using the exact on-disk casing.

    Traverses `base / rel` component-by-component, performing a
    case-insensitive match at each level but returning the directory
    names with their true filesystem casing.

    This is primarily intended to prevent case-insensitive filesystems
    from emitting incorrectly-cased paths into generated build files
    that will later be consumed on case-sensitive systems.

    Args:
        base: Base directory from which resolution begins.
        rel: Relative path to resolve (case-insensitive).

    Returns:
        A Path whose components match the actual casing on disk, relative
        to `base`, or None if any path component does not exist.
    """
    cur = base
    parts_out: list[str] = []

    for part in rel.parts:
        if not cur.is_dir():
            return None

        match = None
        for entry in cur.iterdir():
            if entry.name.lower() == part.lower():
                match = entry
                break
        if match is None:
            return None

        parts_out.append(match.name)
        cur = match

    return Path(*parts_out)

def _get_context(name: str, path: Path, root: Path, libs: list[str]) -> dict[str, Any]:
    # find source file directory
    possible_srcs = [Path("Src"), Path("src"), Path("Core") / "Src"]
    src_dir: Path | None = None
    for cand in possible_srcs:
        resolved = _find_dir_case_exact(path, cand)
        if resolved is not None and (path / resolved).is_dir():
            src_dir = resolved
            esw_logger.info(f"Found Source Directory {(path.absolute() / src_dir).resolve()}")
            break
    if not src_dir:
        err = f"Could not find Source Directory under {path.absolute().resolve()}"
        esw_logger.error(err)
        raise RuntimeError(err)

    # find header file directory
    possible_incs = [Path("Inc"), Path("inc"), Path("Core") / "Inc"]
    inc_dir: Path | None = None
    for cand in possible_incs:
        resolved = _find_dir_case_exact(path, cand)
        if resolved is not None and (path / resolved).is_dir():
            inc_dir = resolved
            esw_logger.info(f"Found Header Directory {(path.absolute() / inc_dir).resolve()}")
            break
    if not inc_dir:
        err = f"Could not find Header Directory under {path.absolute().resolve()}"
        esw_logger.error(err)
        raise RuntimeError(err)

    # find driver script
    scripts = [p for p in path.iterdir() if p.is_file() and p.suffix == ".s"]
    if len(scripts) == 0:
        err = f"No .s script found in directory {path.absolute().resolve()}"
        esw_logger.error(err)
        raise FileNotFoundError(err)
    if len(scripts) > 1:
        err = f"Multiple .s scripts found: {[p.name for p in scripts]}"
        esw_logger.error(err)
        raise RuntimeError(err)
    driver_script = scripts[0].relative_to(path)
    if driver_script:
        esw_logger.info(f"Found Driver Script {driver_script.absolute().resolve()}")
    else:
        err = f"Could not find Driver Script under {path.absolute().resolve()}"
        esw_logger.error(err)
        raise RuntimeError(err)

    # find relative fwlib path
    cmake_path = path.absolute().resolve()
    lib_path = (root / "lib").absolute().resolve()
    lib_relative_path = os.path.relpath(lib_path, cmake_path)
    esw_logger.info(f"Found fwlib at {lib_relative_path}")

    return {
        "timestamp": datetime.now().strftime("%Y-%m-%d %H:%M:%S"),
        "c_standard": _C_STANDARD,
        "cxx_standard": _CXX_STANDARD,
        "project_name": name,
        "mx_src_dir": src_dir,
        "mx_inc_dir": inc_dir,
        "mx_startup_s": driver_script,
        "lib_relative_path": lib_relative_path,
        "libs": libs,
    }


def configure_cmake(name: str, path: Path, root: Path, ctx: Path, libs: list[str]) -> None:
    _clean_project(path)
    context = _get_context(name, path, root, libs)

    env = Environment(loader=FileSystemLoader(ctx))
    cmake_template = env.get_template("templates/CMakeLists.txt.j2")

    cmakelists = path / "CMakeLists.txt"
    esw_logger.info(f"Writing CMakeLists.txt to {cmakelists.absolute().resolve()}")
    with cmakelists.open("w") as handle:
        handle.write(cmake_template.render(context))
