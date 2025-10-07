set shell := ["zsh", "-ceuo", "pipefail"]

default_preset := "Debug"

alias b := build
alias f := flash

# list available recipes
@default:
    just --list

# build a project
@build src preset=default_preset:
    ./scripts/build.sh --src {{src}} --preset {{preset}}

# write an executable to STLINK
@flash src preset=default_preset:
    just build {{src}} {{preset}}
    ./scripts/flash.sh --src {{src}} --preset {{preset}}

