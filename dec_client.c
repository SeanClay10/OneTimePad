// Include necessary standard libraries for input/output, memory management, socket programming, and string manipulation
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
#define HANDSHAKE_MSG "DEC_CLIENT"

// Error handling function that prints error messages to stderr and exits the program
void error(const char *msg) { 
    perror(msg); 
    exit(2); 
} 

// Function to set up the address struct for the server connection
void setupAddressStruct(struct sockaddr_in* address, int portNumber, char* hostname) {
    // Clear the address struct
    memset((char*) address, '\0', sizeof(*address)); 
    // Set the address family to AF_INET for IPv4
    address->sin_family = AF_INET;
    // Convert port number to network byte order and set it
    address->sin_port = htons(portNumber);

    // Get the server host info using the hostname
    struct hostent* hostInfo = gethostbyname(hostname); 
    if (hostInfo == NULL) { 
        // Print error and exit if no such host is found
        fprintf(stderr, "Client: Error, no such host\n"); 
        exit(2); 
    }
    // Copy the host address to the address struct
    memcpy((char*) &address->sin_addr.s_addr, hostInfo->h_addr_list[0], hostInfo->h_length);
}

// Function to read the content of a file into a buffer
void readFileIntoBuffer(const char *filename, char *buffer) {
    // Open the file in read-only mode
    int fd = open(filename, O_RDONLY);
    if (fd < 0) {
        // Print error and exit if file cannot be opened
        fprintf(stderr, "Client: Error opening file %s\n", filename);
        exit(1);
    }
    // Read the file content into the buffer
    ssize_t bytesRead = read(fd, buffer, BUFFER_SIZE - 1);
    if (bytesRead < 0) {
        // Print error and exit if file cannot be read
        fprintf(stderr, "Client: Error reading file %s\n", filename);
        close(fd);
        exit(1);
    }
    // Null-terminate the buffer and close the file
    buffer[bytesRead - 1] = '\0'; // Remove the newline character
    close(fd);
}

// Main function for decryption on the client side
int main(int argc, char *argv[]) {
    int socketFD, portNumber, charsWritten, charsRead;
    struct sockaddr_in serverAddress;
    char buffer[BUFFER_SIZE];
    char keyBuffer[BUFFER_SIZE];
    char ciphertextBuffer[BUFFER_SIZE];

    // Check if the correct number of arguments is provided
    if (argc < 4) { 
        fprintf(stderr,"Using: %s ciphertext key port\n", argv[0]); 
        exit(2); 
    } 

    // Read the ciphertext file into the buffer
    readFileIntoBuffer(argv[1], ciphertextBuffer);
    // Read the key file into the buffer
    readFileIntoBuffer(argv[2], keyBuffer);

    // Check if the key is long enough to encrypt the ciphertext
    if (strlen(keyBuffer) < strlen(ciphertextBuffer)) {
        fprintf(stderr, "Client: Error key is too short\n");
        exit(1);
    } else if (strlen(keyBuffer) > strlen(ciphertextBuffer)) {
        // Truncate the key to match the length of the plaintext
        keyBuffer[strlen(ciphertextBuffer)] = '\0';
    }

    // Create a socket for communication
    socketFD = socket(AF_INET, SOCK_STREAM, 0); 
    if (socketFD < 0){
        error("Client: Error opening socket");
    }

    // Set up the server address struct using the provided port number and hostname
    setupAddressStruct(&serverAddress, atoi(argv[3]), "localhost");

    // Connect to the server
    if (connect(socketFD, (struct sockaddr*)&serverAddress, sizeof(serverAddress)) < 0){
        error("Client: Error connecting");
    }

    // Send handshake message to the server to initiate communication
    charsWritten = send(socketFD, HANDSHAKE_MSG, strlen(HANDSHAKE_MSG), 0);

    // Clear the buffer and receive the server's response
    memset(buffer, '\0', sizeof(buffer));
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
    if (charsRead < 0 || strcmp(buffer, "DEC_SERVER") != 0) {
        fprintf(stderr, "Client: Error communicating with dec_server\n");
        close(socketFD);
        exit(2);
    }

    // Send the ciphertext to the server
    charsWritten = send(socketFD, ciphertextBuffer, strlen(ciphertextBuffer), 0);
    if (charsWritten < 0 || charsWritten < strlen(ciphertextBuffer)) {
        error("Client: Error sending ciphertext");
    }

    // Send the key to the server
    charsWritten = send(socketFD, keyBuffer, strlen(keyBuffer), 0);
    if (charsWritten < 0 || charsWritten < strlen(keyBuffer)) {
        error("Client: Error sending key");
    }

    // Receive the decrypted plaintext from the server
    memset(buffer, '\0', sizeof(buffer));
    charsRead = recv(socketFD, buffer, sizeof(buffer) - 1, 0); 
    if (charsRead < 0) {
        error("Client: Error reading plaintext from socket");
    }

    // Print the decrypted text to stdout
    printf("%s\n", buffer);

    // Close the socket and end the program
    close(socketFD); 
    return 0;
}
