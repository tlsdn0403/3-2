#include "Common.h"

char* SERVERIP = (char*)"192.168.52.1";
#define SERVERPORT 9000
#define BUFSIZE    50

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
	if (sock == INVALID_SOCKET) err_quit("socket()");

	// connect()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
	serveraddr.sin_port = htons(SERVERPORT);
	retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("connect()");

	// argc는 명령행 입력의 개수
	//argv[0]은 기본적으로 프로그램 실행 파일이므로 , argv[1]부터 실제 입력값
	if (argc != 2) {
		WSACleanup();
		return 0;
	}
	FILE* fp = fopen(argv[1], "rb");  // 바이너리 모드로 파일을 열겠다 

	// 파일 열기 실패
	if (!fp)
	{
		printf("File Read Error\n");
		WSACleanup();
		return 0;
	}
	

	char buffer[BUFSIZE] = {};
	while (1)
	{
		int readCnt = fread((void*)buffer, 1, BUFSIZE, fp);

		send(sock, (char*)&buffer, readCnt, 0);

		if (readCnt < BUFSIZE) break;
	}

	shutdown(sock, SD_SEND);
	recv(sock, (char*)buffer, BUFSIZE, 0);
	printf("Message from client: %s \n", buffer);
	fclose(fp);

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}