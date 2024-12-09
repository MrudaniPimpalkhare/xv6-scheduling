#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define MAX_CLIENTS 2

char board[3][3];
int client_sockets[MAX_CLIENTS];

void reset_board()
{
    int i, j;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            board[i][j] = ' ';
}

void print_board(int client_socket)
{
    char buffer[1024];
    snprintf(buffer, sizeof(buffer), "\n %c | %c | %c \n---|---|---\n %c | %c | %c \n---|---|---\n %c | %c | %c \n",
             board[0][0], board[0][1], board[0][2],
             board[1][0], board[1][1], board[1][2],
             board[2][0], board[2][1], board[2][2]);
    send(client_socket, buffer, strlen(buffer), 0);
}

int check_winner()
{
    // Check rows and columns
    for (int i = 0; i < 3; i++)
    {
        if (board[i][0] == board[i][1] && board[i][1] == board[i][2] && board[i][0] != ' ')
            return board[i][0] == 'X' ? 1 : 2;
        if (board[0][i] == board[1][i] && board[1][i] == board[2][i] && board[0][i] != ' ')
            return board[0][i] == 'X' ? 1 : 2;
    }

    // Check diagonals
    if (board[0][0] == board[1][1] && board[1][1] == board[2][2] && board[0][0] != ' ')
        return board[0][0] == 'X' ? 1 : 2;
    if (board[0][2] == board[1][1] && board[1][1] == board[2][0] && board[0][2] != ' ')
        return board[0][2] == 'X' ? 1 : 2;

    return 0;
}

int is_draw()
{
    for (int i = 0; i < 3; i++)
        for (int j = 0; j < 3; j++)
            if (board[i][j] == ' ')
                return 0;
    return 1;
}

void handle_client(int client_socket, int player)
{
    int row, col;
    char buffer[1024];
    char mark = player == 1 ? 'X' : 'O';

    while (1)
    {
        memset(buffer, 0, sizeof(buffer));
        snprintf(buffer, sizeof(buffer), "Your move, Player %d (%c): ", player, mark);
        send(client_socket, buffer, strlen(buffer), 0);

        // Get the player's move
        read(client_socket, buffer, sizeof(buffer));
        sscanf(buffer, "%d %d", &row, &col);

        // Validate move
        if (row >= 0 && row < 3 && col >= 0 && col < 3 && board[row][col] == ' ')
        {
            board[row][col] = mark;
            break;
        }
        else
        {
            send(client_socket, "Invalid move! Try again.\n", 25, 0);
        }
    }
}

int play_again(int player1_socket, int player2_socket)
{
    char buffer1[1024], buffer2[1024];

    // Ask both players if they want to play again
    snprintf(buffer1, sizeof(buffer1), "Would you like to play again? Enter Y for yes and N for no: ");
    send(player1_socket, buffer1, strlen(buffer1), 0);
    send(player2_socket, buffer1, strlen(buffer1), 0);

    // Receive their responses
    memset(buffer1, 0, sizeof(buffer1));
    memset(buffer2, 0, sizeof(buffer2));

    read(player1_socket, buffer1, sizeof(buffer1)); // Read Player 1's response
    read(player2_socket, buffer2, sizeof(buffer2)); // Read Player 2's response

    // Remove trailing newline from response, if any
    buffer1[strcspn(buffer1, "\n")] = '\0';
    buffer2[strcspn(buffer2, "\n")] = '\0';

    // Check both responses
    if ((strcmp(buffer1, "Y") == 0 || strcmp(buffer1, "y") == 0) &&
        (strcmp(buffer2, "Y") == 0 || strcmp(buffer2, "y") == 0))
    {
        // Both players want to play again
        return 1; // Game continues
    }
    else if ((strcmp(buffer1, "N") == 0 || strcmp(buffer1, "n") == 0) &&
             (strcmp(buffer2, "Y") == 0 || strcmp(buffer2, "y") == 0))
    {
        // Player 1 doesn't want to play again
        send(player2_socket, "Player 1 does not want to play again.\n", 39, 0);
        return 2; // Game ends
    }
    else if ((strcmp(buffer2, "N") == 0 || strcmp(buffer2, "n") == 0) &&
             (strcmp(buffer1, "Y") == 0 || strcmp(buffer1, "y") == 0))
    {
        // Player 2 doesn't want to play again
        send(player1_socket, "Player 2 does not want to play again.\n", 39, 0);
        return 2; // Game ends
    }
    else
    {
        // Neither player wants to play again or both declined
        send(player1_socket, "Ending game as both players don't want to continue.\n", 52, 0);
        send(player2_socket, "Ending game as both players don't want to continue.\n", 52, 0);
        return 2; // Game ends
    }
}

