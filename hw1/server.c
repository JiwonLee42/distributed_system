#include "util.h"

int SERVER_PORT; // 서버 포트번호

static volatile int quit = 0; // Trigger conditions for SIGINT
void signal_handler(int signum) {
	if(signum == SIGINT){  // Functions for Ctrl+C (SIGINT)
		quit = 1;
	}
}

int main(int argc, char *argv[]) {
	// 프로그램 시작시 입력받은 매개변수를 parsing한다. 
	if ( argc < 2 ){
	 printf("Input : %s port number\n", argv[0]);
	 return 1;
	}

	signal(SIGINT, signal_handler); // SIGINT에 대한 핸들러 등록

	SERVER_PORT = atoi(argv[1]); // 입력받은 argument를 포트번호 변수에 넣어준다.

	// 서버의 정보를 담을 소켓 구조체 생성 및 초기화
	struct sockaddr_in srv_addr;
	memset(&srv_addr, 0, sizeof(srv_addr));
	srv_addr.sin_family = AF_INET;
	srv_addr.sin_port = htons(SERVER_PORT);
	srv_addr.sin_addr.s_addr = htonl(INADDR_ANY); // 0.0.0.0 i.e., 자기 자신의 IP

	// 소켓을 생성한다.
	int sock;
	if ((sock = socket(AF_INET, SOCK_DGRAM, 0)) < 0) {
		printf("Could not create listen socket\n");
		exit(1);
	}

	// 생성한 소켓에 소켓구조체를 bind시킨다.
	if ((bind(sock, (struct sockaddr *)&srv_addr, sizeof(srv_addr))) < 0) {
		printf("Could not bind socket\n");
		exit(1);
	}

	int n = 0;
  struct KVS RecvMsg={0,}; // 수신용으로 쓸 메시지 구조체 생성 및 초기화

	struct sockaddr_in src_addr; // 패킷을 수신하였을 때, 해당 패킷을 보낸 송신자(Source)의 정보를 저장하기 위한 소켓 구조체
  socklen_t src_addr_len = sizeof(src_addr);

	while (!quit) {
		struct KVS SendMsg={0,}; // 송신용으로 쓸 메시지 구조체 생성 및 초기화
		recvfrom(sock, &RecvMsg, sizeof(RecvMsg), 0,(struct sockaddr*)&src_addr, &src_addr_len); // 클라이언트로부터 RecvMsg를 통해 메시지를 수신
		char str[1024]; // 클라이언트에서 수신한 문자열을 저장할 용도의 문자열 선언
		strcpy(str, RecvMsg.value); // 클라이언트 메시지에서 value 값에 해당하는 문자열을 받아와서 선언한 str 변수에 저장
		strcpy(SendMsg.key, RecvMsg.key); // 클라이언트 메시지에서 key값 복사
		SendMsg.type = READ_REP; // 패킷의 타입을 read reply로 변경
		int len = strlen(str);
		for (int i = 0; i < len / 2; i++){ // 문자열을 역순으로 변환 
			char temp = str[i];
			str[i] = str[len-1-i];
			str[len-1-i] = temp;
		}
		strcpy(SendMsg.value, str); // 변환한 문자열을 송신용 패킷의 value로 복사
		sendto(sock,&SendMsg,sizeof(SendMsg),0,(struct sockaddr*)&src_addr, src_addr_len); // SendMsg를 클라이언트로 전송
	}
	printf("\nCtrl+C pressed. Exit the program after closing the socket\n");
	close(sock);

	return 0;
}
