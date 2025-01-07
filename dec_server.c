// Include standard libraries for input/output, memory management, socket programming, and string manipulation
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
// Define handshake message for client-server communication
#define HANDSHAKE_MSG "DEC_SERVER"

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
    // Allow connections from any IP address
    address->sin_addr.s_addr = INADDR_ANY;
}

// Function to decrypt the ciphertext using the key
void decrypt(char *ciphertext, char *key, char *plaintext) {
    int i;
    // Loop through each character in the ciphertext
    for (i = 0; i < strlen(ciphertext); i++) {
        // Treat spaces as '[' for both ciphertext and key
        if (ciphertext[i] == ' ') {
            ciphertext[i] = '[';
        }
        if (key[i] == ' ') {
            key[i] = '[';
        }
        // Perform the decryption calculation
        int ct = ciphertext[i] - 'A';
        int kt = key[i] - 'A';
        int pt = (ct - kt + 27) % 27;
        plaintext[i] = pt + 'A';
        // Treat '[' as space for the plaintext
        if (plaintext[i] == '[') {
            plaintext[i] = ' ';
        }
    }
    // Null-terminate the plaintext string
    plaintext[i] = '\0';
}

// Function to handle communication with a client
void handleClient(int connectionSocket) {
    char buffer[BUFFER_SIZE];
    char ciphertext[BUFFER_SIZE];
    char key[BUFFER_SIZE];
    char plaintext[BUFFER_SIZE];

    // Handshake with client
    memset(buffer, '\0', sizeof(buffer));
    int charsRead = recv(connectionSocket, buffer, sizeof(buffer) - 1, 0);
    if (charsRead < 0 || strcmp(buffer, "DEC_CLIENT") != 0) {
        fprintf(stderr, "Server: Error communicating with dec_client\n");
        close(connectionSocket);
        exit(2);
    }

    // Send handshake response to client
    int charsWritten = send(connectionSocket, HANDSHAKE_MSG, strlen(HANDSHAKE_MSG), 0);

    // Receive ciphertext from client
    memset(ciphertext, '\0', sizeof(ciphertext));
    charsRead = recv(connectionSocket, ciphertext, sizeof(ciphertext) - 1, 0);
    if (charsRead < 0) {
        error("Server: Error reading ciphertext from socket");
    }

    // Receive key from client
    memset(key, '\0', sizeof(key));
    charsRead = recv(connectionSocket, key, sizeof(key) - 1, 0);
    if (charsRead < 0) {
        error("Server: Error reading key from socket");
    }

    // Perform decryption
    decrypt(ciphertext, key, plaintext);

    // Send decrypted plaintext back to client
    charsWritten = send(connectionSocket, plaintext, strlen(plaintext), 0);
    if (charsWritten < 0) {
        error("Server: Error sending plaintext");
    }

    // Close the connection socket
    close(connectionSocket);
    exit(0);
}

int main(int argc, char *argv[]) {
    int listenSocket, connectionSocket;
    struct sockaddr_in serverAddress, clientAddress;
    socklen_t sizeOfClientInfo = sizeof(clientAddress);
    pid_t pid;

    // Check if the port number is provided
    if (argc < 2) {
        fprintf(stderr,"Using: %s port\n", argv[0]);
        exit(1);
    }

    // Create a socket for listening to incoming connections
    listenSocket = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSocket < 0) {
        error("Error opening socket");
    }

    // Set up the server address struct using the provided port number
    setupAddressStruct(&serverAddress, atoi(argv[1]));

    // Bind the socket to the address and port number
    if (bind(listenSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        error("Error binding");
    }

    // Start listening for incoming connections, with a backlog of 5
    listen(listenSocket, 5);

    // Handle SIGCHLD to avoid zombie processes
    struct sigaction sa;
    sa.sa_handler = SIG_IGN;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if (sigaction(SIGCHLD, &sa, NULL) == -1) {
        error("sigaction");
    }

    // Infinite loop to accept and handle incoming connections
    while (1) {
        // Accept a new connection
        connectionSocket = accept(listenSocket, (struct sockaddr *)&clientAddress, &sizeOfClientInfo);
        if (connectionSocket < 0) {
            error("Error accepting connection");
        }

        // Fork a new process to handle the client
        pid = fork();
        if (pid < 0) {
            error("Error on fork");
        }
        if (pid == 0) {
            // In the child process, close the listening socket and handle the client
            close(listenSocket);
            handleClient(connectionSocket);
        } else {
            // In the parent process, close the connection socket and continue accepting new connections
            close(connectionSocket);
        }
    }

    // Close the listening socket
    close(listenSocket);
    return 0;
}
