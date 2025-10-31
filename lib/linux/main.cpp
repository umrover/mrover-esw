
#include "Inc/serial/serial.hpp"
#include <boost/asio/io_context.hpp>
#include <cstdlib>
#include <cstring>
#include <thread>


void receive(uint8_t* data, size_t size)
{
  uint8_t* data_string = (uint8_t*)malloc(size + 1);
  data_string[size] = 0;
  memcpy(data_string, data, size);
  printf("RECEIVE: %s\n", data_string);
  free(data_string);
}


int main()
{
  boost::asio::io_context io;

  // creates a port object and adds a device
  std::function<void(uint8_t*, size_t)> callback = receive;
  SerialPort port(io, "/dev/ttyACM0", 9600, callback);

  
  // begins receiving
  port.beginReceiving();

  std::thread io_thread([&io]() {
    std::cout << "IO thread started\n";
    io.run();
    std::cout << "IO thread finished\n";
  });

  uint8_t data[] = {'\r', '\n', 'P', 'I', 'N', 'G', '\r', '\n'};

  while (true)
  {
    port.write(data, sizeof(data));
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }




  return 1; 
}
