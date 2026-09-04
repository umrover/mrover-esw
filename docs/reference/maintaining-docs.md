# Maintaining Documentation

These docs are built with [zensical](https://zensical.org) from the markdown in `docs/`.
Configuration lives in `zensical.toml` at the repo root.

## Building Locally

```bash
uv sync --project tools --only-group docs
uv run --project tools zensical serve
```

`serve` watches the tree and reloads as you edit. To produce the static site in `site/` instead:

```bash
uv run --project tools zensical build --clean
```

`site/` is gitignored. There is no `scripts/docs.sh` wrapper.

## Deployment

`.github/workflows/site.yml` runs on push and pull request to `main`, filtered to `docs/**`,
`zensical.toml`, and the Python project files. It runs the same two commands as above and
publishes `site/` to GitHub Pages on `main`.

The workflow does not hardcode a `uv` version; it greps the pin out of `Dockerfile.arm-gnu`, the
same line the firmware build uses. See [Continuous Integration](build/ci.md).

## Mirroring to docs.mrover.org

This tree is also the source for the ESW section of [docs.mrover.org](https://docs.mrover.org/esw).
After `site` deploys on `main`, its `notify-docs` job fires a `repository_dispatch` at
`umrover/mrover-docs`, which re-copies `docs/`, converts it to Astro Starlight, commits, and
redeploys. Nothing is edited by hand on that side; changes made there under `esw/` are lost on the
next sync.

The converter (`scripts/sync_esw.py` in mrover-docs) understands four differences:

- a `# Title` as the first line of every page, lifted into Starlight frontmatter,
- relative `.md` links, resolved and rewritten to absolute site paths,
- `!!! type` admonitions with a 4-space indented body,
- the `nav` table in `zensical.toml`, which becomes the ESW sidebar.

Stay inside those and the mirror needs no attention. A page missing from `nav` is missing from the
docs.mrover.org sidebar too, and a page with no H1 fails the sync outright.

## Navigation

Pages do not appear automatically. Every page must be listed in the `nav` table in
`zensical.toml`, and a nav entry pointing at a missing file breaks the build. Top-level sections
render as tabs.
