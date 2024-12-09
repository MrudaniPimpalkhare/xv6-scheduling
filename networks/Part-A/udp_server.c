#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#define PORT 8080
#define BUFFER_SIZE 1024
char board[3][3];
int current_player = 1;
void init_board()
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            board[i][j] = ' ';
        }
    }
}
int check_win()
{
    // Check rows and columns
    for (int i = 0; i < 3; i++)
    {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return 1;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return 1;
    }
    // Check diagonals
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return 1;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return 1;
    return 0;
}
int is_board_full()
{
    for (int i = 0; i < 3; i++)
    {
        for (int j = 0; j < 3; j++)
        {
            if (board[i][j] == ' ')
                return 0;
        }
    }
    return 1;
}
void send_board(int sockfd, struct sockaddr_in *client_addr, socklen_t addr_len)
{
    char buffer[BUFFER_SIZE];
    snprintf(buffer, BUFFER_SIZE, "\n %c | %c | %c \n---|---|---\n %c | %c | %c \n---|---|---\n %c | %c | %c \n",
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);
    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)client_addr, addr_len);
}
int main(int argc, char *argv[])
{
    int sockfd;
    struct sockaddr_in server_addr, client_addr[2];
    char buffer[BUFFER_SIZE];
    socklen_t addr_len = sizeof(struct sockaddr_in);
    // Create socket
    if ((sockfd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }
    memset(&server_addr, 0, sizeof(server_addr));
    memset(&client_addr, 0, sizeof(client_addr));
    // Server information
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(argv[1]);
    server_addr.sin_port = htons(PORT);
    // Bind socket
    if (bind(sockfd, (const struct sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }
    // Game loop
    while (1)
    {
        printf("Server is running. Waiting for players...\n");
        for (int i = 0; i < 2; i++)
        {
            recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr[i], &addr_len);
            printf("Player %d connected\n", i + 1);
            sprintf(buffer, "You are Player %d. Waiting for opponent...", i + 1);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr[i], addr_len);
        }
        init_board();
        current_player = 1;
        int game_over = 0;
        while (!game_over)
        {
            // Send current board state to both players
            for (int i = 0; i < 2; i++)
            {
                send_board(sockfd, &client_addr[i], addr_len);
            }
            // Prompt current player for move
            sprintf(buffer, "Player %d's turn. Enter row and column (e.g., 1 2):", current_player);
            sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr[current_player - 1], addr_len);
            // Receive move from current player
            recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr[current_player - 1], &addr_len);
            int row, col;
            sscanf(buffer, "%d %d", &row, &col);
            // Check if move is valid
            if (row < 0 || row > 2 || col < 0 || col > 2 || board[row][col] != ' ')
            {
                sprintf(buffer, "Invalid move. Try again.");
                sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr[current_player - 1], addr_len);
                continue;
            }
            // Update board
            board[row][col] = (current_player == 1) ? 'X' : 'O';
            // Check for win or draw
            if (check_win())
            {
                sprintf(buffer, "Player %d wins!", current_player);
                game_over = 1;
            }
            else if (is_board_full())
            {
                sprintf(buffer, "It's a draw!");
                game_over = 1;
            }
            if (game_over)
            {
                // Send final board state and game result to both players
                for (int i = 0; i < 2; i++)
                {
                    send_board(sockfd, &client_addr[i], addr_len);
                    sendto(sockfd, buffer, strlen(buffer), 0, (struct sockaddr *)&client_addr[i], addr_len);
                }
                // Ask if players want to play again
                int play_again[2] = {0, 0};
                sendto(sockfd, "Do you want to play again? (yes/no)", 35, 0, (struct sockaddr *)&client_addr[0], addr_len);
                sendto(sockfd, "Do you want to play again? (yes/no)", 35, 0, (struct sockaddr *)&client_addr[1], addr_len);
                recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr[0], &addr_len);
                play_again[0] = (strncmp(buffer, "yes", 3) == 0);
                recvfrom(sockfd, buffer, BUFFER_SIZE, 0, (struct sockaddr *)&client_addr[1], &addr_len);
                play_again[1] = (strncmp(buffer, "yes", 3) == 0);
                if (play_again[0] && play_again[1])
                {
                    game_over = 0; // Start a new
                    init_board();
                }
                else
                {
                    // End the game session
                    if(!play_again[0] && play_again[1])
                    {   
                        sendto(sockfd, "Player 1 doesnt wanna play", 35, 0, (struct sockaddr *)&client_addr[1], addr_len);
                    }else if(!play_again[1] && play_again[0]){
                        sendto(sockfd, "Player 2 doesnt wanna play", 35, 0, (struct sockaddr *)&client_addr[0], addr_len);
                    }
                    for (int i = 0; i < 2; i++)
                    {
                        sendto(sockfd, "Game session ended. Goodbye!", 28, 0, (struct sockaddr *)&client_addr[i], addr_len);
                    }
                }
            }
            else
            {
                // Switch to the other player
                current_player = (current_player == 1) ? 2 : 1;
            }
        }
    }
    close(sockfd);
    return 0;
}