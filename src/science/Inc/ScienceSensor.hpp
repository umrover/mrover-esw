#ifndef SCIENCE_SENSOR_HPP
#define SCIENCE_SENSOR_HPP

namespace mrover {
    class ScienceSensor {
    private:
        bool m_state;

    public:
        ScienceSensor()
            : m_state(true) {}

        // flags sensor fault
        void flag() {
            m_state = false;
        }

        // clears sensor fault
        void clear() {
            m_state = true;
        }

        // returns the state of the sensor
        bool get_state() const {
            return m_state;
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