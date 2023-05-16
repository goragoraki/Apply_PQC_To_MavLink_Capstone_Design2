#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <mavsdk/mavlink/v2.0/common/mavlink.h>

#define BUFFER_LENGTH 1024

int main() {
    int server_fd, new_socket;
    struct sockaddr_in address;
    int opt = 1;
    int addrlen = sizeof(address);
    char buffer[BUFFER_LENGTH] = {0};
    std::cout << "Receive drone's Data << '\n";
    // Creating socket file descriptor
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    // Forcefully attaching socket to the port 14550
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR | SO_REUSEPORT, &opt, sizeof(opt))) {
        perror("setsockopt");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(14550);

    // Bind the socket to the specified address
    if (bind(server_fd, (struct sockaddr *) &address, sizeof(address)) < 0) {
        perror("bind failed");
        exit(EXIT_FAILURE);
    }

    // Listen for incoming connections
    if (listen(server_fd, 3) < 0) {
        perror("listen");
        exit(EXIT_FAILURE);
    }

    // Accept incoming connection
    if ((new_socket = accept(server_fd, (struct sockaddr *) &address, (socklen_t *) &addrlen)) < 0) {
        perror("accept");
        exit(EXIT_FAILURE);
    }

    // Receive data from client
    mavlink_message_t msg;
    mavlink_status_t status;
    int recv_bytes;
    mavlink_heartbeat_t heartbeat;

    while (true) {
        // Receive Mavlink message
        if ((recv_bytes = recv(new_socket, buffer, BUFFER_LENGTH, 0)) < 0) {
            perror("recv");
            exit(EXIT_FAILURE);
        }

        // Parse Mavlink message
        for (int i = 0; i < recv_bytes; i++) {
            if (mavlink_parse_char(MAVLINK_COMM_0, buffer[i], &msg, &status)) {
                switch (msg.msgid) {
                    case MAVLINK_MSG_ID_HEARTBEAT:
                        mavlink_msg_heartbeat_decode(&msg, &heartbeat);
                        std::cout << "Received data: " << heartbeat.custom_mode << std::endl;
                        break;
                    default:
                        break;
                }
            }
        }
    }

    // Close socket
    close(new_socket);
    close(server_fd);

    return 0;
}
