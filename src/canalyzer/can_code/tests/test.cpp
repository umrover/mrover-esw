#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <stdio.h>
#include <string.h>
#include <string>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <thread>
#include <unistd.h>
#include <vector>

int helper(std::string const& can_name);

int main() {
    std::vector<std::thread> threads;
    threads.reserve(4);
    for (int i = 0; i < 4; ++i) {
        std::string ifname = "vcan" + std::to_string(i);
        threads.emplace_back(helper, ifname);
    }
    while (true) {
        sleep(1);
    }
    return 0;
}

int helper(std::string const& can_name) {
    for (int i = 0; i < 100; ++i) {
        int s;
        struct sockaddr_can addr;
        struct ifreq ifr;
        struct canfd_frame frame, read_frame;

        // Open raw CAN socket
        s = socket(PF_CAN, SOCK_RAW, CAN_RAW);
        if (s < 0) {
            perror("Socket");
            return 1;
        }

        // Locate the interface index
        strcpy(ifr.ifr_name, can_name.c_str());
        if (ioctl(s, SIOCGIFINDEX, &ifr) < 0) {
            perror("ioctl");
            return 1;
        }

        addr.can_family = AF_CAN;
        addr.can_ifindex = ifr.ifr_ifindex;

        if (bind(s, (struct sockaddr*) &addr, sizeof(addr)) < 0) {
            perror("Bind");
            return 1;
        }

        // Enable CAN FD support on socket
        int enable_canfd = 1;
        if (setsockopt(s, SOL_CAN_RAW, CAN_RAW_FD_FRAMES,
                       &enable_canfd, sizeof(enable_canfd)) != 0) {
            perror("setsockopt CAN_RAW_FD_FRAMES");
            return 1;
        }


        // Prepare CAN FD frame
        frame.can_id = 0x123;
        frame.len = 16;  // CAN FD supports up to 64
        frame.flags = 0; // no BRS, no ESI
        for (int i = 0; i < frame.len; i++) {
            frame.data[i] = i;
        }

        // Send CAN FD frame

        ssize_t nbytes = write(s, &frame, sizeof(frame));
        if (nbytes < 0) {
            perror("Write");
            return 1;
        } else if (nbytes != sizeof(frame)) {
            fprintf(stderr, "Partial CAN frame write: %zd bytes\n", nbytes);
            return 1;
        }
    }
    return 0;
}
