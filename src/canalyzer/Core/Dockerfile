# -----------------------------------------------------------------------------
# Stage 1: Build the Application
# CHANGE: Use 'bookworm' (Debian 12). It comes with GCC 13 and CMake 3.25 built-in.
# -----------------------------------------------------------------------------
FROM gcc:13

# 1. Install CMake
#    (Debian 12 "Bookworm" repos include CMake 3.25, so no pip needed)
RUN apt-get update && \
    apt-get install -y cmake libstdc++6 && \
    rm -rf /var/lib/apt/lists/*

# 2. Accept the argument from the shell script
ARG PROJECT_REL_PATH
WORKDIR /usr/src/repo

# 3. Copy the entire repository
COPY . .

# 4. Navigate to project folder
WORKDIR /usr/src/repo/${PROJECT_REL_PATH}

# 5. Build
#    Note: In this image, the command "g++" IS ALREADY version 13.
#    We don't need to specify CMAKE_CXX_COMPILER unless you really want to be explicit.
RUN rm -rf build && \
    cmake -S . -B build && \
    cmake --build build


# Run the application
ENTRYPOINT ["./build/canalyzer", "../canalyzer_config.yaml"]