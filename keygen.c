// Include libraries
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>    

// Buffer constant
#define BUF_SIZE 70000

int main(int argc, char *argv[]) {
    // Check if the number of arguments is exactly 2 (program name + key length)
    if (argc != 2) {
        fprintf(stderr, "Error: invalid argument length\n");
        return 1; // Return an error code
    }

    // Convert the second argument to an integer to get the key length
    int keylength = atoi(argv[1]);
    // Check if the key length is a positive integer and less than or equal to BUF_SIZE
    if (keylength <= 0 || keylength > BUF_SIZE) {
        fprintf(stderr, "Error: keylength must be a positive integer\n");
        return 1; // Return an error code
    }

    // Define the allowed characters for the key, which are A-Z and space
    char allowed_chars[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ ";

    // Declare a buffer to hold the generated key
    char key[BUF_SIZE + 1];
    // Initialize the key buffer to all null characters
    memset(key, '\0', sizeof(key));

    // Seed the random number generator with the current time
    srand(time(NULL));

    // Generate the key by randomly selecting characters from the allowed set
    for (int i = 0; i < keylength; i++) {
        // Generate a random index between 0 and 26 (inclusive)
        int random_index = rand() % 27;
        // If the index is 26, use the space character
        if (random_index == 26) {
            key[i] = ' ';
        } else {
            // Otherwise, use a character from A-Z
            key[i] = 'A' + random_index;
        }
        // Print the randomly selected character
        printf("%c", allowed_chars[random_index]);
    }

    // Output a newline at the end of the key
    printf("\n");

    return 0; // Return success code
}
