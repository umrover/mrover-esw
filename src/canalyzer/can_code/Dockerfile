FROM gcc:latest

WORKDIR /can_code

COPY Yaml.cpp .
COPY Yaml.hpp .
COPY influxdb.hpp .
COPY driver.cpp .
COPY logger.cpp .
COPY logger.hpp .
COPY logger_start.yaml .

RUN g++ -Wall -Werror driver.cpp logger.cpp Yaml.cpp -o socketcan.out

ENTRYPOINT ["bash", "-c", "stdbuf -oL -eL ./socketcan.out 2>&1 | tee /var/log/can_logger/logger.log"]