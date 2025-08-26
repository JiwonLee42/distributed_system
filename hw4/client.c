#include "util.h"

const char* dst_ip = "127.0.0.1";
#define NUM_SRV 4

// 임의의 key를 생성해서 반환해줌
void generate_key(char* key) {
    uint64_t number = rand() % DATASET_SIZE;
    for (int i = 0; i < 5; ++i) number = ((number << 3) - number + 7) & 0xFFFFFFFFFFFFFFFF;
    key[KEY_SIZE - 1] = '\0';
    for (int i = KEY_SIZE - 2; i >= 0; i--) {
        int index = number % SET_SIZE;
        key[i] = SET[index];
        number /= SET_SIZE;
    }
}
 
int main(int argc, char *argv[]) {

  srand((unsigned int)time(NULL));  // 난수 발생기 초기화
  /* 서버 구조체 설정 */
	int SERVER_PORT = 5001;
	struct sockaddr_in srv_addr; // 패킷을 수신할 서버의 정보를 담을 소켓 구조체를 생성한다.
	memset(&srv_addr, 0, sizeof(srv_addr)); // 구조체를 모두 '0'으로 초기화해준다.
	srv_addr.sin_family = AF_INET; // IPv4를 사용할 것이므로 AF_INET으로 family를 지정한다.
	srv_addr.sin_port = htons(SERVER_PORT); // 서버의 포트번호를 넣어준다. 이 때 htons()를 통해 byte order를 network order로 변환한다.
	inet_pton(AF_INET, dst_ip, &srv_addr.sin_addr);  // 문자열인 IP주소를 바이너리로 변환한 후 소켓 구조체에 저장해준다.

  /* 소켓 생성 */
	int sock; // 소켓 디스크립터(socket descriptor)를 생성한다.
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) { // socket()으로 IPv4(AF_INET), UDP(SOC_DGRAM)를 사용하는 소켓을 생성 시도한다.
		printf("Could not create socket\n"); // sock으로 return되는 값이 -1이라면 소켓 생성에 실패한 것이다.
		exit(1);
	}

	int n = 0;

  struct KVS SendMsg={0,}; // 송신용으로 쓸 메시지 구조체 생성 및 초기화

  struct sockaddr_in src_addr; // 패킷을 수신하였을 때, 해당 패킷을 보낸 송신자(Source)의 정보를 저장하기 위한 소켓 구조체
  socklen_t src_addr_len = sizeof(src_addr); // 수신한 패킷의 소켓 구조체 크기를 저장함. IPv4를 사용하므로 sockaddr_in 크기인 16바이트가 저장됨.
  int cnt = 0; // 패킷 5개를 전송한다.
  size_t pkt_size = 0;
  srand(time(NULL));
  int connections[4] = {0,0,0,0}; // 연결 수를 저장하는 배열 선언
	while(cnt < 5){
    printf("Request ID: %d\n",cnt++);
    // 랜덤으로 두대를 선택, 두 서버의 요청수 비교, 더 적은 서버가 목적지
    int first, second;
    first = rand() % 4; // 무작위로 서버 하나를 선택
    do {
      second = rand() % 4;
    } while (second == first);
    int selected = connections[first] < connections[second] ? first : second; // 연결 수가 더 적은 서버
    int port_n = 5001 + selected; // 선택된 서버로 연결할 포트 번호 설정
    srv_addr.sin_port = htons(port_n);
    generate_key(SendMsg.key); // 랜덤 key를 새로 생성
    SendMsg.type = READ_REQ; // 읽기 요청으로 설정 
    int sentBytes = sendto(sock, &SendMsg, 17, 0, (struct sockaddr *)&srv_addr, sizeof(srv_addr)); // 읽기 요청 전송
    connections[selected]++; // 최종 선택된 서버의 연결 수 증가
    printf("Sent bytes: %d\n", sentBytes); // 패킷 크기 출력
    printf("Type: %s Key: %s\n", get_type(SendMsg), SendMsg.key); // 요청 내용 출력
    struct KVS RecvMsg={0,}; // 수신용으로 쓸 메시지 구조체 생성 및 초기화
    int recvBytes = recvfrom(sock, &RecvMsg, sizeof(RecvMsg), 0, (struct sockaddr *)&src_addr, &src_addr_len); // 서버 노드에서 응답 송신
    printf("Received bytes: %d\n", recvBytes); // 받은 패킷 크기 출력
    printf("Type: %s Key: %s Value: %s\n", get_type(RecvMsg), RecvMsg.key,RecvMsg.value); // 전송한 메시지 출력 
	}
	close(sock); // 소켓을 닫아준다.
	return 0;
}