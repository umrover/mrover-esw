FROM gcc:13

RUN apt-get update && \
    apt-get install -y cmake libstdc++6 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app

# Context is the repo root, so this brings in src/, lib/, everything
# this will copy all of mrover-esw
COPY . .

# Build into a directory that can't possibly collide with a host artifact
RUN cmake -S src/canalyzer -B /build && \
    cmake --build /build -j"$(nproc)"

ENTRYPOINT ["/build/canalyzer", "/app/config.yaml", "/app/dbc/"]