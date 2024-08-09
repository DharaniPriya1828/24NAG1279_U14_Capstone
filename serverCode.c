#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>

#define PORT 8080
#define MAX_CLIENTS 10
#define ITEM_COUNT 3

typedef struct {
    char name[256];
    int highestBid;
    char highestBidder[256];
} Item;

Item items[ITEM_COUNT];

pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;
int client_id = 1;  // Initialize the client ID counter

typedef struct {
    int socket;
    int id;
} ClientData;

void* handle_client(void* arg) {
    ClientData *client_data = (ClientData*)arg;
    int client_sock = client_data->socket;
    int client_id = client_data->id;
    free(arg);
    char buffer[1024];
    int bytes_read;

    while ((bytes_read = read(client_sock, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[bytes_read] = '\0';
        printf("Received from client %d: %s\n", client_id, buffer);

        pthread_mutex_lock(&lock);

        char* command = strtok(buffer, " ");
        if (strcmp(command, "status") == 0) {
            char response[1024] = "\nItem status:\n";
            for (int i = 0; i < ITEM_COUNT; i++) {
                char item_status[600];
                snprintf(item_status, sizeof(item_status),
                    "Item: %s, Highest Bid: %d, Highest Bidder: %s\n",
                    items[i].name, items[i].highestBid, items[i].highestBidder);
                strcat(response, item_status);
            }
            write(client_sock, response, strlen(response));
        } else if (strcmp(command, "end") == 0) {
            char response[1024] = "\nAuction ended. Final results:\n";
            for (int i = 0; i < ITEM_COUNT; i++) {
                char item_status[600];
                snprintf(item_status, sizeof(item_status),
                    "Item: %s, Final Bid: %d, Winner: %s\n\n",
                    items[i].name, items[i].highestBid, items[i].highestBidder);
                strcat(response, item_status);
            }
            write(client_sock, response, strlen(response));
        } else if (strcmp(command, "list") == 0) {
            char response[1024] = "Available items for bidding:\n";
            for (int i = 0; i < ITEM_COUNT; i++) {
                char item_info[600];
                snprintf(item_info, sizeof(item_info),
                    "Item: %s\n", items[i].name);
                    strcat(response, item_info);
            }
            write(client_sock, response, strlen(response));
        } else {
            char* item_name = command;
            char* bid_str = strtok(NULL, " ");
            if (bid_str) {
                int bid = atoi(bid_str);
                int item_found = 0;
                for (int i = 0; i < ITEM_COUNT; i++) {
                    if (strcmp(items[i].name, item_name) == 0) {
                        item_found = 1;
                        if (bid > items[i].highestBid) {
                            items[i].highestBid = bid;
                            snprintf(items[i].highestBidder, sizeof(items[i].highestBidder), "Client %d", client_id);
                            snprintf(buffer, sizeof(buffer),
                                "New highest bid for %s: %d by %s \n",
                                items[i].name, bid, items[i].highestBidder);
                        } else {
                            snprintf(buffer, sizeof(buffer),
                                "Bid too low for %s. Current highest bid: %d \n",
                                items[i].name, items[i].highestBid);
                        }
                        break;
                    }
                }
                if (!item_found) {
                    snprintf(buffer, sizeof(buffer), "Item %s not found \n", item_name);
                }
            } else {
                snprintf(buffer, sizeof(buffer), "Invalid format \n");
            }
            write(client_sock, buffer, strlen(buffer));
        }
        pthread_mutex_unlock(&lock);
    }

    printf("Client %d disconnected\n", client_id);
    close(client_sock);
    return NULL;
}

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);
    pthread_t thread_id;

    // Initialize auction items with specific names
    snprintf(items[0].name, sizeof(items[0].name), "Painting");
    snprintf(items[1].name, sizeof(items[1].name), "Car");
    snprintf(items[2].name, sizeof(items[2].name), "Jewellery");

    for (int i = 0; i < ITEM_COUNT; i++) {
        items[i].highestBid = 0;
        items[i].highestBidder[0] = '\0';
    }

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);
    
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, MAX_CLIENTS) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    printf("Server started and listening on port %d\n", PORT);

    while (1) {
        if ((new_socket = accept(server_fd, (struct sockaddr *)&address, (socklen_t*)&addrlen)) < 0) {
            perror("accept");
            exit(EXIT_FAILURE);
        }

        ClientData *new_client = malloc(sizeof(ClientData));
        new_client->socket = new_socket;
        new_client->id = client_id++;

        if (pthread_create(&thread_id, NULL, handle_client, (void*)new_client) != 0) {
            perror("Could not create thread");
            free(new_client);
        }
        pthread_detach(thread_id);
    }

    return 0;
}
