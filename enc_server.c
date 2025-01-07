// Include necessary standard libraries for input/output, memory management, string manipulation, socket programming, and system calls
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/wait.h>

// Define buffer size for data transmission
#define BUFFER_SIZE 70000
// Define handshake message for server-client communication
#define HANDSHAKE_MSG "ENC_SERVER"

// Error handling function that prints error messages to stderr and exits the program
void error(const char *msg) {
    perror(msg);
    exit(1);
}

// Function to set up the address struct for the server
void setupAddressStruct(struct sockaddr_in* address, int portNumber) {
    // Clear the address struct
    memset((char*) address, '\0', sizeof(*address));
    // Set the address family to AF_INET for IPv4
    address->sin_family = AF_INET;
    // Convert port number to network byte order and set it
    address->sin_port = htons(portNumber);
    // Set the IP address to accept connections from any IP address
    address->sin_addr.s_addr = INADDR_ANY;
}

// Function to encrypt plaintext using a key
void encrypt(char *plaintext, char *key, char *ciphertext) {
    int i, j;
    // Define a string containing bad characters
    char badchar[] = "$*!(#*djs8301these-are-all-bad-characters";
    int shouldEncrypt = 1; // Flag to determine if encryption should proceed

    // Check for bad characters in plaintext
    for (i = 0; i < strlen(plaintext); i++) {
        for (j = 0; j < strlen(badchar); j++) {
            if (plaintext[i] == badchar[j]) {
                shouldEncrypt = 0; // Set flag to false if bad character found
                return; // Exit early if bad character found
            }
        }
    }

    // Proceed with encryption if no bad characters found
    if (shouldEncrypt) {
        for (i = 0; i < strlen(plaintext); i++) {
            if (plaintext[i] == ' ') {
                plaintext[i] = '['; // Treat space as [
            }
            if (key[i] == ' ') {
                key[i] = '[';
            }
            // Convert characters to numerical values for encryption
            int pt = plaintext[i] - 'A';
            int kt = key[i] - 'A';
            int ct = (pt + kt) % 27;
            ciphertext[i] = ct + 'A';
            if (ciphertext[i] == '[') {
                ciphertext[i] = ' '; // Treat [ as space
            }
        }
        ciphertext[i] = '\0';
    } else {
        // If bad character found, set ciphertext to an empty file
        strcpy(ciphertext, "");
    }
}

// Function to handle client connections
void handleClient(int connectionSocket) {
    char buffer[BUFFER_SIZE];
    char plaintext[BUFFER_SIZE];
    char key[BUFFER_SIZE];
    char ciphertext[BUFFER_SIZE];

    // Handshake with client
    memset(buffer, '\0', sizeof(buffer));
    int charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0 || strcmp(buffer, "ENC_CLIENT") != 0) {
        fprintf(stderr, "Server: Error communicating with enc_client\n");
        close(connectionSocket);
        exit(2);
    }

    // Send handshake response
    int charsWritten = send(connectionSocket, HANDSHAKE_MSG, strlen(HANDSHAKE_MSG), 0);

    // Receive plaintext
    memset(plaintext, '\0', sizeof(plaintext));
    charsRead = recv(connectionSocket, plaintext, sizeof(plaintext) - 1, 0);
    if (charsRead < 0) {
        error("Server: Error reading plaintext from socket");
    }

    // Receive key
    memset(key, '\0', sizeof(key));
    charsRead = recv(connectionSocket, key, sizeof(key) - 1, 0);
    if (charsRead < 0) {
        error("Server: Error reading key from socket");
    }

    // Perform encryption
    encrypt(plaintext, key, ciphertext);

    // Send ciphertext back to client
    charsWritten = send(connectionSocket, ciphertext, strlen(ciphertext), 0);
    if (charsWritten < 0) {
        error("Server: Error sending ciphertext");
    }

    // Close the connection socket
    close(connectionSocket);
    exit(0);
}

// Main function to set up the server and handle incoming connections
int main(int argc, char *argv[]) {
    int listenSocket, connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);
    pid_t pid;

    // Check if the correct number of arguments is provided
    if (argc < 2) { 
        fprintf(stderr,"Using: %s port\n", argv[0]); 
        exit(1);
    } 

    // Create a socket
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("Error opening socket");
    }

    // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Bind the socket to the port
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("Error binding");
    }

    // Listen for incoming connections (up to 5 pending connections)
    listen(listenSocket, 5);

    // Handle SIGCHLD to avoid zombie processes
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        error("sigaction");
    }

    // Main loop to accept and handle incoming connections
    while (1) {
        // Accept a new connection
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0) {
            error("Error accepting");
        }

        // Fork a new process to handle the client connection
        pid = fork();
        if (pid < 0) {
            error("Error on fork");
        }
        if (pid == 0) {
            // In the child process: close the listening socket and handle the client
            close(listenSocket);
            handleClient(connectionSocket);
        } else {
            // In the parent process: close the connection socket
            close(connectionSocket);
        }
    }

    // Close the listening socket (not reached due to infinite loop)
    close(listenSocket);
    return 0;
}
