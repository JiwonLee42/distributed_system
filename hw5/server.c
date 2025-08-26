#include "util.h"

int SERVER_PORT; // 서버 포트번호
char* kv[DATASET_SIZE]; // 정적 key-value store

void init_kvs() {
    for (int i = 0; i < DATASET_SIZE; i++) {
        kv[i] = malloc(VALUE_SIZE);
        strcpy(kv[i], "DDDCCCCBBBBAAAA");
    }
}

// 간단한 해시 함수로 key를 kv[] 인덱스로 변환
int get_index_from_key(const char* key) {
    int index = 0;
    for (int i = 0; key[i] != '\0'; i++) {
        index += key[i];
    }
    return index % DATASET_SIZE;
}

static volatile int quit = 0; // Ctrl+C 처리
void signal_handler(int signum) {
    if (signum == SIGINT) {
        quit = 1;
    }
}

int main(int argc, char *argv[]) {
    if (argc < 2) {
        printf("Input : %s port number\n", argv[0]);
        return 1;
    }

    signal(SIGINT, signal_handler);
    SERVER_PORT = atoi(argv[1]);

    struct sockaddr_in srv_addr;
    memset(&srv_addr, 0, sizeof(srv_addr));
    srv_addr.sin_family = AF_INET;
    srv_addr.sin_port = htons(SERVER_PORT);
    srv_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    int sock;
    if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
        perror("Could not create socket");
        exit(1);
    }

    if ((bind(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr))) < 0) {
        perror("Could not bind socket");
        exit(1);
    }

    init_kvs(); // 초기화

    struct timeval tv;
    tv.tv_sec = 1;
    tv.tv_usec = 0;
    setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, (const char*)&tv, sizeof(tv));

    if (SERVER_PORT == 5001) {
        // primary 서버의 경우
        while (!quit) {
            struct KVS SendMsg = {0};
            struct KVS RecvMsg = {0};
            struct sockaddr_in src_addr;
            socklen_t src_addr_len = sizeof(src_addr);

            int recvBytes = recvfrom(sock, &RecvMsg, sizeof(RecvMsg), 0, (struct sockaddr *)&src_addr, &src_addr_len);
            if (recvBytes < 0) continue;

            printf("Type: %s Key: %s Value: %s\n", get_type(RecvMsg), RecvMsg.key, RecvMsg.value);

            int index = get_index_from_key(RecvMsg.key);

			// 읽기 요청일 경우
            if (RecvMsg.type == READ_REQ) {
                strncpy(SendMsg.key, RecvMsg.key, KEY_SIZE);
                strncpy(SendMsg.value, kv[index], VALUE_SIZE);
                SendMsg.type = READ_REP;
                sendto(sock, &SendMsg, sizeof(SendMsg), 0, (struct sockaddr *)&src_addr, src_addr_len);
            }

			// 쓰기 요청일 경우
            else if (RecvMsg.type == WRITE_REQ) {
                // primary 저장소 갱신
                strncpy(kv[index], RecvMsg.value, VALUE_SIZE);

                // follower 주소 설정 (5002, 5003)
                struct sockaddr_in follower_addr[2];
                int follower_ports[2] = {5002, 5003};
                for (int i = 0; i < 2; i++) {
                    memset(&follower_addr[i], 0, sizeof(follower_addr[i]));
                    follower_addr[i].sin_family = AF_INET;
                    follower_addr[i].sin_port = htons(follower_ports[i]);
                    inet_pton(AF_INET, "127.0.0.1", &follower_addr[i].sin_addr);
                }

                struct KVS FollowerMsg = RecvMsg;
                struct KVS FollowerResp = {0};
                int ackCount = 0;

				// 팔로워로 메세지 보내기 , 받기 
                for (int i = 0; i < 2; i++) {
                    sendto(sock, &FollowerMsg, sizeof(FollowerMsg), 0, (struct sockaddr *)&follower_addr[i], sizeof(follower_addr[i]));
                    int r = recvfrom(sock, &FollowerResp, sizeof(FollowerResp), 0, NULL, NULL);
                    if (r >= 0 && FollowerResp.type == WRITE_REP) {
                        ackCount++;
                    }
                }

				// 클라이언트로 응답 전송
                if (ackCount == 2) {
                    SendMsg.type = WRITE_REP;
                    strncpy(SendMsg.key, RecvMsg.key, KEY_SIZE);
                    SendMsg.value[0] = '\0'; // write일 때 value는 비워줌
                    sendto(sock, &SendMsg, 17, 0, (struct sockaddr *)&src_addr, src_addr_len);
                    printf("Write is Done\n\n");
                }
            }
        }
    } else {
        // 5002, 5003 follower 서버
        while (!quit) {
            struct KVS RecvMsg = {0};
            struct sockaddr_in src_addr;
            socklen_t src_addr_len = sizeof(src_addr);

            int recvBytes = recvfrom(sock, &RecvMsg, sizeof(RecvMsg), 0, (struct sockaddr*)&src_addr, &src_addr_len);
            if (recvBytes < 0) continue;

			// 쓰기 요청 받기 
            if (RecvMsg.type == WRITE_REQ) {
                printf("Type: %s Key: %s Value: %s\n", get_type(RecvMsg), RecvMsg.key, RecvMsg.value);
                int index = get_index_from_key(RecvMsg.key);
                strncpy(kv[index], RecvMsg.value, VALUE_SIZE);
                RecvMsg.type = WRITE_REP;
                sendto(sock, &RecvMsg, sizeof(RecvMsg), 0, (struct sockaddr*)&src_addr, src_addr_len);
                printf("Write is Done\n\n");
            }
        }
    }

    printf("\nCtrl+C pressed. Exit the program after closing the socket\n");
    close(sock);
    return 0;
}