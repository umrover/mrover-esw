# Building These Docs

These docs are served using [zensical](https://zensical.org/) and are built from the source files in this repository.

Zensical and its plugins are declared in the `docs` dependency group in `tools/pyproject.toml`
and managed with [uv](https://docs.astral.sh/uv/). There is nothing to install first — from the
root directory of the repo, run:

```bash
./scripts/docs.sh
```

which runs `zensical serve` through `uv run`, creating and syncing `tools/.venv` if it does not
already exist.
