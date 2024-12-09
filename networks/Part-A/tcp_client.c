#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <server IP address>\n", argv[0]);
        return -1;
    }
    int sock = 0, valread;
    struct sockaddr_in serv_addr;
    char buffer[1024] = {0};

    // Create socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0)
    {
        printf("\n Socket creation error \n");
        return -1;
    }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(PORT);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, argv[1], &serv_addr.sin_addr) <= 0)
    {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    // Connect to server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0)
    {
        printf("\nConnection Failed \n");
        return -1;
    }

    printf("Connected to the server. Waiting for the game to start...\n");

    // Game loop
    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        valread = read(sock, buffer, sizeof(buffer));
        if (valread > 0)
        {
            printf("%s", buffer);

            // If server asks for player's move
            if (strstr(buffer, "Your move"))
            {
                printf("Enter your move (row and column): ");
                fgets(buffer, sizeof(buffer), stdin);
                send(sock, buffer, strlen(buffer), 0);
            }

            // If server asks if the player wants to play again
            else if (strstr(buffer, "play again"))
            {
                fgets(buffer, sizeof(buffer), stdin);
                send(sock, buffer, strlen(buffer), 0);

                // If player declines, break out of the game loop and exit
                if (buffer[0] != 'N' && buffer[0] != 'n' && buffer[0] != 'Y' && buffer[0] != 'y')
                {
                    while (buffer[0] != 'N' && buffer[0] != 'n' && buffer[0] != 'Y' && buffer[0] != 'y')
                    {
                        printf("please give a valid answer.\n");
                        fgets(buffer, sizeof(buffer), stdin);
                        send(sock, buffer, strlen(buffer), 0);
                    }
                }
                else if (buffer[0] == 'N' || buffer[0] == 'n')
                {                    
                    break;
                }else{
                    valread = read(sock, buffer, sizeof(buffer));
                    printf("%s\n", buffer);
                    if(strstr(buffer, "not want to play again"))
                    {
                        break;
                    }
                    
                }
            }
        }
    }

    // Close the socket connection
    close(sock);
    return 0;
}
