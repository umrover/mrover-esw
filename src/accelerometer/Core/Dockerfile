FROM gcc:13

RUN apt-get update && \
    apt-get install -y cmake python3 python3-venv libstdc++6 && \
    rm -rf /var/lib/apt/lists/*

WORKDIR /usr/src/app

# Context is the repo root, so this brings in src/, lib/, tools/
COPY . .

# Build into a directory that can't possibly collide with a host artifact
RUN cmake -S src/accelerometer -B /build && \
    cmake --build /build --target accelerometer -j"$(nproc)"

ENTRYPOINT ["/build/accelerometer", "/app/config.yaml"]
