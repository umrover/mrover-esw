set shell := ["zsh", "-ceuo", "pipefail"]

default_preset := "Debug"

alias b := build
alias f := flash
alias m := monitor

# list available recipes
@default:
    just --list

# build a project
@build src preset=default_preset:
    ./scripts/build.sh --src {{src}} --preset {{preset}}

# write an executable to STLINK (reset and run application)
@flash src preset=default_preset:
    just build {{src}} {{preset}}
    ./scripts/build.sh --src {{src}} --preset {{preset}} --flash

# update cmake tooling
cmake src *libs:
    #!/usr/bin/env zsh
    source tools/venv/bin/activate
    PY_LIB_ARGS=()
    for lib in {{libs}}; do
        PY_LIB_ARGS+=(--lib "$lib")
    done
    python ./tools/scripts/update_cmake_cfg.py --src {{src}} --root . --ctx ./lib/stm32g4 "${PY_LIB_ARGS[@]}"

monitor baud="115200" log="INFO":
    #!/usr/bin/env zsh
    source tools/venv/bin/activate
    python ./tools/scripts/monitor.py --baud {{baud}} --log-level {{log}}

