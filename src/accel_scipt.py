import serial
import csv

def read(serial_handle):
    if (serial_handle.in_waiting > 0):
        line = serial_handle.readline()
        fileWrite(line.decode('utf-8', errors='ignore').strip())
    return
    
def write(serial_handle, data):
    serial_handle.write(data.encode('utf-8'))

# data must be in the following format (otherwise the function will need to be updated):
# no extra commas or colons
# sensor_id timestamp x_val y_val z_val
def fileWrite(line):
    temp = line.split()
    
    with open('accel_data.csv', 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(temp)


if __name__ == "__main__":
    ser = serial.Serial('/dev/ttyACM0', 115200)

    header = ['sensor','timestamp','x', 'y', 'z']
    with open('accel_data.csv', 'w', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(header)


    while(1):
        read(ser)




