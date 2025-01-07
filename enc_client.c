// Include standard libraries for input/output, memory management, socket programming, and string manipulation
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/stat.h>

// Define buffer size for data transmission
#define BUFFER_SIZE 70000
// Define handshake message for client-server communication
#define HANDSHAKE_MSG "ENC_CLIENT"

// Error handling function that prints error messages to stderr and exits the program
void error(const char *msg) { 
    perror(msg); 
    exit(2); 
} 

// Function to set up the address struct for the server
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname) {
    // Clear the address struct
    memset((char*) address, '\0', sizeof(*address)); 
    // Set the address family to AF_INET for IPv4
    address->sin_family = AF_INET;
    // Convert port number to network byte order and set it
    address->sin_port = htons(portNumber);

    // Get host information based on the hostname
    struct hostent* hostInfo = gethostbyname(hostname); 
    if (hostInfo == NULL) { 
        fprintf(stderr, "Client: Error, no such host\n"); 
        exit(2); 
    }
    // Copy the host address to the address struct
    memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

// Function to read file content into a buffer
void readFileIntoBuffer(const char *filename, char *buffer) {
    // Open the file for reading
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "Client: Error opening file %s\n", filename);
        exit(1);
    }
    // Read the file content into the buffer
    ssize_t bytesRead = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytesRead < 0) {
        fprintf(stderr, "Client: Error reading file %s\n", filename);
        close(fd);
        exit(1);
    }
    // Remove the newline character at the end of the buffer
    buffer[bytesRead - 1] = '\0';
    // Close the file descriptor
    close(fd);
}

// Function to check for invalid characters in the plaintext
int containsInvalidCharacters(char *plaintext) {
    char badchar[] = "$*!(#*djs8301these-are-all-bad-characters";
    for (int i = 0; i < strlen(plaintext); i++) {
        for (int j = 0; j < strlen(badchar); j++) {
            if (plaintext[i] == badchar[j]) {
                return 1;
            }
        }
    }
    return 0;
}

int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE];
    char keyBuffer[BUFFER_SIZE];
    char plaintextBuffer[BUFFER_SIZE];

    // Check if the correct number of arguments is provided
    if (argc < 4) { 
        fprintf(stderr,"Using: %s plaintext key port\n", argv[0]); 
        exit(2); 
    } 

    // Read the plaintext file into a buffer
    readFileIntoBuffer(argv[1], plaintextBuffer);
    // Read the key file into a buffer
    readFileIntoBuffer(argv[2], keyBuffer);

    // Check if the key is long enough
    if (strlen(keyBuffer) < strlen(plaintextBuffer)) {
        fprintf(stderr, "Client: Error, key is too short for encryption\n");
        exit(1);
    } else if (strlen(keyBuffer) > strlen(plaintextBuffer)) {
        // Truncate the key to match the length of the plaintext
        keyBuffer[strlen(plaintextBuffer)] = '\0';
    }

    // Check for invalid characters in plaintext
    if (containsInvalidCharacters(plaintextBuffer)) {
        fprintf(stderr, "\nError, %s contains invalid characters\n\n", argv[1]);
        exit(1);
    }

    // Create a socket
    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD < 0){
        error("Client: Error opening socket");
    }

    // Set up the server address struct
    setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

    // Connect to server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        error("Client: Error connecting");
    }

    // Handshake with server
    charsWritten = send(socketFD, HANDSHAKE_MSG, strlen(HANDSHAKE_MSG), 0);

    // Clear the buffer
    memset(buffer, '\0', sizeof(buffer));
    // Receive server response to the handshake
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
    if (charsRead < 0 || strcmp(buffer, "ENC_SERVER") != 0) {
        fprintf(stderr, "Client: Error not communicating with enc_server\n");
        close(socketFD);
        exit(2);
    }

    // Send plaintext to server
    charsWritten = send(socketFD, plaintextBuffer, strlen(plaintextBuffer), 0);
    if (charsWritten < 0 || charsWritten < strlen(plaintextBuffer)) {
        error("Client: Error sending plaintext");
    }

    // Send key to server
    charsWritten = send(socketFD, keyBuffer, strlen(keyBuffer), 0);
    if (charsWritten < 0 || charsWritten < strlen(keyBuffer)) {
        error("Client: Error sending key");
    }

    // Get ciphertext from server
    memset(buffer, '\0', sizeof(buffer));
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
    if (charsRead < 0) {
        error("Client: Error reading ciphertext from socket");
    }

    // Print the received ciphertext
    printf("%s\n", buffer);

    // Close the socket
    close(socketFD); 
    return 0;
}