int main(int argc, char *argv[])
{
     if (argc != 2) {
        printf("Usage: %s <server IP address>\n", argv[0]);
        return -1;
    }
    int server_fd, opt = 1, addrlen;
    struct sockaddr_in address;
    int current_player = 0;
    int game_over = 0; // Flag to control game loop

    // Initialize socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0)
    {
        perror("Socket failed");
        exit(EXIT_FAILURE);
    }

    // Set socket options
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt)))
    {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    // Define server address
    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    // Bind to the address
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0)
    {
        perror("Bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0)
    {
        perror("Listen failed");
        exit(EXIT_FAILURE);
    }

    // Accept two players
    
    while (1)
    {
        printf("Waiting for players to connect...\n");

        // Accept two players
        for (int i = 0; i < MAX_CLIENTS; i++)
        {
            addrlen = sizeof(address);
            if ((client_sockets[i] = accept(server_fd, (struct sockaddr *)&address, (socklen_t *)&addrlen)) < 0)
            {
                perror("Accept failed");
                exit(EXIT_FAILURE);
            }
            printf("Player %d connected.\n", (i + 1));
            send(client_sockets[i], "Welcome to Tic-Tac-Toe!\n", 24, 0);
        }
        reset_board();
        current_player = 0;
        int game_over = 0;
        while (!game_over)
        {

            for (int i = 0; i < MAX_CLIENTS; i++)
            {
                print_board(client_sockets[i]); // Print the board for both players
            }

            handle_client(client_sockets[current_player], current_player + 1); // Handle the current player's move

            // Check for a winner or draw
            int winner = check_winner();
            if (winner)
            {
                char msg[50];
                snprintf(msg, sizeof(msg), "Player %d Wins!\n", winner);
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    send(client_sockets[i], msg, strlen(msg), 0); // Send win message to both players
                    print_board(client_sockets[i]);
                }

                // Ask if they want to play again
                int response = play_again(client_sockets[0], client_sockets[1]);
                if (response == 1)
                {
                    reset_board();      // Reset the board if both players want to play again
                    current_player = 0; // Reset to Player 1
                    continue;           // Start a new game
                }
                else
                {
                    break; // End the game
                }
            }

            // Check for draw
            if (is_draw())
            {
                for (int i = 0; i < MAX_CLIENTS; i++)
                {
                    send(client_sockets[i], "It's a Draw!\n", 13, 0); // Send draw message to both players
                    print_board(client_sockets[i]);
                }

                // Ask if they want to play again
                int response = play_again(client_sockets[0], client_sockets[1]);
                if (response == 1)
                {
                    reset_board();      // Reset the board if both players want to play again
                    current_player = 0; // Reset to Player 1
                    game_over = 0;
                }
                else
                {
                    game_over = 1;
                }
            }

            // Switch to the next player
            current_player = (current_player + 1) % 2;
        }
    }

    printf("Closing connections...\n");
    for (int i = 0; i < MAX_CLIENTS; i++)
    {
        close(client_sockets[i]);
    }

    // Close connections

    return 0;
}
