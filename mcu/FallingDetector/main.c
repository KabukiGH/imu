#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include <signal.h>

#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 65432
#define BUFFER_SIZE 1024
#define FREQUENCY_HZ 50
#define DELAY_MS (1000 / FREQUENCY_HZ) // 20ms
#define FF_DURATION_MS 100 // Default value 100ms
#define FF_THRESHOLD_G 0.1 // Default value 0.1g for ax, ay, az

typedef struct
{
    double ax;
    double ay;
    double az;
    double gx;
    double gy;
    double gz;
}imu_data_t;

typedef struct
{
    double duration;
    double threshlod;
}FF_detection_params_t;

/*Local variable*/
static volatile unsigned short FF_SetParamsFlag = 0;

// Signal handler function
void signalHandler(int signal) {

    if (signal == SIGUSR1) {
        printf("Received SIGUSR1 signal\n"); //debug
        FF_SetParamsFlag = 1;
    }
    else if(signal == SIGTSTP){
        printf("Received SIGTSTP signal, Press any button to continue\n"); //debug
        FF_SetParamsFlag = 1;
    }
}

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

    // Register signal handlers
    signal(SIGUSR1, signalHandler);
    signal(SIGTSTP, signalHandler);

    printf("Process started. PID: %d\n", getpid());

    // Infinite loop to keep the process running
    printf("Running...To change the parameters of the FF detector Press Ctrl+Z to send SIGTSTP, or use 'kill -SIGUSR1 %d' to send SIGUSR1\n", getpid());

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
    imu_data_t ImuData = {0};
    FF_detection_params_t ImuConfig = {.duration = FF_DURATION_MS, .threshlod=FF_THRESHOLD_G};
    int FF_start_sampling = 0;
    int FreeFallActiveFlag = 0;

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
            // printf("%s", buffer); // debug
        } else if (bytes_received == 0) {
            printf("Connection closed by server\n");
            break;
        } else {
            error("Error reading from socket");
        }

        // Copy data from input buffor to struct imu_data
        parseString(buffer, &ImuData);
        
        //TODO swipe to a function
        if ((ImuData.ax < ImuConfig.threshlod && ImuData.ax > -ImuConfig.threshlod) &&
            (ImuData.ay < ImuConfig.threshlod && ImuData.ay > -ImuConfig.threshlod) &&
            (ImuData.az < ImuConfig.threshlod && ImuData.az > -ImuConfig.threshlod))
        {
            FF_start_sampling += DELAY_MS;

            if(FF_start_sampling >= ImuConfig.duration && FreeFallActiveFlag == 0)
            {
                printf("Free fall: detected by imcu - Start \n");
                FreeFallActiveFlag = 1;
            }
        }
        else
        {
            if(FreeFallActiveFlag == 1)
            {
                printf("Free fall: detected by imcu - End \n");
                FreeFallActiveFlag = 0;
            }
            FF_start_sampling = 0;
        }

        // Interrupt occured - TODO swipe to function that handle params change
        if(FF_SetParamsFlag == 1)
        {
            int ChangeConfig=0;
            printf("Present free fall detection settings:\n");
            printf("Duration %lf [ms], threshlod = %lf [g]\n",ImuConfig.duration, ImuConfig.threshlod);
            printf("Options: \nPress 1 to change settings\nPress 2 or any other keypad button to keep settings\n");
            scanf("%d", &ChangeConfig);

            if(ChangeConfig ==1)
            {
                do {
                   printf("Type duration time and threshold value:");
                } while (scanf("%lf %lf", &ImuConfig.duration, &ImuConfig.threshlod)!=2);
            }

            FF_SetParamsFlag = 0;
        }

        // Wait for the next interval
        delay(DELAY_MS);
    }

    close(sockfd);
    return 0;
}
