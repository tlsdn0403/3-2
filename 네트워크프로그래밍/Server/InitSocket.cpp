#include"Common.h"
int main(int argc, char* argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	const WORD major = 2;       // 메이저 버전
	const WORD minor = 2;       // 마이너 버전
	WORD version = (minor)+(major << 8);  //mahor 왼쪽으로 8칸 이동

	if (WSAStartup(version, &wsa) != 0)
		return 1;
	printf("[알림] 윈속 초기화 성공\n");
	printf("%-3s : %d.%d\n", "[윈속 실제 버전(wVersion)]", wsa.wVersion & 0xFF, (wsa.wVersion >> 8) & 0xFF);
	printf("%-3s : %d.%d\n", "[윈속 지원 최대 버전(wHighVersion)]", wsa.wHighVersion & 0xFF, (wsa.wHighVersion >> 8) & 0xFF);
	printf("%-3s : %s\n", "[윈속 설명(szDescription)]", wsa.szDescription);
	printf("%-3s : %s\n", "[윈속 시스템 상태(szSystemStatus)]", wsa.szSystemStatus);
	// 소켓 생성
	SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);   //tcp 이기 떄문에 SOCK_STREAM
	if (sock == INVALID_SOCKET) err_quit("socket()");
	printf("[알림] 소켓 생성 성공\n");

	// 소켓 닫기
	closesocket(sock);

	// 윈속 종료
	WSACleanup();
	return 0;
}