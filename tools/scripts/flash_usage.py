import argparse
import sys
from pathlib import Path

from esw.flash import (
    category_of,
    component_of,
    demangle,
    object_extents,
    owner_of,
    symbol_entries,
    explain_failure,
    format_usage,
    object_sizes,
    read_usage,
    symbol_sizes,
)


def _table(title: str, rows: list[tuple[str, int]], total: int, width: int = 58, keep: str = "tail") -> None:
    print(f"\n{title}")
    print("-" * (width + 22))
    for name, size in rows:
        share = 100.0 * size / total if total else 0.0
        if len(name) <= width:
            label = name
        elif keep == "tail":
            label = "..." + name[-(width - 3) :]
        else:
            label = name[: width - 3] + "..."
        print(f"{label:<{width}} {size:>8,} B {share:>5.1f}%")


def _report(elf: Path, map_path: Path | None, top: int) -> None:
    symbols = symbol_sizes(elf)
    total_symbols = sum(symbols.values())

    if map_path is not None and map_path.is_file():
        objects = object_sizes(map_path)
        total_objects = sum(objects.values())

        components: dict[str, int] = {}
        for obj, size in objects.items():
            components[component_of(obj)] = components.get(component_of(obj), 0) + size

        _table(
            "by component",
            sorted(components.items(), key=lambda kv: kv[1], reverse=True),
            total_objects,
        )
        _table(
            f"top {top} object files",
            sorted(objects.items(), key=lambda kv: kv[1], reverse=True)[:top],
            total_objects,
        )
    else:
        print(f"\nno map file at {map_path}, skipping object breakdown")

    ranked = sorted(symbols.items(), key=lambda kv: kv[1], reverse=True)[:top]
    readable = demangle([name for name, _ in ranked])
    _table(
        f"top {top} symbols (indicative shares; aliases counted per-name)",
        [(readable[name], size) for name, size in ranked],
        total_symbols,
        keep="head",
    )


def _trace(elf: Path, map_path: Path | None, usage, top: int) -> None:
    import bisect

    symbols = symbol_sizes(elf)
    attributed = sum(symbols.values())

    print("\n" + "=" * 80)
    print("TRACE")
    print("=" * 80)
    print(f"image                {usage.extent:>9,} B")
    print(
        f"attributed to symbols{attributed:>9,} B   ({100.0 * attributed / usage.extent:.1f}% -- >100% means aliases share addresses)"
    )

    categories: dict[str, int] = {}
    owners: dict[str, int] = {}
    counts: dict[str, int] = {}
    for mangled, size in symbols.items():
        owner = owner_of(mangled)
        category = category_of(owner)
        categories[category] = categories.get(category, 0) + size
        owners[owner] = owners.get(owner, 0) + size
        counts[category] = counts.get(category, 0) + 1

    print("\nby category")
    print("-" * 80)
    print(f"{'category':<44} {'bytes':>9} {'share':>7} {'count':>7}")
    for name, size in sorted(categories.items(), key=lambda kv: kv[1], reverse=True)[:top]:
        print(f"{name:<44} {size:>9,} {100.0 * size / attributed:>6.1f}% {counts[name]:>7,}")

    print(f"\ntop {top} owners")
    print("-" * 80)
    for name, size in sorted(owners.items(), key=lambda kv: kv[1], reverse=True)[:top]:
        label = name if len(name) <= 60 else name[:57] + "..."
        print(f"{label:<60} {size:>9,} {100.0 * size / attributed:>6.1f}%")

    if map_path is None or not map_path.is_file():
        return

    spans = object_extents(map_path)
    starts = [start for start, _, _ in spans]

    per_object: dict[str, dict[str, int]] = {}
    for mangled, addr, size in symbol_entries(elf):
        idx = bisect.bisect_right(starts, addr) - 1
        if idx < 0 or addr >= spans[idx][1]:
            continue
        obj = spans[idx][2]
        per_object.setdefault(obj, {})
        category = category_of(owner_of(mangled))
        per_object[obj][category] = per_object[obj].get(category, 0) + size

    print("\ninside the heaviest object files")
    print("-" * 80)
    heaviest = sorted(per_object.items(), key=lambda kv: sum(kv[1].values()), reverse=True)[:3]
    for obj, cats in heaviest:
        total = sum(cats.values())
        print(f"\n  {obj.split('/')[-1]}  ({total:,} B)")
        for name, size in sorted(cats.items(), key=lambda kv: kv[1], reverse=True)[:10]:
            print(f"    {name:<50} {size:>9,} {100.0 * size / total:>6.1f}%")


if __name__ == "__main__":
    parser = argparse.ArgumentParser(description="Report and enforce the flash budget of a firmware image")
    parser.add_argument("--elf", "-e", type=Path, required=True, help="Path to the linked .elf")
    parser.add_argument("--ld", "-l", type=Path, required=True, help="Path to the .ld linker script")
    parser.add_argument("--inc", "-i", type=Path, default=None, help="Project include directory (finds the config header)")
    parser.add_argument("--map", "-m", type=Path, default=None, help="Path to the .map file, for the object breakdown")
    parser.add_argument("--report", "-r", action="store_true", help="Always print the size breakdown")
    parser.add_argument("--trace", action="store_true", help="Full attribution of every symbol by what defines it")
    parser.add_argument("--top", "-t", type=int, default=15, help="Rows per breakdown table")
    parser.add_argument("--no-fail", action="store_true", help="Report only; never exit non-zero")
    args = parser.parse_args()

    usage = read_usage(args.elf, args.ld, args.inc)
    print(format_usage(usage))

    if args.report or args.trace or not usage.ok:
        _report(args.elf, args.map, args.top)

    if args.trace:
        _trace(args.elf, args.map, usage, args.top)

    if not usage.ok:
        sys.stdout.flush()
        print(f"\nerror: {explain_failure(usage)}", file=sys.stderr, flush=True)
        if not args.no_fail:
            sys.exit(1)
