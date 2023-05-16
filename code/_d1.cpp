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
    uint16_t len = mavlink_msg_to_send_buffer(buf, &msg2);
    memcpy(buf + PUBLIC_KEY_BYTES, public_key, PUBLIC_KEY_BYTES);
    len = PUBLIC_KEY_BYTES * 2;
    sendto(sockfd, buf, len, MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));

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
    
    // Encrypt and send data to server
    for(int i =1; i < 11; i++){
        uint8_t nonce[crypto_secretbox_NONCEBYTES];
        randombytes_buf(nonce, sizeof(nonce));
        uint8_t plaintext[] = { rand() };
        std::cout << "Origin data : " << (int)plaintext[0] << " ---- Encrpted  ---> ";
        uint8_t ciphertext[sizeof(plaintext) + crypto_secretbox_MACBYTES];
        crypto_secretbox_easy(ciphertext, plaintext, sizeof(plaintext), nonce, shared_secret);
        len = sizeof(nonce) + sizeof(ciphertext);
        memcpy(buf, nonce, sizeof(nonce));
        memcpy(buf + sizeof(nonce), ciphertext, sizeof(ciphertext));
        std::cout << (char*)buf << '\n'; 
        sendto(sockfd, buf, len, MSG_CONFIRM, (const struct sockaddr *)&servaddr, sizeof(servaddr));
        sleep(1);
    }
    close(sockfd);
    return 0;
}
