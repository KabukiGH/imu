import csv
import socket
import time

# Path to the CSV file
csv_file_path = 'E:/Programs/C/Projects/Embevity/imu/mydata.csv'

# List to store data
file_data = []

# Reading the CSV file
try:
    with open(csv_file_path, newline='') as file:
        csv_reader = csv.reader(file)

        # Iterate through the first six rows
        for i, row in enumerate(csv_reader):
            if i < 6:
                file_data.append(row)
            else:
                break  # Stop after reading six rows
except FileNotFoundError:
    print(f"The file {csv_file_path} was not found.")
except Exception as e:
    print(f"An error occurred while reading the file: {e}")

# Frequency and delay
FREQUENCY_HZ = 50
DELAY_S = 1.0 / FREQUENCY_HZ


# Function to start the TCP/IP server
def start_server(host='127.0.0.1', port=65432):
    # Create a TCP/IP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((host, port))
        server_socket.listen()

        print(f"Server listening on {host}:{port}")

        while True:
            # Wait for a connection
            conn, addr = server_socket.accept()
            with conn:
                print(f"Connected by {addr}")
                # Start from 1 row, as 1st include columns names ax, ay az ..
                current_index = 1
                conn.settimeout(60)  # Set timeout to 60 seconds

                while current_index < len(file_data):
                    try:
                        # Wait for a request from the client
                        request = conn.recv(1024).decode('utf-8')

                        if request.strip().lower() == 'send':
                            # Get the current row
                            row = file_data[current_index]
                            # Increment the index for the next row
                            current_index += 1

                            # Convert row to a string without commas and encode to bytes
                            message = ' '.join(row) + '\n'
                            conn.sendall(message.encode('utf-8'))
                            print(f"Data sent: {message.strip()}")

                            # Wait for the next interval
                            time.sleep(DELAY_S)
                        else:
                            print(f"Unexpected request: {request}")

                    except socket.timeout:
                        print("Timeout: No request from client within 60 seconds.")
                        break
                    except Exception as e:
                        print(f"An error occurred: {e}")
                        break

                # Close the connection after the last row is sent or timeout occurs
                print("Closing connection.")
                conn.close()


# Start the server
if __name__ == "__main__":
    start_server()
