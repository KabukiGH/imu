#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 65432
#define BUFFER_SIZE 1024
#define FREQUENCY_HZ 50
#define DELAY_MS (1000 / FREQUENCY_HZ)

typedef struct
{
    double ax;
    double ay;
    double az;
    double gx;
    double gy;
    double gz;
}imu_data_t;

void parseString(const char *pInputBuf, imu_data_t* pImuData) {
    // Use sscanf to parse the string into the three double variables
    sscanf(pInputBuf, "%lf %lf %lf %lf %lf %lf", &pImuData->ax, &pImuData->ay, &pImuData->az, &pImuData->gx, &pImuData->gy, &pImuData->gz);
}

void error(const char *msg) {
    perror(msg);
    exit(EXIT_FAILURE);
}

void delay(int milliseconds) {
    struct timespec ts;
    ts.tv_sec = milliseconds / 1000;
    ts.tv_nsec = (milliseconds % 1000) * 1000000;
    nanosleep(&ts, NULL);
}

int main() {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    ssize_t bytes_received;

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        error("Error opening socket");
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        error("Invalid server IP address");
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        error("Error connecting to server");
    }
    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    // Continuously send request and receive data from server at 50 Hz
    imu_data_t imu_data = {0};

    while (1) {
        // Send request to server
        char request[] = "send\n";
        if (send(sockfd, request, strlen(request), 0) < 0) {
            error("Error sending request to server");
        }

        // Receive data from server
        bytes_received = recv(sockfd, buffer, BUFFER_SIZE - 1, 0);
        if (bytes_received > 0) {
            buffer[bytes_received] = '\0';
            printf("%s", buffer);
        } else if (bytes_received == 0) {
            printf("Connection closed by server\n");
            break;
        } else {
            error("Error reading from socket");
        }

        // Copy data from input buffor to struct imu_data
        parseString(buffer,&imu_data);

        if(MAIN_imu_data.ax == -0.8137455366679891)
        {
            printf("Get correct value\n");
        }
        else
        {
            printf("Fail incorrect value\n");
        }

        // Wait for the next interval
        delay(DELAY_MS);
    }

    close(sockfd);
    return 0;
}
