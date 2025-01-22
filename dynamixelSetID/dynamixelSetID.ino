#include <DynamixelShield.h>

#if defined(ARDUINO_AVR_UNO) || defined(ARDUINO_AVR_MEGA2560)
  #include <SoftwareSerial.h>
  SoftwareSerial soft_serial(7, 8); // DYNAMIXELShield UART RX/TX
  #define DEBUG_SERIAL soft_serial
#elif defined(ARDUINO_SAM_DUE) || defined(ARDUINO_SAM_ZERO)
  #define DEBUG_SERIAL SerialUSB    
#else
  #define DEBUG_SERIAL Serial
#endif

const float DXL_PROTOCOL_VERSION = 2.0;

const uint8_t DXL_ID_BASE = 4; // Factory Set DXL ID set to 1
const uint8_t DXL_ID_NEW = 4; // Select New ID


DynamixelShield dxl;

//This namespace is required to use Control table item names
using namespace ControlTableItem;

void setup() {
  
  // Read Output from Serial Monitor (115200 baud)
  DEBUG_SERIAL.begin(115200);
  
  // Wait for the serial port to be ready
  while (!DEBUG_SERIAL);

  // Set Port baudrate to 57600bps. This has to match with DYNAMIXEL baudrate.
  dxl.begin(57600);
  // Set Port Protocol Version. This has to match with DYNAMIXEL protocol version.
  dxl.setPortProtocolVersion(DXL_PROTOCOL_VERSION);

  // Check if Dynamixel with DXL_ID_BASE is connected
  if (dxl.ping(DXL_ID_BASE)) {

    DEBUG_SERIAL.println(String("DXL ID ") + String(DXL_ID_BASE) + String(" Found."));

    // Turn off torque when configuring items in EEPROM area
    dxl.torqueOff(DXL_ID_BASE);
    dxl.setID(DXL_ID_BASE, DXL_ID_NEW);
    dxl.torqueOn(DXL_ID_NEW);

    delay(1000);

    // Check conversion status
    String newIdStatus = dxl.ping(DXL_ID_NEW) ? ": Success" : ": Failed";
    DEBUG_SERIAL.println(String("DXL ID ") + String(DXL_ID_BASE) + String(" Set To ID ") + String(DXL_ID_NEW) + newIdStatus);
    
  } else {
    DEBUG_SERIAL.println(String("DXL ID ") + String(DXL_ID_BASE) + String(" Not Found."));
  }
 

}

void loop() {
  
}
