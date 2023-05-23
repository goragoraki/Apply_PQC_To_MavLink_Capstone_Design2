
// 서버
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

int main() {
    std::cout << "server start\n";

    //sodinum_init
    if (sodium_init() < 0) {
        printf("Failed to initialize libsodium\n");
        return 1;
    }

    // UDP 소켓 생성
    int serverSocket;
    struct sockaddr_in serverAddress, clientAddress;
    serverSocket = socket(AF_INET, SOCK_DGRAM, 0);
    if (serverSocket < 0) {
        std::cerr << "Failed to create socket." << std::endl;
        return 1;
    }
    serverAddress.sin_family = AF_INET;
    serverAddress.sin_addr.s_addr = INADDR_ANY;
    serverAddress.sin_port = htons(PORT);

    // UDP 소켓에 주소 바인딩
    if (bind(serverSocket, (struct sockaddr *)&serverAddress, sizeof(serverAddress)) < 0) {
        std::cerr << "Failed to bind socket." << std::endl;
        return 1;
    }

    // 클라이언트로부터 공개 키 수신
    unsigned char client_public_key[PUBLIC_KEY_BYTES];
    socklen_t addrLen = sizeof(clientAddress);
    if (recvfrom(serverSocket, client_public_key, PUBLIC_KEY_BYTES, 0, (struct sockaddr *)&clientAddress, &addrLen) < 0) {
        std::cerr << "Failed to receive client public key." << std::endl;
        return 1;
    }

    // 서버에서 공유 비밀 키 생성
    unsigned char server_private_key[SIKE_PRIVATE_KEY_BYTES];
    unsigned char server_public_key[PUBLIC_KEY_BYTES];
    unsigned char shared_secret[SHARED_SECRET_BYTES];

    // 서버 키 쌍 생성
    randombytes(server_private_key, SIKE_PRIVATE_KEY_BYTES);
    crypto_kem_keypair_SIKEp503(server_public_key, server_private_key);

    // 클라이언트로 전송할 서버 공개 키
    unsigned char server_public_key_sent[PUBLIC_KEY_BYTES];
    memcpy(server_public_key_sent, server_public_key, PUBLIC_KEY_BYTES);

    // 클라이언트에게 서버 공개 키를 서버로 전송
    if (sendto(serverSocket, server_public_key_sent, PUBLIC_KEY_BYTES, 0, (struct sockaddr *)&clientAddress, addrLen) < 0) {
        std::cerr << "Failed to send client public key." << std::endl;
        return 1;
    }

    // 클라이언트에게 전송할 비밀공유키 생성
    unsigned char server_response[PUBLIC_KEY_BYTES + SHARED_SECRET_BYTES];
    crypto_kem_enc_SIKEp503(server_response, client_public_key, server_private_key);

    // 클라이언트에게 비밀공유키 전송
    if (sendto(serverSocket, server_response, sizeof(server_response), 0, (struct sockaddr *)&clientAddress, addrLen) < 0) {
        std::cerr << "Failed to send server response." << std::endl;
        return 1;
    }
   
    // 클라이언트로부터 암호화된 공유 비밀 키를 수신
    unsigned char encrypted_shared_secret_received[SHARED_SECRET_BYTES];
    if (recvfrom(serverSocket, encrypted_shared_secret_received, SHARED_SECRET_BYTES, 0, (struct sockaddr *)&clientAddress, &addrLen) <0){
        std::cerr << "Failed to receive encrypted shared secret." << std::endl;
        return 1;
    }
    
    // 서버에서 공유 비밀 키 복호화
    unsigned char shared_secret_received[SHARED_SECRET_BYTES];
    crypto_kem_dec_SIKEp503(shared_secret_received, encrypted_shared_secret_received, server_private_key);

    std::cout << "Shared secret: ";
    for (int i = 0; i < SHARED_SECRET_BYTES; ++i) {
        std::cout << std::hex << (int)shared_secret_received[i];
    }
    std::cout << std::endl;


    srand((unsigned)time(NULL));
    int cuscmd;
    while(1){
        std::cout << '\n';
        std::cout << "1. 비행 타입 명령 \n";
        std::cout << "2. 이륙 명령 \n";
        std::cout << "3. 목표 위치 이동명령\n";
        std::cout << "4. 착륙 명령 \n";
        std::cout << "5. 종료\n";
        std::cout << "명령 입력 : ";
        std::cin >> cuscmd;

        mavlink_message_t msg;
        if(cuscmd==1){
            int cusmod;
            std::cout<< "비행 타입 명령 전송 - custom mod 입력: ";
            std::cin >> cusmod;
            mavlink_msg_heartbeat_pack(1, 200, &msg, MAV_TYPE_QUADROTOR, MAV_AUTOPILOT_ARDUPILOTMEGA, 0, cusmod, MAV_STATE_ACTIVE);
        }
        if(cuscmd==2){
            int atitude;
            std::cout << "이륙 명령 - 목표 고도 입력 : ";
            std::cin >> atitude;

            mavlink_msg_command_long_pack(1, 1, &msg,
                                  0,   
                                  0,                    
                                  MAV_CMD_NAV_TAKEOFF,   // 이륙 명령                 
                                  0,                    
                                  0,                     // 명령 매개변수 1
                                  0,                     // 명령 매개변수 2
                                  0,                     // 명령 매개변수 3
                                  0,                     // 명령 매개변수 4
                                  0,                     // 명령 매개변수 5
                                  0,                     // 명령 매개변수 6
                                  atitude);             // 고도
        }
        if(cuscmd==3){
            int x,y,z,vx,vy,vz,afx,afy,afz,yw,yrate;

            std::cout << "목표 위치, 목표 가속도, 회전 반경 전송\n";
            printf("목표 위치 입력 m - (x, y, z) : ");
            std::cin >> x >> y >> z;
            printf("속도 입력 m/s - (vx, vy, vz) : ");
            std::cin >> vx >> vy >> vz;
            printf("목표 가속도 입력 m/s/s - (afx, afy, afz) : ");
            std::cin >> afx >> afy >> afz;
            printf("회전 반경 & 비율 degree 입력 - (yw, yrate) : ");
            std::cin >> yw >> yrate;
            mavlink_msg_set_position_target_local_ned_pack(1, 1, &msg,
                                                          0,                       // 타임스탬프
                                                          1,    // 시스템 ID
                                                          1,   // 컴포넌트 ID
                                                          MAV_FRAME_LOCAL_NED,     // 좌표 프레임
                                                          0b0000111111111000,      // 비트마스크
                                                          x, y, z,           // 목표 위치 (x, y, z)
                                                          vx, vy, vz, 
                                                          afx, afy, afz, // 목표 가속도 (afx, afy, afz)
                                                          yw, yrate); // yaw, yaw rate 
        }
        if(cuscmd==4){
            int x,y,z,e;
            std::cout << "착륙위치 입력 (x,y,z) : ";
            std::cin >> x >> y >> z;
            std::cout << "착륙 방향 입력 : "; 
            std::cin >> e;

            mavlink_msg_command_long_pack(1, 1, &msg,
                                  1,   
                                  1,                    
                                  MAV_CMD_NAV_LAND,   // 착륙 명령                 
                                  0,                    
                                  x,                     // 착륙 위치 x
                                  y,                     // 착륙 위치 y
                                  z,                     // 착륙 위치 z
                                  e,                     // 착륙 방향
                                  0,                     //
                                  0,                     // 
                                  0);              //
        }
        if(cuscmd==5){
            break;
        }
        printf("Encrypted Mavlink message sent to drone\n");
        uint8_t buf[MAVLINK_MAX_PACKET_LEN];
        uint16_t len = mavlink_msg_to_send_buffer(buf, &msg);
        uint8_t nonce[SHARED_SECRET_BYTES];
        randombytes_buf(nonce, crypto_secretbox_NONCEBYTES);
        uint8_t encrypted_mavlink[len + crypto_aead_chacha20poly1305_IETF_ABYTES];
        unsigned long long encrypted_len;
        crypto_aead_chacha20poly1305_ietf_encrypt(encrypted_mavlink, &encrypted_len, buf, len, NULL, 0, NULL, nonce, shared_secret_received);
        // Send encrypted Mavlink message to drone
        uint8_t packet[encrypted_len + crypto_secretbox_NONCEBYTES];
        memcpy(packet, nonce, crypto_secretbox_NONCEBYTES);
        memcpy(packet + crypto_secretbox_NONCEBYTES, encrypted_mavlink, encrypted_len);
        len = encrypted_len + crypto_secretbox_NONCEBYTES;
        sendto(serverSocket, packet, len, MSG_CONFIRM, (struct sockaddr *)&clientAddress, addrLen);
        sleep(1);
        std::cout <<'\n';
    }
    // UDP 소켓 종료
    close(serverSocket);

    return 0;
}




