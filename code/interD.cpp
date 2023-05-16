#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <mavsdk/mavlink/v2.0/common/mavlink.h>
#define BUFFER_LENGTH 1024

int main() {
    int client_fd;
    struct sockaddr_in server_address;
    char buffer[BUFFER_LENGTH] = {0};
    std::cout << "Send the Data to Drone Center" << '\n';
    sleep(1);
    // Create socket file descriptor
    if ((client_fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_address, '0', sizeof(server_address));

    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(14550);

    // Convert IPv4 and IPv6 addresses from text to binary form
    if (inet_pton(AF_INET, "127.0.0.1", &server_address.sin_addr) <= 0) {
        perror("invalid address or address not supported");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    if (connect(client_fd, (struct sockaddr *) &server_address, sizeof(server_address)) < 0) {
        perror("connection failed");
        exit(EXIT_FAILURE);
    }

    // Send data to server
    for(int i =0; i<10; i++){
        mavlink_message_t msg;
        uint8_t buffer_send[BUFFER_LENGTH];
        mavlink_heartbeat_t heartbeat = {0};

        heartbeat.custom_mode = rand();
        mavlink_msg_heartbeat_encode(1, 200, &msg, &heartbeat);
        uint16_t len = mavlink_msg_to_send_buffer(buffer_send, &msg);

        if (send(client_fd, buffer_send, len, 0) < 0) {
            perror("send");
            exit(EXIT_FAILURE);
        }

        std::cout << "Data sent: " << heartbeat.custom_mode << std::endl;
        sleep(1);
    }
    // Close socket
    close(client_fd);

    return 0;
}