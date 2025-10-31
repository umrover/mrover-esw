#include "main.h"

namespace mrover {

    // TODO instantiate your class here to test

    auto init() -> void {
        // TODO initialization logic here
    }

    [[noreturn]] auto loop() -> void {

        for ( ;; ) {
            // TODO infinite loop logic here
        }

    }

} // namespace mrover

extern "C" {

    void PostInit() {
        mrover::init();
    }

    void Loop() {
        mrover::loop();
    }

}
