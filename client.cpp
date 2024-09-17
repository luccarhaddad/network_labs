//
// Created by Lucca Haddad on 11/09/24.
//

#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "utils/functions.h"            // Include custom utility functions

#define PORT 8000                       // Client and server must agree on this port
#define BUFFER_SIZE 4096                // Size of buffer for file transfer

// Function to receive and process the server's response
void receive_response(int socket, const char* command) {
    char buffer[BUFFER_SIZE];
    int bytes_received;

    if(strncmp(command, "MyGet ", 6) == 0) {
        // Receive the first line from the server (e.g., "OK\n" or an error message)
        bytes_received = recv_line(socket, buffer, BUFFER_SIZE);
        handle_received_bytes(bytes_received, socket);

        buffer[bytes_received] = '\0';
        printf("%s", buffer);

        if(strncmp(buffer, "OK\n", 3) == 0) {
            // Receive the file size from the server
            bytes_received = recv_line(socket, buffer, BUFFER_SIZE);
            handle_received_bytes(bytes_received, socket);
            
            buffer[bytes_received] = '\0';
            long filesize = atol(buffer);
            //printf("File size: %ld bytes\n", filesize); // Uncomment for debugging

            long total_received = 0;
            // Receive the file data in chunks until the entire file is received
            while(total_received < filesize) {
                bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
                handle_received_bytes(bytes_received, socket);

                // Write the received data to stdout
                fwrite(buffer, 1, bytes_received, stdout);
                total_received += bytes_received;
                //printf("Total received: %ld bytes\n", total_received); // Uncomment for debugging
            }
        }
    } else {
        // For other commands (e.g., "MyLastAccess"), receive a single line from the server
        bytes_received = recv_line(socket, buffer, BUFFER_SIZE);
        handle_received_bytes(bytes_received, socket);
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }
}

int main() {
    // Ignore SIGPIPE signals to prevent crashes when writing to a closed socket
    signal(SIGPIPE, SIG_IGN);

    int sock;
    sockaddr_in server_addr;
    char command[BUFFER_SIZE];

    // Create a TCP socket
    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Unable to create the socket.\n");
        exit(EXIT_FAILURE);
    }

    // Set up the server address structure
    server_addr.sin_family = AF_INET;           // Use IPv4
    server_addr.sin_port = htons(PORT);         // Set the server port number

    // Convert the IP address from text to binary form and store it in server_addr
    if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address.\n");
        exit(EXIT_FAILURE);
    }

    // Connect to the server
    if(connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error while connecting to server.\n");
        exit(EXIT_FAILURE);
    }

    printf("Connecting to server.\n");
    printf("Type 'MyGet <file_path>' or 'MyLastAccess'\n");

    // Main loop to send commands and receive responses
    while(true) {
        printf("> ");
        fgets(command, BUFFER_SIZE, stdin);     // Read command from standard input
        trim_newline(command);                  // Remove any trailing newline characters
        strcat(command, "\n");                  // Append a newline character to the command

        // Check if the user wants to exit
        if(strncmp(command, "exit", 4) == 0){
            break;
        }

        // Send the command to the server
        if(send(sock, command, strlen(command), 0) < 0) {
            perror("Error while sending command.\n");
            break;
        }

        // Receive and process the server's response
        receive_response(sock, command);
    }
    //Close the socket connection
    close(sock);
    return 0;
}
