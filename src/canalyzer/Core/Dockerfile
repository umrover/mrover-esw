FROM gcc:13

RUN apt-get update && \
    apt-get install -y cmake libstdc++6 && \
    rm -rf /var/lib/apt/lists/*

# lib pulls in tools, which needs uv to configure
# same uv version as Dockerfile.arm-gnu, keep them in sync
COPY --from=ghcr.io/astral-sh/uv:0.12.15 /uv /usr/local/bin/

WORKDIR /usr/src/app

# Context is the repo root, so this brings in src/, lib/, everything
# this will copy all of mrover-esw
COPY . .

# Build into a directory that can't possibly collide with a host artifact
# only build canalyzer, the default target also runs uv sync and the python dbc header gen which we don't need
    RUN cmake -S src/canalyzer -B /build && \
    cmake --build /build --target canalyzer -j"$(nproc)"

ENTRYPOINT ["/build/canalyzer", "/app/config.yaml", "/app/dbc/"]