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
    cmakepresets = path / "CMakePresets.json"
    cmakelists_stm = path / "cmake" / "stm32cubemx" / "CMakeLists.txt"

    if cmakelists.exists() and cmakelists.is_file():
        cmakelists.unlink()
    if cmakepresets.exists() and cmakepresets.is_file():
        cmakepresets.unlink()
    if cmakelists_stm.exists() and cmakelists_stm.is_file():
        cmakelists_stm.unlink()
    if cmakelists_stm.parent.exists() and cmakelists_stm.parent.is_dir():
        cmakelists_stm.parent.rmdir()


def _get_context(name: str, path: Path, root: Path, libs: list[str]) -> dict[str, Any]:
    # find source file directory
    src_dir: Path | None = None
    possible_srcs = [path / "Src", path / "Core" / "Src"]
    for possible_src in possible_srcs:
        if possible_src.exists() and possible_src.is_dir():
            src_dir = possible_src.relative_to(path)
            esw_logger.info(f"Found Source Directory {src_dir.absolute().resolve()}")
    if not src_dir:
        err = f"Could not find Source Directory under {path.absolute().resolve()}"
        esw_logger.error(err)
        raise RuntimeError(err)

    # find header file directory
    inc_dir: Path | None = None
    possible_incs = [path / "Inc", path / "Core" / "Inc"]
    for possible_inc in possible_incs:
        if possible_inc.exists() and possible_inc.is_dir():
            inc_dir = possible_inc.relative_to(path)
            esw_logger.info(f"Found Header Directory {inc_dir.absolute().resolve()}")
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
    cmake_presets_template = env.get_template("templates/CMakePresets.json.j2")

    cmakelists = path / "CMakeLists.txt"
    esw_logger.info(f"Writing CMakeLists.txt to {cmakelists.absolute().resolve()}")
    with cmakelists.open("w") as handle:
        handle.write(cmake_template.render(context))

    cmakepresets = path / "CMakePresets.json"
    esw_logger.info(f"Writing CMakePresets.json to {cmakepresets.absolute().resolve()}")
    with cmakepresets.open("w") as handle:
        handle.write(cmake_presets_template.render())
