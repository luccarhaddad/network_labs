//
// Created by Lucca Haddad on 11/09/24.
//

#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/fcntl.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

#define SERVER_PORT 8080  /* arbitrary, but client & server must agree*/
#define BUFFER_SIZE 4096  /* block transfer size */
#define QUEUE_SIZE 10

int setup_server_socket() {
    int on = 1;
    sockaddr_in channel{};  // Holds the IP address and port information

    // Initialize the channel structure with zeros
    memset(&channel, 0, sizeof(channel));

    // Specifies that we're using the IPv4 protocol
    channel.sin_family = AF_INET;

    // Allows the socket to listen on all available network interfaces
    channel.sin_addr.s_addr = htonl(INADDR_ANY);

    // Sets the port number (converted to network byte order)
    channel.sin_port = htons(SERVER_PORT);

    // Create the TCP socket using the AF_INET (IPv4), SOCK_STREAM, and IPPROTO_TCP protocol
    int server_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
    if (server_socket < 0) {
        printf("Socket creation failed\n");
        exit(-1);
    }

    // Set the SO_REUSEADDR option to avoid "address already in use" errors on restart
    setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));

    // Bind the socket to the address and port specified in the channel structure
    int bind_status = bind(server_socket, reinterpret_cast<struct sockaddr *>(&channel), sizeof(channel));
    if (bind_status < 0) {
        printf("Bind failed\n");
        exit(-1);
    }

    // Set the socket to listen for incoming client connections, with the specified queue size
    int listen_status = listen(server_socket, QUEUE_SIZE);
    if (listen_status < 0) {
        printf("Listen failed\n");
        exit(-1);
    }

    return server_socket;
}

void handle_client(int client_socket) {
    char buffer[BUFFER_SIZE];
    int bytes_read;

    // Reads file name from socket
    read(client_socket, buffer, BUFFER_SIZE);

    // Opens the file to be sent back
    int file_descriptor = open(buffer, O_RDONLY);
    if (file_descriptor < 0) {
        printf("Failed to open the file\n");
        close(file_descriptor);
        return;
    }

    // Reads from file
    while((bytes_read = read(file_descriptor, buffer, BUFFER_SIZE)) > 0) {
        // Write bytes to socket
        write(client_socket, buffer, bytes_read);
    }

    // Closes file and connection
    close(file_descriptor);
    close(client_socket);
}

int main(int argc, char *argv[]) {
    int server_socket = setup_server_socket();

    while (true) {
        int client_socket = accept(server_socket, NULL, NULL);
        if (client_socket < 0) {
            printf("Accept connection failed\n");
            exit(-1);
        }
        handle_client(client_socket);
    }
    return 0;
}
