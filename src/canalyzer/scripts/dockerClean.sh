#!/bin/bash
# Removes the canalyzer docker images built by dockerBuld.sh:
#   - every tag of can_logger_image
#   - untagged leftovers from earlier rebuilds, recognized by their
#     canalyzer (or older socketcan.out) entrypoint
# along with any containers created from them. influxdb/grafana images and
# their data volumes are not touched.
#
# usage: dockerClean.sh [-n] [-y] [--cache]
#   -n, --dry-run  only list what would be removed
#   -y, --yes      don't ask for confirmation
#   --cache        also prune ALL unused docker build cache (frees the gcc
#                  base image and build layers, but the next dockerBuld.sh
#                  starts from scratch; affects every project on this machine)
set -euo pipefail

IMAGE_NAME="${IMAGE_NAME:-can_logger_image}"
DRY_RUN=0
ASSUME_YES=0
PRUNE_CACHE=0

# prints the comment block at the top of this file
usage() { awk 'NR == 1 { next } /^#/ { sub(/^# ?/, ""); print; next } { exit }' "$0"; }

while [ $# -gt 0 ]; do
    case "$1" in
        -n | --dry-run) DRY_RUN=1 ;;
        -y | --yes) ASSUME_YES=1 ;;
        --cache) PRUNE_CACHE=1 ;;
        -h | --help)
            usage
            exit 0
            ;;
        *)
            echo "Error: unknown option '$1'" >&2
            usage >&2
            exit 1
            ;;
    esac
    shift
done

command -v docker >/dev/null || {
    echo "Error: docker not found" >&2
    exit 1
}

# every repo:tag of the image
mapfile -t tagged_refs < <(docker images --format '{{.Repository}}:{{.Tag}}' "$IMAGE_NAME" | grep -v ':<none>$' | sort -u)

# untagged images whose entrypoint is (some past version of) the logger
mapfile -t dangling_ids < <(docker images -q --no-trunc --filter dangling=true | sort -u)
leftover_ids=()
if [ "${#dangling_ids[@]}" -gt 0 ]; then
    while read -r id ep; do
        case "$ep" in
            *canalyzer* | *socketcan.out*) leftover_ids+=("$id") ;;
        esac
    done < <(docker image inspect --format '{{.Id}} {{if .Config}}{{json .Config.Entrypoint}}{{end}}' "${dangling_ids[@]}")
fi

if [ "${#tagged_refs[@]}" -eq 0 ] && [ "${#leftover_ids[@]}" -eq 0 ]; then
    echo "No canalyzer images found."
else
    image_ids=("${leftover_ids[@]}")
    for ref in "${tagged_refs[@]}"; do
        image_ids+=("$(docker image inspect --format '{{.Id}}' "$ref")")
    done
    mapfile -t image_ids < <(printf '%s\n' "${image_ids[@]}" | sort -u)

    containers=()
    for id in "${image_ids[@]}"; do
        while read -r c; do
            [ -n "$c" ] && containers+=("$c")
        done < <(docker ps -aq --no-trunc --filter "ancestor=$id")
    done
    [ "${#containers[@]}" -gt 0 ] && mapfile -t containers < <(printf '%s\n' "${containers[@]}" | sort -u)

    echo "Images to remove:"
    for ref in "${tagged_refs[@]}"; do
        docker image inspect --format "  {{slice .Id 7 19}}  built {{slice .Created 0 10}}  $ref" "$ref"
    done
    for id in "${leftover_ids[@]}"; do
        docker image inspect --format '  {{slice .Id 7 19}}  built {{slice .Created 0 10}}  <untagged>' "$id"
    done
    if [ "${#containers[@]}" -gt 0 ]; then
        echo "Containers to remove (they are recreated by docker compose up):"
        id_filters=()
        for c in "${containers[@]}"; do id_filters+=(--filter "id=$c"); done
        docker ps -a --format '  {{.Names}}  ({{.Status}})' "${id_filters[@]}"
    fi
    echo "Disk use before:"
    docker system df | sed 's/^/  /'

    if [ "$DRY_RUN" -eq 1 ]; then
        echo "Dry run, nothing removed."
        exit 0
    fi

    if [ "$ASSUME_YES" -ne 1 ]; then
        read -r -p "Remove ${#image_ids[@]} image(s) and ${#containers[@]} container(s)? [y/N] " answer || answer=""
        case "$answer" in
            [yY] | [yY][eE][sS]) ;;
            *)
                echo "Aborted."
                exit 1
                ;;
        esac
    fi

    if [ "${#containers[@]}" -gt 0 ]; then
        echo ">> Removing containers..."
        docker rm -f "${containers[@]}" >/dev/null
    fi

    echo ">> Removing images..."
    failed=0
    # tags are removed by name so a tag shared with another repo is only untagged
    for target in "${tagged_refs[@]}" "${leftover_ids[@]}"; do
        if ! out="$(docker rmi "$target" 2>&1)"; then
            # an image already deleted by an earlier removal is fine
            if ! grep -q 'No such image' <<<"$out"; then
                echo "  could not remove $target: $out" >&2
                failed=$((failed + 1))
            fi
        fi
    done
    [ "$failed" -gt 0 ] && echo "$failed image(s) could not be removed (see above)." >&2
fi

if [ "$PRUNE_CACHE" -eq 1 ]; then
    if [ "$DRY_RUN" -eq 1 ]; then
        echo "Dry run: would prune all unused build cache."
    else
        echo ">> Pruning build cache..."
        docker builder prune -af
    fi
fi

if [ "$DRY_RUN" -ne 1 ]; then
    echo "Disk use after:"
    docker system df | sed 's/^/  /'
fi
