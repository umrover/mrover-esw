#include <iostream>
#include <fcntl.h>      // File control definitions
#include <unistd.h>     // UNIX standard function definitions
#include <termios.h>    // POSIX terminal control definitions
#include <cstring>      // For memset
#include <sstream>      // For stringstream

int main() {
    const char *device = "/dev/ttyACM0"; // Replace with the appropriate device
    int baudrate = B115200;               // Match the Arduino's baud rate

    // Open the serial port
    int serial_port = open(device, O_RDWR | O_NOCTTY | O_SYNC);
    if (serial_port < 0) {
        std::cerr << "Error: Unable to open serial port " << device << std::endl;
        return 1;
    }

    // Configure the serial port
    struct termios tty;
    memset(&tty, 0, sizeof tty);

    if (tcgetattr(serial_port, &tty) != 0) {
        std::cerr << "Error: Unable to get terminal attributes" << std::endl;
        close(serial_port);
        return 1;
    }

    // Set baud rate
    cfsetospeed(&tty, baudrate);
    cfsetispeed(&tty, baudrate);

    // 8N1 mode: 8 data bits, no parity, 1 stop bit
    tty.c_cflag = (tty.c_cflag & ~CSIZE) | CS8;
    tty.c_iflag &= ~IGNBRK;
    tty.c_lflag = 0;             // No signaling chars, no echo, no canonical processing
    tty.c_oflag = 0;             // No remapping, no delays
    tty.c_cc[VMIN] = 1;          // Read blocks until at least 1 byte is available
    tty.c_cc[VTIME] = 1;         // Timeout for read

    tty.c_iflag &= ~(IXON | IXOFF | IXANY);  // Disable software flow control
    tty.c_cflag |= (CLOCAL | CREAD);         // Enable receiver, local mode
    tty.c_cflag &= ~(PARENB | CSTOPB | CRTSCTS); // No parity, 1 stop bit, no flow control

    if (tcsetattr(serial_port, TCSANOW, &tty) != 0) {
        std::cerr << "Error: Unable to set terminal attributes" << std::endl;
        close(serial_port);
        return 1;
    }

    char read_buffer[256];  // Buffer to store the incoming data
    memset(&read_buffer, 0, sizeof(read_buffer));

    // Continuously read numbers from user and send them to the Arduino
    int deg, id;
    while (true) {
        std::cout << "Enter ID: ";
        std::cin >> id; // Read the number
        std::cout << "Enter a number (degrees to spin): ";
        std::cin >> deg; // Read the number

        // Format the message with the single number
        std::stringstream ss;
        ss << id << " " << deg << "\n"; // Format as "num\n"
        std::string message = ss.str();

        // Send the formatted message to the serial port
        ssize_t bytes_written = write(serial_port, message.c_str(), message.length());

        if (bytes_written < 0) {
            std::cerr << "Error: Unable to write to serial port" << std::endl;
            close(serial_port);
            return 1;
        }

        std::cout << "Sent: " << message;


        // Read data from Arduino
        sleep(3);

        ssize_t bytes_read = read(serial_port, &read_buffer, sizeof(read_buffer) - 1);
        if (bytes_read > 0) {
            read_buffer[bytes_read] = '\0';  // Null-terminate the string
            std::cout << "Arduino says: " << read_buffer << std::endl;
        }
    }

    // Close the serial port (this will never be reached as the loop is infinite)
    close(serial_port);
    return 0;
}
