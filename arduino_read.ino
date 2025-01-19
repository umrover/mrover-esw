
#include <DynamixelShield.h>

DynamixelShield dxl;
const uint8_t DXL_ID = 0; // check label on Dynamixel

const float DXL_PROTOCOL_VERSION = 2.0;

using namespace ControlTableItem;

void setup() {
  
  Serial.begin(115200); // Match the baud rate with the C++ program
  
  // Set Port baudrate to 57600bps. This has to match with DYNAMIXEL baudrate.
  dxl.begin(57600);

  // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

  // Get DYNAMIXEL information
  dxl.ping(DXL_ID);

  // Turn off torque when configuring items in EEPROM
  // Operating Mode 4 is Extended Position Control Mode
  dxl.torqueOff(DXL_ID);
  dxl.setOperatingMode(DXL_ID, 4);
  dxl.torqueOn(DXL_ID);

}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // Read the incoming data until newline
    input.trim();  // Remove extra whitespace or newline characters

    // Check if the input is not empty or invalid
    if (input.length() > 0) {
      int deg_in = input.toInt();  // Convert the string to an integer
      if (deg_in != 0 || input == "0") {  // Only process if conversion is valid (deg_in != 0, or input is exactly "0")
        // Adjust the motor position
        int new_position = dxl.getPresentPosition(DXL_ID) + (deg_in * (511 / 45)); // conversion isn't perfect
        dxl.setGoalPosition(DXL_ID, new_position);
      }
    } 
  }
}
