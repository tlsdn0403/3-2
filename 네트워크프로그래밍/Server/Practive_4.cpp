#include "Common.h"


// 도메인 이름 -> IPv4 주소
void PrinIPv4AddrsAndAliases(const char* domain)
{
	struct hostent* he = gethostbyname(domain);								// hostent 구조체 안 값을 입력한채로 반환
	if (he == NULL) {
		err_display("gethostbyname()");
		return;
	}

	if (he->h_addrtype != AF_INET) {										// h_addrtype 멤버가 AF_INET이 아니면 IPv4 주소가 아니다	
		printf("IPv4가 아닌 결과입니다. addrtype=%d\n", he->h_addrtype);
		return;
	}

	printf("IPv4 주소들:\n");
	if (he->h_addr_list && he->h_addr_list[0]) {							// hostent 구조체의 h_addr_list 멤버에 ipv4 주소들이 들어있다
		char buf[INET_ADDRSTRLEN];											// INET_ADDRSTRLEN :문자열 형태의 IPv4 주소 최대 길이를 나타내느 상수
		for (int i = 0; he->h_addr_list[i]; i++) {
			//numeric to presentation (주소체계 , 주소, 문자열 버퍼, 버퍼 크기)
			inet_ntop(AF_INET, he->h_addr_list[i], buf, sizeof(buf));		// 32비트 숫자 형태의 IPv4 주소를 10진수로 된 문자열 형태로 변환
			printf("  %s\n", buf);
		}
	}
	else {
		printf("IPv4 주소가 없습니다. \n");
	}
	printf("별명(aliases):\n");
	// h_aliases 맴버에 별명들을 저장
	if (he->h_aliases && he->h_aliases[0]) {
		for (int i = 0; he->h_aliases[i]; i++) {
			printf("  %s\n", he->h_aliases[i]);

		}
	}
	else {
		printf("별명이 없습니다 \n");
	}
}

int main(int argc, char* argv[])
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// argc는 명령행 입력의 개수
	//argv[0]은 기본적으로 프로그램 실행 파일이므로 , argv[1]부터 실제 입력값
	if (argc != 2) {
		WSACleanup();
		return 1;
	}
	const char* domain = argv[1];
	printf("도메인 이름(변환 전) = %s\n", domain);

	PrinIPv4AddrsAndAliases(domain);
	// 윈속 종료
	WSACleanup();
	return 0;
}