#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <errno.h>

#define UDP_PORT 18333
#define BROADCAST_IP "255.255.255.255"
#define UDP_MESSAGE "UDP BROADCAST MESSAGE\n"
#define BUFFER_SIZE 1024
#define BROADCAST_DELAY 5 //5 seconds wait timeout between broadcast messages


void* Broadcast_task(void* arg) {
    struct sockaddr_in udp_broadcast_addr, udp_server_addr;
    int udp_sock;
    int broadcast_enable = 1;
    int received_server_response = 0;
    socklen_t addr_len = sizeof(udp_server_addr); // Corrected to use udp_server_addr
    char buffer[BUFFER_SIZE];

    // Set up the broadcast address
    memset(&udp_broadcast_addr, 0, sizeof(udp_broadcast_addr));
    udp_broadcast_addr.sin_family = AF_INET;
    udp_broadcast_addr.sin_port = htons(UDP_PORT);
    udp_broadcast_addr.sin_addr.s_addr = inet_addr(BROADCAST_IP);

    // Set receive timeout
    struct timeval timeout;
    timeout.tv_sec = BROADCAST_DELAY;  // Set timeout duration in seconds
    timeout.tv_usec = 0;             // Set microseconds to 0

    // Create a UDP socket for broadcasting
    udp_sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (udp_sock == -1) {
        perror("UDP socket creation failed");
        pthread_exit(NULL);
    }

    // Enable broadcast option on the UDP socket
    if (setsockopt(udp_sock, SOL_SOCKET, SO_BROADCAST, &broadcast_enable, sizeof(broadcast_enable)) == -1) {
        perror("UDP setsockopt (SO_BROADCAST) failed");
        close(udp_sock);
        pthread_exit(NULL);
    }
    // Enable recvfrom timeout
    if (setsockopt(udp_sock, SOL_SOCKET, SO_RCVTIMEO, &timeout, sizeof(timeout)) < 0) {
        perror("setsockopt failed");
        close(udp_sock);
        pthread_exit(NULL);
    }

    // Send a UDP broadcast message in a loop
    while (!received_server_response) {
        // Send broadcast message
        if (sendto(udp_sock, UDP_MESSAGE, strlen(UDP_MESSAGE), 0,
                   (struct sockaddr*)&udp_broadcast_addr, sizeof(udp_broadcast_addr)) == -1) {
            perror("UDP sendto failed");
            close(udp_sock);
            pthread_exit(NULL);
        }

        printf("UDP broadcast message sent: %s\n", UDP_MESSAGE);
        
        // Wait for a response
        printf("Waiting for response...\n");
        ssize_t bytes_received = recvfrom(udp_sock, buffer, BUFFER_SIZE, 0,
                                          (struct sockaddr*)&udp_server_addr, &addr_len);

        // Check if recvfrom failed
        if (bytes_received < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("No response received within %d seconds.\n", BROADCAST_DELAY);
            } else {
                perror("UDP recvfrom failed");
                close(udp_sock);
                pthread_exit(NULL);
            }
        } else {
            buffer[bytes_received] = '\0';  // Null-terminate the received message
            printf("Received response from server: %s\n", buffer);
            received_server_response = 1; // Mark that we received a response
        }
    }

    // Close the socket
    close(udp_sock);
    printf("Broadcast Task finished.\n");
    pthread_exit(NULL);
}

int main() {
    //Task
    pthread_t broadcast_task_handler;

    // Create the task for sending broadcast messages
    if (pthread_create(&broadcast_task_handler, NULL, Broadcast_task, NULL) != 0) {
        perror("Failed to create broadcast thread");
        return 1;
    }

    // Wait for broadcast task to finish
    pthread_join(broadcast_task_handler, NULL);
    return 0;
}