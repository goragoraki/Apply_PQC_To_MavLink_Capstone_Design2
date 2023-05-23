// 클라이언트
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

#define PORT 12345
#define SERVER_IP "127.0.0.1" // 서버 IP 주소

int main() {
    //sodinum_init
    if (sodium_init() < 0) {
        printf("Failed to initialize libsodium\n");
        return 1;
    }

    // UDP 소켓 생성
    int clientSocket;
    struct sockaddr_in serverAddress;
    clientSocket = socket(AF_INET, SOCK_DGRAM, 0);
    socklen_t len_serv = sizeof(clientSocket);
    if (clientSocket < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_port = htons(PORT);
    if (inet_aton(SERVER_IP, &serverAddress.sin_addr) == 0) {
        std::cerr << "Invalid server IP address." << std::endl;
        return 1;
    }

    // SIKEp503의 키 생성
    unsigned char client_private_key[SHARED_SECRET_BYTES];
    unsigned char client_public_key[PUBLIC_KEY_BYTES];
    unsigned char shared_secret[SHARED_SECRET_BYTES];

    // 클라이언트 키 쌍 생성
    randombytes(client_private_key, SHARED_SECRET_BYTES);
    crypto_kem_keypair_SIKEp503(client_public_key, client_private_key);

    // 서버로 전송할 클라이언트 공개 키
    unsigned char client_public_key_sent[PUBLIC_KEY_BYTES];
    memcpy(client_public_key_sent, client_public_key, PUBLIC_KEY_BYTES);

    // 클라이언트 공개 키를 서버로 전송
    if (sendto(clientSocket, client_public_key_sent, PUBLIC_KEY_BYTES, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to send client public key." << std::endl;
        return 1;
    }

    // 서버 공개 키 수신
    unsigned char server_public_key[PUBLIC_KEY_BYTES];
    if (recvfrom(clientSocket, server_public_key, PUBLIC_KEY_BYTES, 0, (struct sockaddr *)&serverAddress, &len_serv) < 0) {
        std::cerr << "Failed to receive client public key." << std::endl;
        return 1;
    }

    // 서버로부터 암호화된 공유 비밀 키를 수신
    unsigned char encrypted_shared_secret_received[SHARED_SECRET_BYTES];
    if (recvfrom(clientSocket, encrypted_shared_secret_received, SHARED_SECRET_BYTES, 0, (struct sockaddr *)&serverAddress, &len_serv) < 0) {
        std::cerr << "Failed to receive encrypted shared secret." << std::endl;
        return 1;
    }

    // 클라이언트에서 암호화된 공유 비밀 키를 복호화하여 공유 비밀 키 복원
    unsigned char shared_secret_received[SHARED_SECRET_BYTES];
    crypto_kem_dec_SIKEp503(shared_secret_received, encrypted_shared_secret_received, client_private_key);

    std::cout << "Shared secret: ";
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) {
        std::cout << std::hex << (int)shared_secret_received[i];
    }
    std::cout << std::endl;

    // 클라이언트에게 전송할 비밀공유키 생성
    unsigned char client_response[PUBLIC_KEY_BYTES + SHARED_SECRET_BYTES];
    crypto_kem_enc_SIKEp503(client_response, server_public_key, client_private_key);

    // 암호화된 비밀 공유 키 전송
    if (sendto(clientSocket, client_response, SHARED_SECRET_BYTES, 0, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to send encrypted shared secret." << std::endl;
        return 1;
    }
    std::cout << "\n";
    srand((unsigned)time(NULL));

    // Handle decrypted Mavlink message
    for(int j=0; j<10; j++){
        // Receive encrypted Mavlink message from server
        uint8_t encrypted_packet[MAVLINK_MAX_PACKET_LEN];
        ssize_t n = recvfrom(clientSocket, encrypted_packet, MAVLINK_MAX_PACKET_LEN, MSG_WAITALL, (struct sockaddr *)&serverAddress, &len_serv);
        printf("Received encrypted Mavlink message from server\n");

        // Decrypt Mavlink message with shared secret
        uint8_t decrypted_mavlink[n - crypto_secretbox_NONCEBYTES];
        crypto_aead_chacha20poly1305_ietf_decrypt(decrypted_mavlink, NULL, NULL, encrypted_packet + crypto_secretbox_NONCEBYTES, n - crypto_secretbox_NONCEBYTES, NULL, 0, encrypted_packet, shared_secret_received);
        mavlink_message_t msg;
        mavlink_status_t status;
        for (int i = 0; i < n - crypto_secretbox_NONCEBYTES; i++) {
            uint8_t byte = decrypted_mavlink[i];
            if (mavlink_parse_char(MAVLINK_COMM_0, byte, &msg, &status)) {
                // Handle received message
                switch (msg.msgid) {
                    case MAVLINK_MSG_ID_HEARTBEAT:{
                        mavlink_heartbeat_t heartbeat;
                        mavlink_msg_heartbeat_decode(&msg, &heartbeat);
                        // Process heartbeat message
                        printf("비행 타입 정보: Type=%u, Autopilot=%u, Base Mode=%u, Custom Mode=%u, System Status=%u\n",
                               heartbeat.type, heartbeat.autopilot, heartbeat.base_mode, heartbeat.custom_mode, heartbeat.system_status);
                        break;
                    }
                    case MAVLINK_MSG_ID_COMMAND_LONG: {
                        mavlink_command_long_t cmd;
                        mavlink_msg_command_long_decode(&msg, &cmd);
                        if (cmd.command == MAV_CMD_NAV_TAKEOFF) {
                            std::cout << "이륙 명령 수신 - 목표 고도: " << cmd.param7 << "m\n";
                        }else if(cmd.command == MAV_CMD_NAV_LAND){
                            std::cout << "착륙 명령 수신 - 위치: (" << cmd.param1 << ", " << cmd.param2 << ", " << cmd.param3 << ") m\n";
                            std::cout << "착륙 방향 " << cmd.param4 <<" degree\n";
                        }
                        break;
                    }
                    case MAVLINK_MSG_ID_SET_POSITION_TARGET_LOCAL_NED: {
                        mavlink_set_position_target_local_ned_t pos;
                        mavlink_msg_set_position_target_local_ned_decode(&msg, &pos);
                        std::cout << "비행 명령 수신 - 위치: (" << pos.x << ", " << pos.y << ", " << pos.z << ") m\n";
                        std::cout << "비행 명령 수신 - 속도: (" << pos.vx << ", " << pos.vy << ", " << pos.vz << ") m/s\n";
                        std::cout << "비행 명령 수신 - 목표 가속도: (" << pos.afx << ", " << pos.afy << ", " << pos.afz << ") m/s/s\n";
                        std::cout << "비행 명령 수신 - Yaw: " << pos.yaw << ", Yaw Rate: " << pos.yaw_rate << " degree\n";
                        break;
                    }
                }
                break;
            }
        }
        sleep(1);
        std::cout <<'\n';
    }
    
    // UDP 소켓 종료
    close(clientSocket);

    return 0;
}
