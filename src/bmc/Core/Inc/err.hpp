#pragma once

namespace mrover {

    enum struct bmc_error_t : std::uint8_t {

        // no error
        NONE,

        // no mode selected
        NO_MODE,

        // in position or velocity mode without feedback mechanism configured to close the loop
        INVALID_CONFIGURATION_FOR_MODE,

        // watchdog expired
        WWDG_EXPIRED,

        // unrecoverable CAN error encountered
        CAN_ERROR_FATAL,

        // unrecoverable I2C error encountered
        I2C_ERROR_FATAL,

        // unrecoverable SPI error encountered
        SPI_ERROR_FATAL,

        // received position or velocity mode before calibrated
        UNCALIBRATED,

    };


} // namespace mrover
