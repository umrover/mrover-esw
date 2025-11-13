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

    for (auto const& message: parser.messages()) {
        std::cout << message << "\n";
    }

    assert(parser.messages().size() == 3);

    // Science_Sensors
    {
        assert(parser.message(80) != nullptr);
        CanMessageDescription const* science_message = parser.message(80);
        assert(science_message->is_valid());
        assert(science_message->name() == "Science_Sensors");
        assert(science_message->id() == 80);
        assert(science_message->length() == 20);
        assert(science_message->transmitter() == "science");
        assert(science_message->signals_size() == 5);
        assert(science_message->comment() == "sensor information from the\nscience\n\nboard\n");

        CanSignalDescription const* temp_signal = science_message->signal("Sensors_Temperature");
        assert(temp_signal != nullptr);
        assert(temp_signal->bit_start() == 0);
        assert(temp_signal->bit_length() == 32);
        assert(temp_signal->endianness() == Endianness::LittleEndian);
        assert(temp_signal->data_format() == DataFormat::SignedInteger);
        assert(temp_signal->factor_offset_used() == false);
        assert(temp_signal->minimum() == -3.4e+38);
        assert(temp_signal->maximum() == 3.4e+38);
        assert(temp_signal->unit() == "celsius");
        assert(temp_signal->receiver() == "jetson");

        CanSignalDescription const* humidity_signal = science_message->signal("Sensors_Humidity");
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

        CanSignalDescription const* uv_signal = science_message->signal("Sensors_UV");
        assert(uv_signal != nullptr);
        assert(uv_signal->bit_start() == 64);
        assert(uv_signal->bit_length() == 32);
        assert(uv_signal->endianness() == Endianness::LittleEndian);
        assert(uv_signal->data_format() == DataFormat::Float);
        assert(uv_signal->factor_offset_used() == false);
        assert(uv_signal->minimum_maximum_used() == false);
        assert(uv_signal->unit() == "");
        assert(uv_signal->receiver() == "jetson");

        CanSignalDescription const* oxygen_signal = science_message->signal("Sensors_Oxygen");
        assert(oxygen_signal != nullptr);
        assert(oxygen_signal->bit_start() == 96);
        assert(oxygen_signal->bit_length() == 32);
        assert(oxygen_signal->endianness() == Endianness::LittleEndian);
        assert(oxygen_signal->data_format() == DataFormat::Float);
        assert(oxygen_signal->factor_offset_used() == false);
        assert(oxygen_signal->minimum_maximum_used() == false);
        assert(oxygen_signal->unit() == "");
        assert(oxygen_signal->receiver() == "jetson");

        CanSignalDescription const* co2_signal = science_message->signal("Sensors_CO2");
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

    {
        struct ScienceSensorsTestMessage {
            int32_t temperature = -22;
            float humidity = 55.0f;
            float uv = 3.2f;
            float oxygen = 20.8f;
            float co2 = 415.0f;
        } __attribute__((packed));

        ScienceSensorsTestMessage ScienceSensorsTestMessage{};

        CanMessageDescription const* science_message = parser.message(80);
        CanFrameProcessor frame_processor;
        frame_processor.add_message_description(*science_message);
        auto decoded_signals = frame_processor.decode(80, std::string_view(
                                                                  reinterpret_cast<char const*>(&ScienceSensorsTestMessage),
                                                                  sizeof(ScienceSensorsTestMessage)));
        assert(decoded_signals.size() == 5);
        assert(std::holds_alternative<int32_t>(decoded_signals.at("Sensors_Temperature")));
        assert(std::get<int32_t>(decoded_signals.at("Sensors_Temperature")) == -22);
        assert(std::holds_alternative<float>(decoded_signals.at("Sensors_Humidity")));
        assert(std::get<float>(decoded_signals.at("Sensors_Humidity")) == 55.0f);
        assert(std::holds_alternative<float>(decoded_signals.at("Sensors_UV")));
        assert(std::get<float>(decoded_signals.at("Sensors_UV")) == 3.2f);
        assert(std::holds_alternative<float>(decoded_signals.at("Sensors_Oxygen")));
        assert(std::get<float>(decoded_signals.at("Sensors_Oxygen")) == 20.8f);
        assert(std::holds_alternative<float>(decoded_signals.at("Sensors_CO2")));
        assert(std::get<float>(decoded_signals.at("Sensors_CO2")) == 415.0f);

        std::cout << "Decoded Signals from Science_Sensors message:\n";
        for (auto const& [name, value]: decoded_signals) {
            std::cout << "  " << name << ": ";
            if (std::holds_alternative<float>(value)) {
                std::cout << static_cast<float>(std::get<float>(value));
            } else if (std::holds_alternative<int32_t>(value)) {
                std::cout << static_cast<int32_t>(std::get<int32_t>(value));
            } else {
                std::cout << "Unknown Type";
            }
        }
    }


    return 0;
}
