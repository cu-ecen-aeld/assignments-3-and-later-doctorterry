#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <syslog.h>

#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

#define PORT "9000" // Port users will be connecting to
#define BACKLOG 10 // Number of pending connections queue will hold
#define BUFFER_SIZE 10000
#define ADDR_BUFFER_SIZE 255 

// Path to the log file. Note that for the remove() function to work this
// needs to be a const char*. 
const char* LOG_PATH = "/var/tmp/aesdsocketsdata"; 


void error(const char *msg){
    perror(msg);
    exit(-1);
}


// This happens on SIGTERM and SIGINT
void signal_handler(int s)
{
    syslog(LOG_INFO, "Exiting application gracefully.");
    // Delete the aesdsocketdata file
    remove(LOG_PATH);
    exit(0);
}


// 1 p.21
int main(int argc, char *argv[]){
    pid_t pid;
    int status, sockfd, new_fd, output_fd, res_bind, res_listen, log_fd, res_send;
    char buffer[BUFFER_SIZE];
    struct addrinfo hints;
    struct addrinfo *res;
    struct sockaddr_storage their_addr;
    socklen_t addr_size;
    char addr_buffer[ADDR_BUFFER_SIZE];
    ssize_t size_rcv; // Signed value or received message

    // make a socket:
    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0){
        printf("ERROR: Open socket failed\n");
        syslog(LOG_ERR, "ERROR: Open socket failed.");
        exit(-1);
    }
    printf("Socket opened\n");

    memset(&hints, 0, sizeof hints); // make sure the struct is empty
    hints.ai_family = AF_INET; // Set for IPv4
    hints.ai_socktype = SOCK_STREAM; // TCP stream sockets
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE; // Fill in the IP automatically


    if ((status = getaddrinfo(NULL, PORT, &hints, &res)) != 0) {
        printf("getaddrinfo error\n");
        exit(-1);
    }
    printf("Get Address successful\n");



    res_bind = bind(sockfd, res->ai_addr, res->ai_addrlen);
    if(res_bind != 0){
        printf("ERROR: Bind failed\n");
        syslog(LOG_ERR, "bind() failed with Error %d", errno);
        freeaddrinfo(res); // free the linked list
        exit(-1);
    } 

    // Start listen
    res_listen = listen(sockfd, BACKLOG);
    if(res_listen != 0){
        printf("ERROR: Listen failed\n");
        syslog(LOG_ERR, "ERROR: Listen failed\n");
        freeaddrinfo(res);
        exit(-1);
    }

    output_fd = open(LOG_PATH, O_CREAT | O_APPEND | O_RDWR, 0664);
    // if (output_fd == -1){
    if (output_fd < 0){
        printf("ERROR: File open failed\n");
        syslog(LOG_ERR, "ERROR: File open failed\n");
        exit(-1);
    }

    // Refer to LSP p174
    // Fork to a daemon if -d command line argument present
    if (argc > 1) { // Note this line is necessary or test will fail
        if (strcmp(argv[1], "-d") == 0) {
            pid = fork ();
            if (pid == -1){
                exit(-1);
            }
            else if (pid > 0){
                exit(0);
            }
        }
    }

    // Refer to LSP p343 for code
    // Register signal_handler as our signal handler for SIGINT.
    if (signal (SIGINT, signal_handler) == SIG_ERR) {
        printf ("ERROR: Cannot handle SIGINT!\n");
        exit(-1);
    }

    //Register signal_handler as our signal handler for SIGTERM.
    if (signal (SIGTERM, signal_handler) == SIG_ERR) {
        printf ("ERROR: Cannot handle SIGTERM!\n");
        exit(-1);
    }

    freeaddrinfo(res);

    // Refer to LSP chapter 2
    while(1){
        // now accept an incoming connection:
        addr_size = sizeof(their_addr);
        new_fd = accept(sockfd, (struct sockaddr *)&their_addr, &addr_size);

        // Convert binary format address of client to characters and store in addr_buffer
        inet_ntop(AF_INET, &their_addr, addr_buffer, addr_size);

        syslog(LOG_INFO, "Accepted connection\n");
        printf("Accepted connection\n");

        // Loop that reads the message from the client and stores in file
        do {
            // Allocate space in memory for the buffer that is filled with nothing character $
            // to be overwritten. 
            memset(buffer, '$', BUFFER_SIZE);
            size_rcv = recv(new_fd, buffer, BUFFER_SIZE, 0);
            if (size_rcv < 0) {
              printf("ERROR: Receive failed\n");
              syslog(LOG_ERR,"ERROR: Receive failed");
              exit(1);
            }
            // Write to the log file /var/tmp/aesdsocketdata
            write(output_fd, buffer, size_rcv);
            printf("Write success\n");

        }while (size_rcv == sizeof(buffer));

        // Determine the size of the log file
        int file_length = lseek(output_fd, 0, SEEK_END);

        // Set size of buffer based on file_length
        char* output_buffer = malloc(sizeof(char)*file_length);

        // Set position in file
        lseek(output_fd, 0, SEEK_SET);
        int read_len = read(output_fd, output_buffer, file_length);

        // Send message
        int send_len = send(new_fd, output_buffer, file_length, 0);   
        if (send_len < 0)
        {
            printf("ERROR: Send failed\n");
            syslog(LOG_ERR, "ERROR: Send failed");
            return -1;
        }        
        
        // Release memory
        free(output_buffer);
        printf("Send success\n");

    }
    // Deletes file when process done
    remove(LOG_PATH);    
    printf("Successfully completed.\n");

    return 0;

}
