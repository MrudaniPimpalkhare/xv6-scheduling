#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>


#define PORT 8080
#define BUFFER_SIZE 1024
#define SERVER_IP "127.0.0.1"
int main(int argc, char *argv[]) {
    if (argc != 2) {
        printf("Usage: %s <server IP address>\n", argv[0]);
        return -1;
    }
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    // Server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);

    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    //server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    // Connect to server
    sendto(sockfd, "Connect", 7, 0, (struct sockaddr *)&server_addr, addr_len);
    while (1) {
        // Receive message from server
        int n = recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&server_addr, &addr_len);
        buffer[n] = '\0';
        // Check if game has ended
        if (strncmp(buffer, "Game session ended", 18) == 0) {
            printf("%s\n", buffer);
            break;
        }
        // Print received message
        printf("%s\n", buffer);
        // Check if it's this player's turn
        if (strstr(buffer, "turn")) {
            printf("Enter your move: ");
            fgets(buffer, BUFFER_SIZE, stdin);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, addr_len);
        } else if (strstr(buffer, "play again")) {
            fgets(buffer, BUFFER_SIZE, stdin);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&server_addr, addr_len);
        }
    }
    close(sockfd);
    return 0;
}