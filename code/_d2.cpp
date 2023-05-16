#include <iostream>
#include <mavsdk/mavlink/v2.0/common/mavlink.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <sodium.h>
#include <PQCrypto-SIDH/src/P503/P503_api.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string>
#include <time.h>
#define SIKE_PRIVATE_KEY_BYTES 434
#define PUBLIC_KEY_BYTES 564
#define SHARED_SECRET_BYTES 32
#define SERVER_PORT_NUMBER 14550

int main()
{
    //sodinum_init
    if (sodium_init() < 0) {
        printf("Failed to initialize libsodium\n");
        return 1;
    }

    // Create socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);

    // Configure server address
    struct sockaddr_in servaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(14550);

    // Generate SIKE keys
    uint8_t public_key[PUBLIC_KEY_BYTES], private_key[SIKE_PRIVATE_KEY_BYTES], shared_secret[SHARED_SECRET_BYTES];
    randombytes(private_key, SIKE_PRIVATE_KEY_BYTES);
    crypto_kem_keypair_SIKEp503(public_key, private_key);

    // send client's public key to server
    mavlink_message_t msg2;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    uint16_t len2 = mavlink_msg_to_send_buffer(buf, &msg2);
    memcpy(buf + PUBLIC_KEY_BYTES, public_key, PUBLIC_KEY_BYTES);
    len2 = PUBLIC_KEY_BYTES * 2;
    sendto(sockfd, buf, len2, MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

    // Receive server's public key from server 
    socklen_t len_serv = sizeof(servaddr);
    int n = recvfrom(sockfd, buf, PUBLIC_KEY_BYTES * 2, MSG_WAITALL, (struct sockaddr *)&servaddr, &len_serv);
    uint8_t server_public_key[PUBLIC_KEY_BYTES];
    memcpy(server_public_key, buf + SHARED_SECRET_BYTES, PUBLIC_KEY_BYTES);

    // Generate shared secret with server's public key and client's private key
    crypto_kem_enc_SIKEp503(shared_secret, buf, private_key);

    // Print shared secret
    std::cout << "Shared secret: ";
    for (int i = 0; i < SHARED_SECRET_BYTES; i++) {
        std::cout << std::hex << (int)shared_secret[i];
    }
    std::cout << std::endl;
    srand((unsigned)time(NULL));

    // Handle decrypted Mavlink message
    for(int j=0; j<10; j++){
        // Receive encrypted Mavlink message from server
        uint8_t encrypted_packet[MAVLINK_MAX_PACKET_LEN];
        ssize_t n = recvfrom(sockfd, encrypted_packet, MAVLINK_MAX_PACKET_LEN, MSG_WAITALL, (struct sockaddr *)&servaddr, &len_serv);
        printf("Received encrypted Mavlink message from server\n");

        // Decrypt Mavlink message with shared secret
        uint8_t decrypted_mavlink[n - crypto_secretbox_NONCEBYTES];
        crypto_aead_chacha20poly1305_ietf_decrypt(decrypted_mavlink, NULL, NULL, encrypted_packet + crypto_secretbox_NONCEBYTES, n - crypto_secretbox_NONCEBYTES, NULL, 0, encrypted_packet, shared_secret);
        mavlink_message_t msg;
        mavlink_status_t status;
        for (int i = 0; i < n - crypto_secretbox_NONCEBYTES; i++) {
            uint8_t byte = decrypted_mavlink[i];
            if (mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &status)) {
                // Handle received message
                switch (msg.msgid) {
                    case MAVLINK_MSG_ID_HEARTBEAT:
                        mavlink_heartbeat_t heartbeat;
                        mavlink_msg_heartbeat_decode(&msg, &heartbeat);
                        // Process heartbeat message
                        printf("Received heartbeat message: Type=%u, Autopilot=%u, Base Mode=%u, Custom Mode=%u, System Status=%u\n",
                               heartbeat.type, heartbeat.autopilot, heartbeat.base_mode, heartbeat.custom_mode, heartbeat.system_status);
                        break;
                    // Handle other message types as needed
                }
                break;
            }
        }
        sleep(1);
    }

    // Clean up
    close(sockfd);

    return 0;
}
