//
// Created by Lucca Haddad on 11/09/24.
//

#include <pthread.h>            // threads management
#include <netdb.h>
#include <arpa/inet.h>          // inet_ntop and socket structures
#include <netinet/in.h>         // socket structures
#include <sys/fcntl.h>
#include <time.h>               // time manipulation
#include "utils/functions.h"    // include custom utility functions

#define PORT 8000               // arbitrary, but client & server must agree
#define BUFFER_SIZE 4096        // block transfer size

typedef struct ClientInfo {
    char ip_address[INET_ADDRSTRLEN];   // stores IPv4 addresses
    unsigned short port;                // stores the client's port number
    time_t last_access;                 // stores last access timestamp
    ClientInfo *next;                   // pointer to next ClientInfo on linked list
} ClientInfo;

ClientInfo *client_list = NULL;       // global linked list to store clients
pthread_mutex_t client_list_mutex;    // ensures thread-safe access to linked list by multiple threads

void update_client_access(const char *ip_address, unsigned short port) {
    pthread_mutex_lock(&client_list_mutex); // lock the mutex for exclusive access
    ClientInfo *current = client_list;

    // check if client already exists in the list
    while (current != NULL) {
        if (strcmp(current->ip_address, ip_address) == 0 && current->port == port) {
            current->last_access = time(NULL);          // update last access time
            pthread_mutex_unlock(&client_list_mutex);   // unlock the muter
            return;
        }
        current = current->next;
    }

    // if client was not found, appends it to the linked list
    ClientInfo *new_client = (ClientInfo *)malloc(sizeof(ClientInfo));
    strcpy(new_client->ip_address, ip_address);
    new_client->port = port;
    new_client->last_access = time(NULL);
    new_client->next = client_list;
    client_list = new_client;                   // update the head of the list
    pthread_mutex_unlock(&client_list_mutex);   // unlock the mutex
}

time_t get_client_last_access(const char* ip_address, unsigned short port) {
    pthread_mutex_lock(&client_list_mutex);
    ClientInfo* current = client_list;

    // traverse the list to find the client
    while(current != NULL) {
        if(strcmp(current->ip_address, ip_address) == 0 && current->port == port){
            time_t last_access = current->last_access;
            pthread_mutex_unlock(&client_list_mutex);
            return last_access;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&client_list_mutex);
    return (time_t)0; // return 0 if not found
}

void* handle_client(void* arg) {
    int client_socket = *(int*) arg;    // get the client socket descriptor
    free(arg);                          // free the allocated memory for the socket descriptor

    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];
    sockaddr_in addr;
    socklen_t addr_size = sizeof(sockaddr_in);

    // get the client's IP address and port number
    getpeername(client_socket, (sockaddr*)&addr, &addr_size);
    inet_ntop(AF_INET, &addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    unsigned short client_port = ntohs(addr.sin_port);

    while(true) {
        memset(buffer, 0, BUFFER_SIZE);     // clear the buffer
        // receive a line of data from the client
        ssize_t bytes_received = recv_line(client_socket, buffer, BUFFER_SIZE);
        if(bytes_received <= 0) {
            if(bytes_received == 0) {
                printf("Client disconnected: %s:%d\n", client_ip, client_port);
            } else {
                perror("Error receiving data");
            }
            close(client_socket);   // close the client socket
            pthread_exit(NULL);     // exit the thread
        }
        
        trim_newline(buffer);       // remove any trailing newline characters
        printf("Received command from %s:%d '%s'\n", client_ip, client_port, buffer);

        // handle "MyGet" command
        if(strncmp(buffer, "MyGet ", 6) == 0) {
            char* file_path = buffer + 6;                   // extract the file path from the command
            FILE* file = fopen(file_path, "rb");            // open the file in binary read mode
            if(file == NULL) {
                char* msg = "ERROR: File not found.\n";
                send(client_socket, msg, strlen(msg), 0);   // send an error message to the client
            } else {
                char* msg = "OK\n";
                send(client_socket, msg, strlen(msg), 0);   // send an OK message to the client

                // get the size of the file
                fseek(file, 0, SEEK_END);
                long filesize = ftell(file);
                fseek(file, 0, SEEK_SET);

                // send the file size to the client
                sprintf(buffer, "%ld\n", filesize);
                send(client_socket, buffer, strlen(buffer), 0);

                // send the file content to the client in chunks
                size_t n;
                while((n = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                    send(client_socket, buffer, n, 0);
                }
                fclose(file);   // close the file
                update_client_access(client_ip, client_port); // update client's last access time
            }
        }
        // handle the "MyLastAccess" command
        else if(strncmp(buffer, "MyLastAccess", strlen("MyLastAccess")) == 0) {
            time_t last_access = get_client_last_access(client_ip, client_port);
            if(last_access == 0) {
                char* msg = "Last Access = Null\n";
                send(client_socket, msg, strlen(msg), 0);   // send a message indicating no previous access
            } else {
                char msg[BUFFER_SIZE];
                tm* tm_info = localtime(&last_access);
                // format the last access time and send it to the client
                strftime(msg, BUFFER_SIZE, "Last Access=%Y-%m-%d %H:%M:%S\n", tm_info);
                send(client_socket, msg, strlen(msg), 0);
            }
        } 
        // handle invalid commands
        else {
            char* msg = "ERROR: Invalid command.\n";
            send(client_socket, msg, strlen(msg), 0);
        }
    }
    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    // Ignore SIGPIPE signals to prevent crashes when writing to a closed socket
    signal(SIGPIPE, SIG_IGN);

    int server_socket;
    int* new_sock;
    sockaddr_in server_addr, cleint_addr;
    pthread_t thread_id;
    socklen_t client_size = sizeof(sockaddr_in);

    // initialize the mutex for client list access
    pthread_mutex_init(&client_list_mutex, NULL);

    // Cceate the socket
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Unable to create the socket.\n");
        exit(EXIT_FAILURE);
    }

    // prepare the sockaddr_in structure
    server_addr.sin_family = AF_INET;           // use IPv4
    server_addr.sin_addr.s_addr = INADDR_ANY;   // accept connections from any IP address
    server_addr.sin_port = htons(PORT);         // set the port number
    
    // bind the socket to the specified IP and port
    if(bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Unable to bind\n");
        exit(EXIT_FAILURE);
    }

    // listen for incoming connections
    listen(server_socket, 5);
    printf("Server listening on port %d\n", PORT);

    // accept incoming connections in a loop
    while((new_sock = (int*)malloc(sizeof(int)), *new_sock = accept(server_socket, (sockaddr*)&cleint_addr, &client_size)) >= 0) {
        // create a new thread to handle the client
        if(pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) != 0) {
            perror("Unable to create a new thread.\n");
            free(new_sock);             // free the allocated memory on failure
        } else {
            pthread_detach(thread_id);  // detach the thread to reclaim resources when it finishes
        }
    }

    // check if accept() failed
    if(*new_sock < 0) {
        perror("Error while accepting connection.\n");
        free(new_sock);
        exit(EXIT_FAILURE);
    }

    close(server_socket);   // close the server socket
    pthread_mutex_destroy(&client_list_mutex);  // destroy the mutex
    return 0;
}