//
// Created by Lucca Haddad on 11/09/24.
//

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>

#define PORT 8080                       /* Client and server must agree on this port */
#define BUFFER_SIZE 4096                /* Size of buffer for file transfer */

void receive_response(int socket) {
    char buffer[BUFFER_SIZE];
    int bytes_received;
    while((bytes_received = recv(socket, buffer, BUFFER_SIZE - 1, 0)) > 0) {
        buffer[bytes_received] = '\0';
        printf("%s", buffer);
        if(bytes_received < BUFFER_SIZE - 1) {
            break;
        }
    }
}

int main() {
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
        command[strcspn(command, "\n")] = '\0';

        if(send(sock, command, strlen(command), 0) < 0) {
            perror("Error while sending command.\n");
            break;
        }

        receive_response(sock);

        if(strncmp(command, "exit", 4) == 0){
            break;
        }
    }
    close(sock);
    return 0;
}
