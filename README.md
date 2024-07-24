Manual for Building and Launching server.py and client.c
Prerequisites
Python 3: Ensure you have Python 3.10 installed on your system.
C Compiler: Ensure you have a C compiler installed (GCC).

Directory Structure
Ensure you have the following files in the specified directories:
imu/
├── imu/
│   ├── server.py
│   └── file.csv
└── mcu/
    └── FallingDetector/
        └── client.c

Steps for Building and Launching
1. Clone repository
git clone https://github.com/KabukiGH/imu.git

2. Ensure you have a imu.csv in the same directory as main.py
   
4. Run the Python Server:
python3 main.py
This will start the server, which will listen for incoming connections.

5.Compile the Client Code:
Use your C compiler to compile client.c. For example, using GCC:
gcc -o client main.c

6. Run the Client:
./client.exe
This will start the client, which will connect to the server and begin receiving data at the specified frequency (50 Hz).

Detailed Explanation of the Process
Server (main.py)
The server reads the rows from file.csv and stores them in file_data.
It then creates a TCP/IP socket and listens for incoming connections.
Once a client connects, it enters a loop where it waits for the client to send a request.
Upon receiving the request, it sends a row of data from file_data, waits for the next interval, and then repeats until all data is sent.
After sending all data, it sends an empty message to indicate the end of the transmission and closes the connection.

Client (client.c)
The client creates a TCP/IP socket and connects to the server.
It enters a loop where it sends a request to the server and waits to receive data.
Upon receiving data, it prints it to the console (used for debbuging)
If the received data length is zero, it prints a message indicating the connection was closed by the server and exits the loop.
The client maintains a frequency of 50 Hz between requests by using a delay function.

Additional Notes
Ensure that your network allows the chosen port (e.g., 65432) for communication.
The server script has a timeout of 60 seconds for receiving requests. Ensure that your client sends requests within this timeframe to avoid connection termination.
If you encounter any issues, check that the file paths are correct and that the server and client are running on the same network.

To ensure that the free-fall detector parameters (FF Duration and FF Threshold) can be changed while the proccess is running without recompiling the programme, 
inter-process communication in the form of the signal.h library was used.
1. Raising signals from another the terminal.
kill -SIGUSR1 <pid>
2. Raised signal via keyboard press (same terminal):
SIGTSTP: Ctrl+Z
