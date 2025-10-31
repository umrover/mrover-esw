#include <array>
#include <boost/asio.hpp>
#include <boost/asio/impl/write.hpp>
#include <boost/asio/placeholders.hpp>
#include <boost/bind.hpp>
#include <boost/system/detail/error_code.hpp>
#include <cstdint>
#include <functional>
#include <iostream>
#include <utility>

#define BUFFER_SIZE 128


class SerialPort
{
public: 

  /**
  * Make sure to wait until the device is online
  */
  SerialPort(boost::asio::io_context& io, const std::string& device, uint32_t baud_rate, std::function<void(uint8_t*, size_t)>& receive_callback)
  : ctx(io), port(io), callback(receive_callback), buffer{}
  {
    // open device and set options
    port.open(device);
    port.set_option(boost::asio::serial_port::baud_rate(baud_rate));
    port.set_option(boost::asio::serial_port_base::character_size(8));
    port.set_option(boost::asio::serial_port_base::flow_control(boost::asio::serial_port_base::flow_control::none));
    port.set_option(boost::asio::serial_port_base::parity(boost::asio::serial_port_base::parity::none));
    port.set_option(boost::asio::serial_port_base::stop_bits(boost::asio::serial_port_base::stop_bits::one));

    // make sure port is open
    if (port.is_open())
    {
      std::cout << "Opened serial port with baud rate: " << baud_rate << "!\n";
    }
    else {
      std::cout << "Failed to open serial port!\n";
    }
  }

  void beginReceiving()
  {
    receiving = true;
    receive();
  }

  void stopReceiving()
  {
    // will stop receiving after last receive
    receiving = false;
  }

  /**
  * Writes data to device
  * @return number of bytes written
  */
  size_t write(uint8_t* data, size_t size)
  {
    return boost::asio::write(port, boost::asio::buffer(data, size));
  }

private:

  void receive()
  {
    // Begin receiving bytes by registering callback with buffer
    port.async_read_some(
      boost::asio::buffer(buffer, maxMessageLength),
      boost::bind(&SerialPort::handle_receive, this, 
                        boost::asio::placeholders::error,
                        boost::asio::placeholders::bytes_transferred)
    );
  }

  void handle_receive(const boost::system::error_code& error, size_t bytes_transferred) {
    if (!error) {
        // Process the received data
        callback((uint8_t*)buffer.data(), bytes_transferred);
        
        // after the receive is processed, receive more data
        if (receiving) receive();
    } else {
        std::cerr << "Error during serial read: " << error.message() << std::endl;
    }
  }

private:

  static constexpr int maxMessageLength = BUFFER_SIZE;
  boost::asio::serial_port port;
  boost::asio::io_context& ctx;
  std::array<char, maxMessageLength> buffer;
  bool receiving = true;

  std::function<void(uint8_t*, size_t)>& callback;

};
