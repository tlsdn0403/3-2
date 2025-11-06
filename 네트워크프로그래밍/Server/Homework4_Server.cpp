#include <fstream>
#include"Common.h"     
#define SERVERPORT 9000
#define BUFSIZE    20

#pragma pack(1)
struct DataSet {
	int len = 0;	// 데이터 길이
	char name[50] = "\0";	// 데이터 파일 이름
	char buf[BUFSIZE] = "\0";	// 데이터 버퍼
};
#pragma pack()

#pragma pack(1)
struct Params {
	SOCKET client_sock;
	COORD curpos;
};
#pragma pack()

HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
CRITICAL_SECTION cs;

// 클라이언트와 데이터 통신
//스레드 함수
DWORD WINAPI ProcessClient(LPVOID arg)
{
	Params params;
	params.client_sock = ((Params*)arg)->client_sock;
	params.curpos = ((Params*)arg)->curpos;
	params.curpos.Y += 1;
	int retval;
	struct sockaddr_in clientaddr;
	char addr[INET_ADDRSTRLEN];
	int addrlen;
	DataSet data;

	// 클라이언트 정보 얻기
	addrlen = sizeof(clientaddr);
	getpeername(params.client_sock, (struct sockaddr*)&clientaddr, &addrlen);  //스레드에서 클라이언트 정보를 출력 가능하게 해줌
	inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));

	// 클라이언트와 데이터 통신
	while (1) {
		retval = recv(params.client_sock, (char*)&data.len, sizeof(int), MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;

		retval = recv(params.client_sock, data.name, 50, MSG_WAITALL);
		if (retval == SOCKET_ERROR) {
			err_display("recv()");
			break;
		}
		else if (retval == 0)
			break;

		std::ofstream out{ data.name, std::ios::binary };

		while (1)
		{
			retval = recv(params.client_sock, data.buf, BUFSIZE, MSG_WAITALL);
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			if (retval == 0)
				break;

			out.write(data.buf, retval);
			EnterCriticalSection(&cs);
			SetConsoleCursorPosition(handle, params.curpos);
			printf("전송률: %d%%\n", static_cast<int>(((float)(out.tellp()) / (float)data.len) * (float)100.0f));
			LeaveCriticalSection(&cs);
		}
	}

	// 소켓 닫기
	closesocket(params.client_sock);
	params.curpos.Y += 1;

	EnterCriticalSection(&cs);
	SetConsoleCursorPosition(handle, params.curpos);
	printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n", addr, ntohs(clientaddr.sin_port));
	LeaveCriticalSection(&cs);
	return 0;  //스레드 함수가 리턴해서 스레드 종료

}

int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;


	// 소켓 생성
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);  // IPV4 ,TCP 데이터 전송지원
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind()
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);  //INADDR_ANY: 모든 IP주소를 나타내는 상수 ,호스트 바이트 -> 네트워크 바이트 long 32bit
	serveraddr.sin_port = htons(SERVERPORT);         //포트 번호를 네트워크 바이트 순서로 변환  short 16bit
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));  //지역 ip , 포트번호 
	if (retval == SOCKET_ERROR) err_quit("bind()");

	// listen()
	retval = listen(listen_sock, SOMAXCONN);  //소켓 상태를 listen으로 변경
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	struct sockaddr_in clientaddr;
	int addrlen;
	HANDLE hThread;

	CONSOLE_SCREEN_BUFFER_INFO curInfo; // 콘솔 출력창의 정보를 담기 위해서 정의한 구조체
	GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
	COORD globalCurpos{ curInfo.dwCursorPosition.X, curInfo.dwCursorPosition.Y - 4 };

	InitializeCriticalSection(&cs);  //임계영역 사용하기전에 초기화 하기위해 호출

	while (1) {
		// accept() 클라이언트 접속을 수용하고 접손한 클라이어트와 통신할 수 있는 소켓을 생성하여 리턴
		Params clientParams;
		addrlen = sizeof(clientaddr);
		clientParams.client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
		if (clientParams.client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}


		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		globalCurpos.Y += 4;

		EnterCriticalSection(&cs);  //공유자원 사용하기전에 호출하는 함수
		SetConsoleCursorPosition(handle, globalCurpos);
		printf("[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d", addr, ntohs(clientaddr.sin_port));
		LeaveCriticalSection(&cs);  //공유자원 사용 끝나면 호출하는 함수

		clientParams.curpos = globalCurpos;

		// 스레드 생성
		hThread = CreateThread(NULL, 0, &ProcessClient, (LPVOID)&clientParams, 0, NULL);  //스레드 핸들을 리턴함
		if (hThread == NULL) { closesocket(clientParams.client_sock); }
		else { CloseHandle(hThread); }
	}

	DeleteCriticalSection(&cs);  //임계영역 사용 끝나면 호출 다 삭제 해버림
	// 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}