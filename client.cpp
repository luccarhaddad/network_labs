//
// Created by Lucca Haddad on 11/09/24.
//

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 8080                /* Client and server must agree on this port */
#define BUFFER_SIZE 4096                /* Size of buffer for file transfer */

int main(int argc, char **argv) {
    char buffer[BUFFER_SIZE];           /* Buffer for incoming file */
    hostent *server_host{};             /* Holds server info */
    sockaddr_in channel{};              /* Holds server's IP address and port */
    
    /* Check if the program received exactly 2 arguments (server name and file name) */
    if (argc != 3) {
        printf("Usage: client server-name file-name\n");
        exit(-1);
    }

    /* Get the server's IP address from its name */
    server_host = gethostbyname(argv[1]);
    if (!server_host) {
        printf("gethostbyname failed to locate %s\n", argv[1]);
        exit(-1);
    }

    /* Create a TCP socket */
    int client_socket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (client_socket <0) {
        printf("Socket call failed\n");
        exit(-1);
    }

    /* Set up the server's address (IP and port) */
    // Zero out the channel structure
    memset(&channel, 0, sizeof(channel));
    // Use IPv4
    channel.sin_family= AF_INET;
    // Copy server's IP
    memcpy(&channel.sin_addr.s_addr, server_host->h_addr, server_host->h_length);
    // Set the server port
    channel.sin_port= htons(SERVER_PORT);

    /* Attempt to connect to the server */
    int connect_status = connect(client_socket, reinterpret_cast<struct sockaddr *>(&channel), sizeof(channel));
    if (connect_status < 0) {
        printf("Connect failed\n");
        exit(-1);
    }
    // Connection is now established, send the file name to the server

    // Send file name with null terminator
    write(client_socket, argv[2], strlen(argv[2]) + 1);

    // Receive the file content and write it to standard output
    while (true) {
        // Read data from socket
        int bytes = read(client_socket, buffer, BUFFER_SIZE);
        // Exit on EOF or error
        if (bytes <= 0)
            exit(0);
        // Write received data to stdout
        write(1, buffer, bytes);
    }
}
