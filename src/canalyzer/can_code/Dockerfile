FROM gcc:latest

WORKDIR /can_code

COPY Yaml.cpp .
COPY Yaml.hpp .
COPY influxdb.hpp .
COPY driver.cpp .
COPY logger.cpp .
COPY logger.hpp .

RUN g++ -Wall -Werror driver.cpp logger.cpp Yaml.cpp -o socketcan.out

ENTRYPOINT [ "/can_code/socketcan.out" ]
