#ifndef SCIENCE_SENSOR_HPP
#define SCIENCE_SENSOR_HPP

namespace mrover {
    class ScienceSensor {
    private:
        bool state;

    public:
        ScienceSensor()
            : state(true) {}

        // flags sensor fault
        void flag() {
            state = false;
        }

        // clears sensor fault
        void clear() {
            state = true;
        }

        // returns the state of the sensor
        bool get_state() const {
            return state;
        }

        // attempts to restart the sensor, returns true on success and false on failure
        bool restart() {
            if (!get_state()) {
                if (init()) {
                    clear();
                    return true;
                }
            }

            return false;
        }

        // updates the value of the sensor
        virtual void update() = 0;

        // polls the sensor for data
        virtual void poll() = 0;

        // attempts to initialize sensor, returns true on success and false on failure
        virtual bool init() = 0;
    };
}

#endif