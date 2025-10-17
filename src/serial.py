#!/usr/bin/env python3

# install this with `pip install pyserial`
import serial
import csv

ready = True
#py file creates csv at the end of what it does

def read(serial_handle):
    if (ser.Serial.in_waiting > 0):
        line = serial_handle.readline()
        fileWrite(line.decode('utf-8', errors='ignore').strip())
    return
    

def write(serial_handle, data):
    serial_handle.write(data.encode('utf-8'))

def fileWrite(line):
    temp = []
    for word in line:
        temp.append(word)
    with open('accelData.csv', 'a', newline='') as csvfile:
        writer = csv.writer(csvfile)
        writer.writerow(temp)

if __name__ == "__main__":
    ser = serial.Serial('INSERT_PORT_HERE', 115200)

header = ['sensor','timestamp','x', 'y', 'z']
with open('accelData.csv', 'w', newline='') as csvfile:
    writer = csv.writer(csvfile)#, delimiter=' ',
                            #quotechar='|', quoting=csv.QUOTE_MINIMAL)
    writer.writerow(header)


while(1):
    read(ser)




