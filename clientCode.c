#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main() {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};
    char input[1024];

    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        printf("\nSocket creation error\n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, "127.0.0.1", &serv_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported\n");
        return -1;
    }

    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        printf("\nConnection Failed\n");
        return -1;
    }

    while (1) {
        printf("Enter command (format: ItemName BidAmount, 'status', 'end', 'list bid items', or 'exit'): ");
        fgets(input, sizeof(input), stdin);
        input[strcspn(input, "\n")] = '\0';  // Remove trailing newline

        if (strcmp(input, "exit") == 0) {
            break;
        }

        send(sock, input, strlen(input), 0);
        int valread = read(sock, buffer, sizeof(buffer) - 1);
        buffer[valread] = '\0';
        printf("Server: %s\n", buffer);
    }

    close(sock);
    return 0;
}

        
