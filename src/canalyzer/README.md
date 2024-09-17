# CANalyzer

***DEVELOPMENT FOR THIS PROJECT TAKES PLACE ON THE `canalyzer` BRANCH***

CANalyzer will be a terminal-based CAN bus analyzer tool designed to run on Linux. It will utilize `ncurses` for a text-based terminal user interface and `socketcan` to interface with the CAN bus.

## Project Goals

- **Real-time Monitoring:** Display CAN bus messages in a structured and easy-to-read format
- **Custom CAN Message Handling:** Implement support for our team's custom CAN message format
- **Filter and Search:** Implement filters to allow users to focus on specific CAN IDs or data patterns
- **Logging:** Enable saving of CAN bus data to a file for later analysis
