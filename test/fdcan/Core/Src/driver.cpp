#include "fdcan.hpp"

namespace mrover {
    FDCAN fdcan;

    void init() {
        while (true) {
        }
    }
} // namespace mrover

extern "C" {
void HAL_PostInit() {
    mrover::init();
}
}
