# Networking

# XOXO 

## TCP 

### Client Code 

The client program performs the following actions:

1. Establish a connection with the server using the provided IP address and port number.
2. Receive board updates and game status from the server after each move.
3. Send input (row and column) for the player’s move.
At the end of the game, ask the player if they want to play again.
4. Close the socket if the player or opponent decides not to continue.

5. Socket creation: Initializes a TCP socket to connect to the server.
6. Receiving and sending data: Uses read() and send() to communicate with the server.
7. Game loop: Receives game state updates and sends player input until the game ends.


### Server code 
The server handles multiple clients and coordinates the game. It performs the following tasks:

1. Initialize the board and wait for two players to connect.
2. Send the game board to both clients after each move.
3. Validate moves and ensure that the players play in turn.
Check for a winner or draw after every move.
4. Ask both players if they want to play again after the game ends. If not, the server closes the connection.
Key sections:

5. Socket setup: Creates a TCP socket, binds to a port, and listens for two client connections.
6. Game loop:
Sends board updates to both clients.
Validates moves and switches turns between players.
Checks if there is a winner or draw.
Handles the "play again" logic.
7. Board reset: Resets the game state if both players agree to replay.

```reset_board()``` : empties the board 

```print_board()``` : sends board results to the client 

```check_winner()``` : checks if there is a winner

```is_draw()``` : checks if the board is full and there is no winner 

```handle_client()``` : sends the prompt to the appropriate player, recieves the move and checks for invalid moves 


```play_again()``` : checks whether both players want to play again. If either one doesnt want to play, it sends the appropriate message. 


## UDP 

Similar logic/ functioning as TCP, except with a few differences in implementing networking. 

1. Uses sendto() and recvfrom() for both sending and receiving data, allowing direct communication without maintaining a connection state.

2. There is no explicit connect() or accept() like in TCP, which manages sessions.

3. The server differentiates between clients using the socket address (stored in struct sockaddr_in for each client).

4. In UDP, since packets can be lost, the communication may not always be reliable. This means you need to implement error handling if a packet (like a move) doesn’t arrive.


# Fake it till you make it

## **Overview**
This code simulates a **reliable UDP communication** between a client and server, where:
- A large message is **divided into smaller chunks** of fixed size.
- UDP, being **unreliable**, requires mechanisms for handling lost packets.
- **Stop-and-Wait ARQ** is implemented with **ACKs (acknowledgments)** and **timeouts** to ensure successful transmission.
- A **sliding window approach** improves transmission efficiency.

---

## **Global Definitions**
```c
#define PORT 8080           // Communication port
#define MAX_CHUNKS 100      // Maximum number of chunks in a message
#define CHUNK_SIZE 10       // Size of each chunk in bytes
#define M_SIZE 2048         // Maximum message size
#define TIMEOUT 100000      // Timeout for retransmission (0.1s)
#define WINDOW_SIZE 5       // Max number of unacknowledged chunks (window size)
```

1. each message is broken into multiple chunk structures, and the ChunkStatus structure tracks the status (acknowledged, sent, time at which it was sent).

2. ```set_nonblocking()``` sets the socket to non blocking mode to prevent waiting during ```recvfrom()```

3. ```send_chunk()``` sends chunks to the server and ```send_ack``` sends acknowledgements

4. ```receive_ack()``` checks for recieved acknowledgments

5. ```send_chunks()``` sends message in chunks using a sliding window technique. chunks are sent within a particular window size, and the send time is recorded. If no acknowledgment is received within the timeout, then the chunk is retransmitted. Once a chunk is acknowledged, slide the window forward.


the server logic is almost the same, and contains the same functions since the communication is two-way.