import serial
import re

# Function to send data through UART
def send_data(ser, data):
    if ser:
        try:
            ser.write(data.encode())
            print(f"Sent data: {data}")
        except serial.SerialException as e:
            print(f"Failed to send data: {e}")

# Function to receive data through UART
def receive_data(ser):
    if ser:
        try:
            incoming_data = ser.readline().decode().strip()
            print(f"Received data: {incoming_data}")
            return incoming_data
        except serial.SerialException as e:
            print(f"Failed to receive data: {e}")
            return None

# Function to open UART port with default configuration
def open_uart_port(port='/dev/ttyUSB1', baudrate=115200):
    try:
        ser = serial.Serial(
            port=port,
            baudrate=baudrate,  # Set baudrate dynamically
            parity=serial.PARITY_NONE,  # No parity
            stopbits=serial.STOPBITS_ONE,  # One stop bit
            bytesize=serial.EIGHTBITS  # 8 bit
        )
        print(f"UART port {port} opened with baudrate {baudrate}")
        return ser
    except serial.SerialException as e:
        print(f"Failed to open UART port {port}: {e}")
        return None

# Function to receive data until a newline character and return the buffer
def receive_until_newline(ser):
    buffer = ''
    if ser:
        try:
            while True:
                char = ser.read().decode()
                if char == '\n':
                    break
                buffer += char
            print(f"SOCFPGA_UART: {buffer}")
            return buffer
        except serial.SerialException as e:
            print(f"Failed to receive data until newline: {e}")
            return None

# Function to echo back received bytes
def echo_received_bytes(ser, num_bytes):
    echoed_bytes = 0
    if ser:
        try:
            while echoed_bytes < num_bytes:
                if ser.in_waiting > 0:
                    char = ser.read(1)  # Read one byte
                    ser.write(char)  # Echo back the received byte
                    echoed_bytes += 1
            print(f"Echoed {echoed_bytes} bytes.")
        except serial.SerialException as e:
            print(f"Failed to echo received bytes: {e}")

# Function to change the baud rate dynamically
def change_baud_rate(ser, new_baudrate):
    if ser:
        ser.baudrate = new_baudrate
        print(f"Baud rate changed to {new_baudrate}")

# Function to handle different tests including loopback and baudrate changes
def run_tests(ser):
    try:
        while True:
            received_line = receive_until_newline(ser)

            if received_line.startswith("SET LOOPBACK"):
                # Loopback test logic
                num_bytes = int(received_line.split(" ")[2])
                print(f"Loopback test for {num_bytes} bytes")
                echo_received_bytes(ser, num_bytes)
            
            elif received_line.startswith("SET BAUD"):
                # Baud rate change logic
                new_baudrate = int(received_line.split(" ")[2])
                print(f"Changing baud rate to {new_baudrate}")
                change_baud_rate(ser, new_baudrate)
            
            elif "TEST PASSED" in received_line:
                print("TEST PASSED")
            
            elif "TEST FAILED" in received_line:
                print("TEST FAILED")

            elif "TESTS COMPLETED" in received_line:
                # This line contains the result summary, print it and exit the loop
                print(received_line)
                break

    except serial.SerialException as e:
        print(f"Error during tests: {e}")

# Main function to start tests
def main():
    ser = open_uart_port()
    if ser:
        run_tests(ser)

if __name__ == "__main__":
    main()
