#include "Common.h"              // 공용 헤더(윈속 초기화/정리, 오류 처리 함수 등 선언 가정)
#include <fstream>               // 파일 입출력(std::ofstream) 사용

#define SERVERPORT 9000          // 서버가 바인딩할 포트 번호
#define BUFSIZE    5120          // 수신 버퍼 크기(클라이언트가 보내는 데이터 청크 크기 가정)

#pragma pack(1)                  // 구조체 패딩 제거(네트워크 전송 시 정확한 바이트 정렬을 위함)
struct DataSet {
	int len;                     // 전체 데이터(파일) 길이(바이트)
	char name[50];               // 파일 이름(최대 49자 + 널 종료 가정)
	char buf[BUFSIZE];           // 수신용 버퍼
};
#pragma pack()

// 서버 코드
int main(int argc, char* argv[])
{
	int retval;

	// 윈속 초기화: WinSock2 DLL 사용을 위한 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 소켓 생성: IPv4, TCP 스트림 소켓
	SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
	if (listen_sock == INVALID_SOCKET) err_quit("socket()");

	// bind(): 서버 주소/포트 설정 후 소켓에 바인딩
	struct sockaddr_in serveraddr;
	memset(&serveraddr, 0, sizeof(serveraddr));
	serveraddr.sin_family = AF_INET;              // IPv4
	serveraddr.sin_addr.s_addr = htonl(INADDR_ANY); 
	serveraddr.sin_port = htons(SERVERPORT);      // 포트 설정
	retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
	if (retval == SOCKET_ERROR) err_quit("bind()");

	
	retval = listen(listen_sock, SOMAXCONN); // 연결 대기 상태로 전환
	if (retval == SOCKET_ERROR) err_quit("listen()");

	// 데이터 통신에 사용할 변수
	SOCKET client_sock;                 // 클라이언트 소켓
	struct sockaddr_in clientaddr;      // 접속한 클라이언트의 주소 정보
	int addrlen;

	DataSet data;                       // 길이/이름/버퍼를 담는 구조체

	while (1) {
		
		addrlen = sizeof(clientaddr);
		client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen); // 클라이언트 접속 수락
		if (client_sock == INVALID_SOCKET) {
			err_display("accept()");
			break;
		}

		// 접속한 클라이언트 정보 출력
		char addr[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
		printf("\n[TCP 서버] 클라이언트 접속: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));

		while (1) {
			retval = recv(client_sock, (char*)&data.len, sizeof(int), MSG_WAITALL); //    MSG_WAITALL: 요청한 바이트가 모두 수신될 때까지 대기
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break; 
			}
			else if (retval == 0)
				break; // 정상 종료

		
			retval = recv(client_sock, data.name, sizeof(data.name), MSG_WAITALL); 	// 파일 이름 수신
			if (retval == SOCKET_ERROR) {
				err_display("recv()");
				break;
			}
			else if (retval == 0)
				break;

		
			std::ofstream out{ data.name, std::ios::binary }; 	//  파일 열기(바이너리 모드)

			// 콘솔 커서 위치를 얻어 진행률 표시를 깔끔히 하기 위한 코드
			HANDLE handle = GetStdHandle(STD_OUTPUT_HANDLE);
			CONSOLE_SCREEN_BUFFER_INFO curInfo; // 콘솔 화면 버퍼 정보
			GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &curInfo);
			COORD pos{ curInfo.dwCursorPosition.X, curInfo.dwCursorPosition.Y };

			int percent = 0, prev_percent = -1;  // 진행률 및 이전 진행률
			while (retval)
			{
				
				retval = recv(client_sock, data.buf, BUFSIZE, MSG_WAITALL); // BUFSIZE 바이트가 모두 들어올 때까지 대기하여 수신
				if (retval == SOCKET_ERROR) {
					err_display("recv()");
					break;
				}
				
				out.write(data.buf, BUFSIZE); // 파일로 기록 버퍼 사이즈 바이트만큼 기록

				// 진행률 계산(현재 파일에 기록된 바이트 수 기준)
				percent = static_cast<int>(((float)out.tellp() / (float)data.len) * 100.0f);  //tellp()은 파일에서 쓰고 있는 현재 위치를 반환

				// 10% 단위로 출력
				if (percent % 10 == 0 && percent != prev_percent && percent < 100) {
					printf("수신중: %d%%\n", percent);
					prev_percent = percent;
				}
				// 100% 시 완료 메시지
				else if (percent == 100) {
					printf("전송률: %d%% , 전송완료 \n", percent);
				}
			}

		}

		// 소켓 닫기(클라이언트 단위)
		closesocket(client_sock);
		printf("[TCP 서버] 클라이언트 종료: IP 주소=%s, 포트 번호=%d\n",
			addr, ntohs(clientaddr.sin_port));
	}

	// 리스닝 소켓 닫기
	closesocket(listen_sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}