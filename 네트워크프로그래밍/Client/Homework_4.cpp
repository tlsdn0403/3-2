#include "Common.h"      // 공용 오류 처리 및 유틸 함수(예: err_quit, err_display) 선언부
#include <Windows.h>     // WinSock2를 포함한 윈도우 네트워크/시스템 API
#include <string>
#include <fstream>

char* SERVERIP = (char*)"127.0.0.1";
#define SERVERPORT 9000
#define BUFSIZE 512

#pragma pack(1)
struct DataSet {
	int len = 0;	// 데이터 길이
	char name[50] = "\0";	// 데이터 파일 이름
	char buf[BUFSIZE] = "\0";	// 데이터 버퍼
};
#pragma pack()

int main(int argc, char* argv[])
{
	int retval = 0;

	if (argc != 3)
		return 1;

	SERVERIP = argv[1];  //서버 ip주소 받아줌

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

	// 데이터 통신에 사용할 변수
	DataSet data;
	std::ifstream in{ argv[2], std::ios::binary };
	in.seekg(0, std::ios::end);
	data.len = in.tellg();
	in.seekg(0, std::ios::beg);

	char* pFileName = strrchr(argv[2], '\\');

	(pFileName) ? strcpy(data.name, pFileName + 1) : strcpy(data.name, argv[2]);

	retval = send(sock, (char*)&data.len, sizeof(int), 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
	}

	retval = send(sock, (char*)&data.name, 50, 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
	}

	while (in.read(data.buf, BUFSIZE))
	{
		retval = send(sock, data.buf, BUFSIZE, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
	}

	retval = send(sock, data.buf, data.len % BUFSIZE, 0);
	if (retval == SOCKET_ERROR) {
		err_display("send()");
	}

	printf("[TCP 클라이언트] %d+%d바이트를 "
		"보냈습니다.\n", (int)sizeof(int), data.len);

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}