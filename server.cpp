//
// Created by Lucca Haddad on 11/09/24.
//

#include <pthread.h>    // threads management
#include <netdb.h>
#include <arpa/inet.h>  // inet_ntop and socket structures
#include <netinet/in.h> // socket structures
#include <sys/fcntl.h>
#include <time.h>       // time manipulation
#include "utils/functions.h"

#define PORT 8000              // arbitrary, but client & server must agree
#define BUFFER_SIZE 4096              // block transfer size

typedef struct ClientInfo {
    char ip_address[INET_ADDRSTRLEN]; // stores IPv4 addresses
    unsigned short port;
    time_t last_access;               // stores last access timestamp
    ClientInfo *next;                 // pointer to next ClientInfo on linked list
} ClientInfo;

ClientInfo *client_list = NULL;       // global linked list to store clients
pthread_mutex_t client_list_mutex;    // ensures safe access to linked list by multiple threads

void update_client_access(const char *ip_address, unsigned short port) {
    pthread_mutex_lock(&client_list_mutex); // ensures unique thread access
    ClientInfo *current = client_list;

    // Check if client already exists in the list
    while (current != NULL) {
        if (strcmp(current->ip_address, ip_address) == 0 && current->port == port) {
            current->last_access = time(NULL);
            pthread_mutex_unlock(&client_list_mutex);
            return;
        }
        current = current->next;
    }

    // If client was not found, appends it to the linked list
    ClientInfo *new_client = (ClientInfo *)malloc(sizeof(ClientInfo));
    strcpy(new_client->ip_address, ip_address);
    new_client->port = port;
    new_client->last_access = time(NULL);
    new_client->next = client_list;
    client_list = new_client;
    pthread_mutex_unlock(&client_list_mutex);
}

time_t get_client_last_access(const char* ip_address, unsigned short port) {
    pthread_mutex_lock(&client_list_mutex);
    ClientInfo* current = client_list;

    while(current != NULL) {
        if(strcmp(current->ip_address, ip_address) == 0 && current->port == port){
            time_t last_access = current->last_access;
            pthread_mutex_unlock(&client_list_mutex);
            return last_access;
        }
        current = current->next;
    }
    pthread_mutex_unlock(&client_list_mutex);
    return (time_t)0; // not found
}

void* handle_client(void* arg) {
    int client_socket = *(int*) arg;
    free(arg);

    char buffer[BUFFER_SIZE];
    char client_ip[INET_ADDRSTRLEN];
    sockaddr_in addr;
    socklen_t addr_size = sizeof(sockaddr_in);
    getpeername(client_socket, (sockaddr*)&addr, &addr_size);
    inet_ntop(AF_INET, &addr.sin_addr, client_ip, INET_ADDRSTRLEN);
    unsigned short client_port = ntohs(addr.sin_port);

    while(true) {
        memset(buffer, 0, BUFFER_SIZE);
        ssize_t bytes_received = recv_line(client_socket, buffer, BUFFER_SIZE);
        if(bytes_received <= 0) {
            if(bytes_received == 0) {
                printf("Client disconnected: %s:%d\n", client_ip, client_port);
            } else {
                perror("Error receiving data");
            }
            close(client_socket);
            pthread_exit(NULL);
        }
        
        trim_newline(buffer);
        printf("Received command from %s:%d '%s'\n", client_ip, client_port, buffer);

        if(strncmp(buffer, "MyGet ", 6) == 0) {
            char* file_path = buffer + 6;
            FILE* file = fopen(file_path, "rb");
            if(file == NULL) {
                char* msg = "ERROR: File not found.\n";
                send(client_socket, msg, strlen(msg), 0);
            } else {
                char* msg = "OK\n";
                send(client_socket, msg, strlen(msg), 0);

                fseek(file, 0, SEEK_END);
                long filesize = ftell(file);
                fseek(file, 0, SEEK_SET);

                sprintf(buffer, "%ld\n", filesize);
                send(client_socket, buffer, strlen(buffer), 0);

                size_t n;
                while((n = fread(buffer, 1, BUFFER_SIZE, file)) > 0) {
                    send(client_socket, buffer, n, 0);
                }
                fclose(file);
                update_client_access(client_ip, client_port);
            }
        }
        else if(strncmp(buffer, "MyLastAccess", strlen("MyLastAccess")) == 0) {
            time_t last_access = get_client_last_access(client_ip, client_port);
            if(last_access == 0) {
                char* msg = "Last Access = Null\n";
                send(client_socket, msg, strlen(msg), 0);
            } else {
                char msg[BUFFER_SIZE];
                tm* tm_info = localtime(&last_access);
                strftime(msg, BUFFER_SIZE, "Last Access=%Y-%m-%d %H:%M:%S\n", tm_info);
                send(client_socket, msg, strlen(msg), 0);
            }
        } else {
            char* msg = "ERROR: Invalid command.\n";
            send(client_socket, msg, strlen(msg), 0);
        }
    }
    close(client_socket);
    pthread_exit(NULL);
}

int main() {
    signal(SIGPIPE, SIG_IGN);

    int server_socket;
    int* new_sock;
    sockaddr_in server_addr, cleint_addr;
    pthread_t thread_id;
    socklen_t client_size = sizeof(sockaddr_in);

    pthread_mutex_init(&client_list_mutex, NULL);

    // Creates the socket
    if((server_socket = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Unable to create the socket.\n");
        exit(EXIT_FAILURE);
    }

    // Prepares the sockaddr_in structure
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(PORT);
    
    // Connect the socket
    if(bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("Unable to bind\n");
        exit(EXIT_FAILURE);
    }

    // Listen for connections
    listen(server_socket, 5);
    printf("Server listening on port %d\n", PORT);

    while((new_sock = (int*)malloc(sizeof(int)), *new_sock = accept(server_socket, (sockaddr*)&cleint_addr, &client_size)) >= 0) {
        if(pthread_create(&thread_id, NULL, handle_client, (void*)new_sock) != 0) {
            perror("Unable to create a new thread.\n");
            free(new_sock);
        } else {
            pthread_detach(thread_id);
        }
    }

    if(*new_sock < 0) {
        perror("Error while accepting connection.\n");
        free(new_sock);
        exit(EXIT_FAILURE);
    }

    close(server_socket);
    pthread_mutex_destroy(&client_list_mutex);
    return 0;
}