
#include <DynamixelShield.h>

DynamixelShield dxl;
//const uint8_t DXL_ID = 0; // check label on Dynamixel

const float DXL_PROTOCOL_VERSION = 2.0;

using namespace ControlTableItem;

void setup() {
  
  Serial.begin(115200); // Match the baud rate with the C++ program
  
  // Set Port baudrate to 57600bps. This has to match with DYNAMIXEL baudrate.
  dxl.begin(57600);

  // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

}

void loop() {
  if (Serial.available()) {
    String input = Serial.readStringUntil('\n');  // Read the incoming data until newline
    input.trim();  // Remove extra whitespace or newline characters

    // Check if the input is not empty
    if (input.length() > 0) {
      int spaceIndex = input.indexOf(' ');  // Find the space separating ID and Degrees
      if (spaceIndex != -1) {
        // Extract ID and Degrees from the input string
        int id = input.substring(0, spaceIndex).toInt();  // Get the ID (before the space)
        int deg_in = input.substring(spaceIndex + 1).toInt();  // Get Degrees (after the space)

        // Validate parsed values
        if (id >= 0 && deg_in != 0 || input.substring(spaceIndex + 1) == "0") {

          // Get DYNAMIXEL information
          dxl.ping(id);
          
          // Turn off torque when configuring items in EEPROM
          // Operating Mode 4 is Extended Position Control Mode
          dxl.torqueOff(id);
          dxl.setOperatingMode(id, 4);
          dxl.torqueOn(id);


          // Adjust the motor position for the specified ID
          int new_position = dxl.getPresentPosition(id) + (deg_in * (511 / 45));
          dxl.setGoalPosition(id, new_position);

          while (true) {
            Serial.print("Present Position: ");
            Serial.print(dxl.getPresentPosition(id));
            Serial.print("\t\t");
            if(dxl.getPresentPosition(id) == new_position) {
              break;
            }
            delay(100);
          }

          delay(1000);

          // Send back the ID and new position
          Serial.print("ID: ");
          Serial.print(id);
          Serial.print(", Current Position: ");
          Serial.println(dxl.getPresentPosition(id));

        }
      }
    } 
  }
}
