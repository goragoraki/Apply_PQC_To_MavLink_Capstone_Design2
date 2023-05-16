#include <iostream>
#include <mavsdk/mavlink/v2.0/common/mavlink.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sodium.h>
#include <PQCrypto-SIDH/src/P503/P503_api.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctime>
#include <string>
#define SIKE_PRIVATE_KEY_BYTES 434
#define PUBLIC_KEY_BYTES 378
#define SHARED_SECRET_BYTES 32

int main()
{
    std::cout << "server start\n";

    //sodinum_init
    if (sodium_init() < 0) {
        printf("Failed to initialize libsodium\n");
        return 1;
    }

    // Generate SIKE keys
    uint8_t public_key[PUBLIC_KEY_BYTES], private_key[SIKE_PRIVATE_KEY_BYTES], shared_secret[SHARED_SECRET_BYTES];
    randombytes(private_key, SIKE_PRIVATE_KEY_BYTES);
    
    crypto_kem_keypair_SIKEp503(public_key, private_key);

    // Set up Mavlink message
    mavlink_message_t msg;
    uint8_t buf[MAVLINK_MAX_PACKET_LEN];
    mavlink_msg_heartbeat_pack(1, 200, &msg, MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_ARDUPILOTMEGA, 0, 0, MAV_STATE_ACTIVE);
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);

    // Receive client's public key
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in servaddr, cliaddr;
    memset(&servaddr, 0, sizeof(servaddr));
    memset(&cliaddr, 0, sizeof(cliaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = INADDR_ANY;
    servaddr.sin_port = htons(14550);
    bind(sockfd, (const struct sockaddr *)&servaddr, sizeof(servaddr));
    socklen_t len_cli = sizeof(cliaddr);
    int n = recvfrom(sockfd, buf, len, MSG_WAITALL, (struct sockaddr *)&cliaddr, &len_cli);
    uint8_t client_public_key[PUBLIC_KEY_BYTES];
    memcpy(client_public_key, buf, PUBLIC_KEY_BYTES);

    // Send server's public key to client
    memcpy(buf + PUBLIC_KEY_BYTES, public_key, PUBLIC_KEY_BYTES);
    len = PUBLIC_KEY_BYTES * 2;
    sendto(sockfd, buf, len, MSG_CONFIRM, (const struct sockaddr *)&cliaddr, len_cli);

    // Generate shared secret with cilent's public key and server's private key
    crypto_kem_enc_SIKEp503(shared_secret, client_public_key, private_key);
    
    // Print shared secret
    std::cout << "Shared secret: ";
    for (int i = 0; i < SHARED_SECRET_BYTES; i++) {
        std::cout << std::hex << (int)shared_secret[i];
    }
    std::cout << std::endl;

   // Receive and decrypt integer from client
    for(int i =1; i<11; i++){
        uint8_t nonce[crypto_secretbox_NONCEBYTES];
        randombytes_buf(nonce, sizeof(nonce));
        uint8_t plaintext[] = { 0x01 };
        uint8_t ciphertext[sizeof(plaintext) + crypto_secretbox_MACBYTES];
        n = recvfrom(sockfd, buf, sizeof(nonce) + sizeof(ciphertext), MSG_WAITALL, (struct sockaddr *)&cliaddr, &len_cli);
        uint8_t received_nonce[crypto_secretbox_NONCEBYTES];
        memcpy(received_nonce, buf, sizeof(received_nonce));
        uint8_t received_ciphertext[sizeof(ciphertext)];
        memcpy(received_ciphertext, buf + sizeof(nonce), sizeof(ciphertext));
        uint8_t received_plaintext[sizeof(received_ciphertext) - crypto_secretbox_MACBYTES];
        std::cout << "Received Data " << " : " <<received_ciphertext << "----- Decrypt integer ----->: ";
        if (crypto_secretbox_open_easy(received_plaintext, received_ciphertext, sizeof(received_ciphertext), received_nonce, shared_secret) != 0) {
            printf("Failed to decrypt message\n");
            return 1;
        }
        std::cout << (int)received_plaintext[0] << '\n';
    }
    close(sockfd);
    return 0;
}
