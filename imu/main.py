import csv
import socket

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

# Displaying the data
for index, row in enumerate(file_data):
    print(f"Row {index + 1}: {row}")

# Function to start the TCP/IP server
def start_server(host='127.0.0.1', port=65432):
    # Create a TCP/IP socket
    with socket.socket(socket.AF_INET, socket.SOCK_STREAM) as server_socket:
        server_socket.bind((host, port))
        server_socket.listen()

        print(f"Server listening on {host}:{port}")

        # Wait for a connection
        conn, addr = server_socket.accept()
        with conn:
            print(f"Connected by {addr}")

            try:
                # Send data to the client
                for row in file_data:
                    # Convert row to a string and encode to bytes
                    message = ','.join(row) + '\n'
                    conn.sendall(message.encode('utf-8'))
                print("Data sent successfully.")
            except Exception as e:
                print(f"An error occurred while sending data: {e}")

# Start the server
if __name__ == "__main__":
    start_server()