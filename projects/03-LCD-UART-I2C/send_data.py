import serial
import time

# Configure and open the serial port
baud_rate = 9600
port = 'COM7'  
ser = serial.Serial(port, baud_rate) 

time.sleep(0.5)  

# Send command to STM32 to display text on LCD
try:
    message = "HELLO STM32\n"
    ser.write(message.encode('utf-8'))
    print (f"Sent message: {message.strip()}")
    
except Exception as e:
    print(f"Error sending data: {e}")