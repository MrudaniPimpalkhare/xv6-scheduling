#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <stdint.h>
#include <fcntl.h>
#include <errno.h>
#include <sys/time.h>

#define PORT 8080
#define MAX_CHUNKS 100
#define CHUNK_SIZE 10
#define M_SIZE 2048
#define TIMEOUT 100000 // 0.1 seconds in microseconds
#define WINDOW_SIZE 5


char message[M_SIZE];

typedef struct
{
    int total_chunks;
    uint32_t sequence_number;
    char data[CHUNK_SIZE];
} Chunk;

typedef struct {
    int is_sent;
    int is_acked;
    struct timeval send_time;
} ChunkStatus;

void set_nonblocking(int sockfd) {
    int flags = fcntl(sockfd, F_GETFL, 0);
    if (flags < 0) {
        perror("fcntl(F_GETFL) failed");
        exit(EXIT_FAILURE);
    }

    if (fcntl(sockfd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl(F_SETFL) failed");
        exit(EXIT_FAILURE);
    }
}

void send_ack(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, uint32_t sequence_number) {
    uint32_t ack_seq = htonl(sequence_number); // Convert sequence number to network byte order
    sendto(sockfd, &ack_seq, sizeof(ack_seq), 0, (struct sockaddr *)server_addr, addr_len);
    printf("Sent ACK for chunk %d\n", sequence_number);
}

void send_chunk(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, Chunk *chunk)
{
    sendto(sockfd, chunk, sizeof(Chunk), 0, (struct sockaddr *)server_addr, addr_len);
    printf("Sent chunk %d: %s\n", ntohl(chunk->sequence_number), chunk->data);
}

int receive_ack(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, uint32_t expected_seq) {
    uint32_t ack_seq;
    int recv_len = recvfrom(sockfd, &ack_seq, sizeof(ack_seq), 0, (struct sockaddr *)server_addr, &addr_len);
    
    if (recv_len > 0) {
        ack_seq = ntohl(ack_seq); // Convert ACK to host byte order
        if (ack_seq == expected_seq) {
            printf("Received ACK for chunk %d\n", ack_seq);
            return 1;
        }
    }
    return 0; // ACK not received or incorrect ACK
}

void send_chunks(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len, const char *message)
{
    int total_chunks = (strlen(message) + CHUNK_SIZE - 1) / CHUNK_SIZE;
    ChunkStatus chunk_status[MAX_CHUNKS] = {0};
    Chunk chunks[MAX_CHUNKS];
    int window_start = 0;
    int next_seq_num = 0;
    
    // Prepare all chunks
    for (int i = 0; i < total_chunks; i++) {
        chunks[i].sequence_number = htonl(i);
        strncpy(chunks[i].data, message + i * CHUNK_SIZE, CHUNK_SIZE);
    }

    // Send total_chunks first
    int total_chunks_net = htonl(total_chunks);
    sendto(sockfd, &total_chunks_net, sizeof(total_chunks_net), 0, (struct sockaddr *)server_addr, addr_len);

    while (window_start < total_chunks) {
        // Send new chunks within the window
        while (next_seq_num < total_chunks && next_seq_num < window_start + WINDOW_SIZE) {
            if (!chunk_status[next_seq_num].is_sent) {
                send_chunk(sockfd, server_addr, addr_len, &chunks[next_seq_num]);
                gettimeofday(&chunk_status[next_seq_num].send_time, NULL);
                chunk_status[next_seq_num].is_sent = 1;
            }
            next_seq_num++;
        }

        // Check for acknowledgments and retransmit if necessary
        for (int i = window_start; i < next_seq_num; i++) {
            if (!chunk_status[i].is_acked) {
                struct timeval now;
                gettimeofday(&now, NULL);
                long elapsed = (now.tv_sec - chunk_status[i].send_time.tv_sec) * 1000000 +
                               (now.tv_usec - chunk_status[i].send_time.tv_usec);

                if (elapsed > TIMEOUT) {
                    printf("Retransmitting chunk %d\n", i);
                    send_chunk(sockfd, server_addr, addr_len, &chunks[i]);
                    gettimeofday(&chunk_status[i].send_time, NULL);
                }
            }
        }

        // Receive ACKs
        uint32_t ack;
        int recv_len = recvfrom(sockfd, &ack, sizeof(ack), 0, NULL, NULL);
        if (recv_len > 0) {
            int acked_seq = ntohl(ack);
            printf("Received ACK for chunk %d\n", acked_seq);
            if (acked_seq >= window_start && acked_seq < total_chunks) {
                chunk_status[acked_seq].is_acked = 1;

                // Move window if possible
                while (window_start < total_chunks && chunk_status[window_start].is_acked) {
                    window_start++;
                }
            }
        }

        usleep(1000); // Small delay to avoid busy waiting
    }
}

void receive_chunks(int sockfd, struct sockaddr_in *server_addr, socklen_t addr_len)
{
    static int total_chunks = 0;
    static char *received_message[MAX_CHUNKS] = {NULL};
    static int received_flags[MAX_CHUNKS] = {0};
    static int received_count = 0;

    Chunk chunk;
    int recv_len;

    if (total_chunks == 0) {
        recv_len = recvfrom(sockfd, &total_chunks, sizeof(total_chunks), 0, (struct sockaddr *)server_addr, &addr_len);
        if (recv_len > 0) {
            total_chunks = ntohl(total_chunks);
            printf("Expecting %d chunks.\n", total_chunks);
        } else if (recv_len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("recvfrom failed");
            return;
        }
    }

    if (total_chunks > 0) {
        while (received_count < total_chunks) {
            memset(&chunk, 0, sizeof(chunk));
            recv_len = recvfrom(sockfd, &chunk, sizeof(chunk), 0, (struct sockaddr *)server_addr, &addr_len);

            if (recv_len > 0) {
                int sequence_number = ntohl(chunk.sequence_number);

                if (!received_flags[sequence_number]) {
                    received_message[sequence_number] = strdup(chunk.data);
                    received_flags[sequence_number] = 1;
                    received_count++;
                    printf("Received chunk %d: %s\n", sequence_number, chunk.data);
                    send_ack(sockfd, server_addr, addr_len,sequence_number);
                }

            } else if (recv_len < 0 && errno != EAGAIN && errno != EWOULDBLOCK) {
                perror("recvfrom failed");
                break;
            } else {
                break;
            }
        }

        if (received_count == total_chunks) {
            printf("Aggregating received message:\n");
            for (int i = 0; i < total_chunks; i++) {
                if (received_message[i] != NULL) {
                    printf("%s", received_message[i]);
                    free(received_message[i]);
                    received_message[i] = NULL;
                }
            }
            printf("\n");

            total_chunks = 0;
            received_count = 0;
            memset(received_flags, 0, sizeof(received_flags));
        }
    }
}

int main(int argc, char *argv[])
{
    if (argc != 2) {
        printf("Usage: %s <server IP address>\n", argv[0]);
        return -1;
    }
    int sockfd;
    struct sockaddr_in server_addr;
    socklen_t addr_len = sizeof(server_addr);

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd < 0) {
        perror("Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(PORT);
    
    if (inet_pton(AF_INET, argv[1], &server_addr.sin_addr) <= 0) {
        printf("\nInvalid address/ Address not supported \n");
        return -1;
    }

    set_nonblocking(sockfd);

    fd_set readfds;
    struct timeval tv;

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(sockfd, &readfds);
        FD_SET(STDIN_FILENO, &readfds);

        tv.tv_sec = 0;
        tv.tv_usec = 100000;  // 100ms timeout

        int activity = select(sockfd + 1, &readfds, NULL, NULL, &tv);

        if (activity < 0) {
            perror("select error");
            continue;
        }

        if (FD_ISSET(sockfd, &readfds)) {
            receive_chunks(sockfd, &server_addr, addr_len);
        }

        if (FD_ISSET(STDIN_FILENO, &readfds)) {
            if (fgets(message, M_SIZE, stdin) != NULL) {
                size_t len = strlen(message);
                if (len > 0 && message[len - 1] == '\n') {
                    message[len - 1] = '\0';
                }
                send_chunks(sockfd, &server_addr, addr_len, message);
            }
        }
    }

    close(sockfd);
    return 0;
}