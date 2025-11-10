#include "dbc.hpp"

#include <cassert>
#include <iostream>

using namespace mrover::dbc;

auto main(int argc, char** argv) -> int {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " <dbc_file>" << std::endl;
        return 1;
    }
    std::string dbc_filename = argv[1];
    mrover::dbc::CanDbcFileParser parser;
    if (!parser.parse(dbc_filename)) {
        std::cerr << "Failed to parse DBC file: " << dbc_filename << std ::endl;
        std::cerr << "Error after parsing " << parser.lines_parsed() << " lines." << std::endl;
        if (parser.is_error()) {
            std::cerr << "Error: " << parser.error() << std::endl;
        }
        return 1;
    }

    for (auto const& [id, message]: parser.messages()) {
        std::cout << message << "\n";
    }

    assert(parser.messages().size() == 3);

    // Science_Sensors
    {
        assert(parser.messages().find(80) != parser.messages().end());
        CanMessageDescription const& science_message = parser.messages().at(80);
        assert(science_message.is_valid());
        assert(science_message.name() == "Science_Sensors");
        assert(science_message.id() == 80);
        assert(science_message.length() == 20);
        assert(science_message.transmitter() == "science");
        assert(science_message.signals_size() == 5);
        assert(science_message.comment() == "sensor information from the\nscience\n\nboard\n");

        CanSignalDescription const* temp_signal = science_message.signal("Sensors_Temperature");
        assert(temp_signal != nullptr);
        assert(temp_signal->bit_start() == 0);
        assert(temp_signal->bit_length() == 32);
        assert(temp_signal->endianness() == Endianness::LittleEndian);
        assert(temp_signal->data_format() == DataFormat::Float);
        assert(temp_signal->factor_offset_used() == false);
        assert(temp_signal->minimum() == -3.4e+38);
        assert(temp_signal->maximum() == 3.4e+38);
        assert(temp_signal->unit() == "celsius");
        assert(temp_signal->receiver() == "jetson");

        CanSignalDescription const* humidity_signal = science_message.signal("Sensors_Humidity");
        assert(humidity_signal != nullptr);
        assert(humidity_signal->bit_start() == 32);
        assert(humidity_signal->bit_length() == 32);
        assert(humidity_signal->endianness() == Endianness::LittleEndian);
        assert(humidity_signal->data_format() == DataFormat::Float);
        assert(humidity_signal->factor_offset_used() == false);
        assert(humidity_signal->minimum() == -3.4e+38);
        assert(humidity_signal->maximum() == 3.4e+38);
        assert(humidity_signal->unit() == "percent");
        assert(humidity_signal->receiver() == "jetson");
        assert(humidity_signal->comment() == "percent relative humidity");

        CanSignalDescription const* uv_signal = science_message.signal("Sensors_UV");
        assert(uv_signal != nullptr);
        assert(uv_signal->bit_start() == 64);
        assert(uv_signal->bit_length() == 32);
        assert(uv_signal->endianness() == Endianness::LittleEndian);
        assert(uv_signal->data_format() == DataFormat::Float);
        assert(uv_signal->factor_offset_used() == false);
        assert(uv_signal->minimum_maximum_used() == false);
        assert(uv_signal->unit() == "");
        assert(uv_signal->receiver() == "jetson");

        CanSignalDescription const* oxygen_signal = science_message.signal("Sensors_Oxygen");
        assert(oxygen_signal != nullptr);
        assert(oxygen_signal->bit_start() == 96);
        assert(oxygen_signal->bit_length() == 32);
        assert(oxygen_signal->endianness() == Endianness::LittleEndian);
        assert(oxygen_signal->data_format() == DataFormat::Float);
        assert(oxygen_signal->factor_offset_used() == false);
        assert(oxygen_signal->minimum_maximum_used() == false);
        assert(oxygen_signal->unit() == "");
        assert(oxygen_signal->receiver() == "jetson");

        CanSignalDescription const* co2_signal = science_message.signal("Sensors_CO2");
        assert(co2_signal != nullptr);
        assert(co2_signal->bit_start() == 128);
        assert(co2_signal->bit_length() == 32);
        assert(co2_signal->endianness() == Endianness::LittleEndian);
        assert(co2_signal->data_format() == DataFormat::Float);
        assert(co2_signal->factor_offset_used() == false);
        assert(co2_signal->minimum_maximum_used() == false);
        assert(co2_signal->unit() == "");
        assert(co2_signal->receiver() == "");
    }


    return 0;
}
