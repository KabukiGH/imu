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
#define FF_IS_ACTIVE 1
#define FF_IS_NOT_ACTIVE 0
#define INTERRUPT_MASK 1U 

/*Typedefs*/
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
static volatile unsigned int FF_SetParamsFlag = 0;

/*Local function prototypes*/
static void initSignalHandlers(void);
static void signalHandler(int signal);
static int TCP_initClient(void);
static void TCP_sendPacket(int sockfd);
static void TCP_getPacket(int sockfd, char* pBuffor);
static void parsePacketData(const char *pInputBuf, imu_data_t* pImuData);
static void checkFreeFallIsActive(FF_detection_params_t* pImuConfig, imu_data_t* pImuData);
static void handleParamsConfig(FF_detection_params_t* pImuConfig);
static void error(const char *msg);
static void delay(int milliseconds);

int main()
{
    char inputBuffer[BUFFER_SIZE] = {0};

    imu_data_t imuData = {0};
    FF_detection_params_t imuConfig = {.duration = FF_DURATION_MS, .threshlod = FF_THRESHOLD_G};

    initSignalHandlers();

    int sockfd = TCP_initClient();

    // Main loop - Continuously send request and receive data from server at 50 Hz
    while (1)
    {
        // Send request to server
        TCP_sendPacket(sockfd);

        // Receive data from server
        TCP_getPacket(sockfd,inputBuffer);

        // Copy data from input buffor to struct imu_data
        parsePacketData(inputBuffer, &imuData);

        // Detect if imu data are in set threshold for specified amount of time 
        checkFreeFallIsActive(&imuConfig, &imuData);

        // Interrupt occured - handle in main loop
        handleParamsConfig(&imuConfig);

        // Wait for the next interval
        delay(DELAY_MS);
    }

    close(sockfd);
    return 0;
}


// Local function
static void signalHandler(int signal) {

    if (signal == SIGUSR1) {
        printf("Received SIGUSR1 signal\n"); //debug
        FF_SetParamsFlag |= INTERRUPT_MASK;
    }
    else if(signal == SIGTSTP){
        printf("Received SIGTSTP signal, Press any button to continue\n"); //debug
        FF_SetParamsFlag |= INTERRUPT_MASK;
    }
}


static void initSignalHandlers(void)
{
    // Register signal handlers
    signal(SIGUSR1, signalHandler);
    signal(SIGTSTP, signalHandler);

    printf("Process started. PID: %d\n", getpid());
    // Startup notifcation to inform about possiblities of changing free falling detector parameters threshold level and duration time
    printf("Running...To change the parameters of the FF detector Press Ctrl+Z to send SIGTSTP, or use 'kill -SIGUSR1 %d' to send SIGUSR1\n", getpid());
}

static int TCP_initClient(void)
{
    int sockfd = 0 ;
    struct sockaddr_in server_addr = {0};

    // Create socket
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
    {
        error("Error opening socket");
    }

    // Set up server address structure
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0)
    {
        error("Invalid server IP address");
    }

    // Connect to server
    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        error("Error connecting to server");
    }
    printf("Connected to server at %s:%d\n", SERVER_IP, SERVER_PORT);

    return sockfd;
}

static void TCP_sendPacket(int sockfd)
{
    // Send request to server
    const char request[] = "send\n";
    if (send(sockfd, request, strlen(request), 0) < 0)
    {
        error("Error sending request to server");
    }
}

static void TCP_getPacket(int sockfd, char* pBuffor )
{
    ssize_t bytes_received = 0;
    // Receive data from server
    bytes_received = recv(sockfd, pBuffor, BUFFER_SIZE - 1, 0);
    if (bytes_received > 0)
    {
        pBuffor[bytes_received] = '\0';
        // printf("%s", buffer); // debug
    }
    else if (bytes_received == 0)
    {
        printf("Connection closed by server\n");
        exit(EXIT_SUCCESS);
    }
    else
    {
        error("Error reading from socket");
    }
}

void parsePacketData(const char *pInputBuf, imu_data_t* pImuData) {
    // Use sscanf to parse the string into the three double variables
    sscanf(pInputBuf, "%lf %lf %lf %lf %lf %lf", &pImuData->ax, &pImuData->ay, &pImuData->az, &pImuData->gx, &pImuData->gy, &pImuData->gz);
}

static void checkFreeFallIsActive(FF_detection_params_t* pImuConfig, imu_data_t* pImuData)
{
    static int FF_triggeredThresholdFlag = 0;
    static int FF_calculatedDuration = 0;

    if ((pImuData->ax < pImuConfig->threshlod && pImuData->ax > -pImuConfig->threshlod) &&
        (pImuData->ay < pImuConfig->threshlod && pImuData->ay > -pImuConfig->threshlod) &&
        (pImuData->az < pImuConfig->threshlod && pImuData->az > -pImuConfig->threshlod))
    {
        FF_calculatedDuration += DELAY_MS;

        if(FF_calculatedDuration >= pImuConfig->duration && FF_triggeredThresholdFlag == FF_IS_NOT_ACTIVE)
        {
            printf("Free fall: detected by imcu - Start \n");
            FF_triggeredThresholdFlag = FF_IS_ACTIVE;
        }
    }
    else
    {
        if(FF_triggeredThresholdFlag == FF_IS_ACTIVE)
        {
            printf("Free fall: detected by imcu - End \n");
            FF_triggeredThresholdFlag = FF_IS_NOT_ACTIVE;
        }

        FF_calculatedDuration = 0; // Reset calculated time of free falling 
    }

}

static void handleParamsConfig(FF_detection_params_t* pImuConfig)
{
    if( (FF_SetParamsFlag&INTERRUPT_MASK) == INTERRUPT_MASK)
    {
        int ChangeConfig=0;
        printf("Present free fall detection settings:\n");
        printf("Duration %lf [ms], threshlod = %lf [g]\n",pImuConfig->duration, pImuConfig->threshlod);
        printf("Options: \nPress 1 to change settings\nPress 2 or any other keypad button to keep settings\n");
        scanf("%d", &ChangeConfig);
        
        if(ChangeConfig ==1)
        {
            do
            {
               printf("Type duration time and threshold value:");
            } 
            while (scanf("%lf %lf", &pImuConfig->duration, &pImuConfig->threshlod)!=2);
        }       
        FF_SetParamsFlag &= ~INTERRUPT_MASK;
    }
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