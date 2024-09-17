//
// Created by Lucca Haddad on 11/09/24.
//

#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include "utils/functions.h"

#define PORT 8000                       /* Client and server must agree on this port */
#define BUFFER_SIZE 4096                /* Size of buffer for file transfer */

void receive_response(int socket, const char* command) {
    char buffer[BUFFER_SIZE];
    int bytes_received;

    if(strncmp(command, "MyGet ", 6) == 0) {
        bytes_received = recv_line(socket, buffer, BUFFER_SIZE);
        handle_received_bytes(bytes_received, socket);

        buffer[bytes_received] = '\0';
        printf("%s", buffer);

        if(strncmp(buffer, "OK\n", 3) == 0) {
            bytes_received = recv_line(socket, buffer, BUFFER_SIZE);
            handle_received_bytes(bytes_received, socket);
            
            buffer[bytes_received] = '\0';
            long filesize = atol(buffer);
            //printf("File size: %ld bytes\n", filesize);

            long total_received = 0;
            while(total_received < filesize) {
                bytes_received = recv(socket, buffer, BUFFER_SIZE, 0);
                handle_received_bytes(bytes_received, socket);
                fwrite(buffer, 1, bytes_received, stdout);
                total_received += bytes_received;
                //printf("Total received: %ld bytes\n", total_received);
            }
        }
    } else {
        bytes_received = recv_line(socket, buffer, BUFFER_SIZE);
        handle_received_bytes(bytes_received, socket);
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
    }
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    int sock;
    sockaddr_in server_addr;
    char command[BUFFER_SIZE];

    if((sock = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Unable to create the socket.\n");
        exit(EXIT_FAILURE);
    }

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if(inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr) <= 0) {
        perror("Invalid address.\n");
        exit(EXIT_FAILURE);
    }

    if(connect(sock, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Error while connecting to server.\n");
        exit(EXIT_FAILURE);
    }

    printf("Connecting to server.\n");
    printf("Type 'MyGet <file_path>' or 'MyLastAccess'\n");

    while(true) {
        printf("> ");
        fgets(command, BUFFER_SIZE, stdin);
        trim_newline(command);
        strcat(command, "\n");

        if(strncmp(command, "exit", 4) == 0){
            break;
        }

        if(send(sock, command, strlen(command), 0) < 0) {
            perror("Error while sending command.\n");
            break;
        }

        receive_response(sock, command);
    }
    close(sock);
    return 0;
}
