#include "Common.h"      // 공용 오류 처리 및 유틸 함수(예: err_quit, err_display) 선언부
#include <Windows.h>     // WinSock2를 포함한 윈도우 네트워크/시스템 API
#include <string>
#include <fstream>

// 서버 접속 설정
char* SERVERIP = (char*)"172.30.1.26";
#define SERVERPORT 9000                
#define BUFSIZE 5120                   

// 패딩 없이 구조체를 메모리에 배치
#pragma pack(1)
// 전송 및 보조 데이터 구조체(참고용)
// - 실제 전송은 len과 name을 별도 전송 후, 파일 바디는 buf를 반복 전송
struct DataSet {
    int len = 0;               // 전체 데이터(파일) 길이(바이트). 호스트 바이트 순서 그대로 전송됨(동일 플랫폼 가정).
    char name[50] = "\0";      // 파일 이름(고정 50바이트 전송)
    char buf[BUFSIZE] = "\0";  // 파일 데이터 버퍼(고정 크기 블록 전송에 사용)
};
#pragma pack()


int main(int argc, char* argv[])
{
    int retval = 0;

    // 인자 체크: 전송할 파일 경로 1개 필요
    if (argc != 2)
        return 1;

    // 윈속 초기화(WinSock 2.2)
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return 1;

    // TCP 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // 서버 주소 설정 및 connect()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr); // 문자열 IP -> 네트워크 바이트 순서의 이진 주소
    serveraddr.sin_port = htons(SERVERPORT);            // 포트는 네트워크 바이트 순서로 변환
    retval = connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("connect()");

    // 데이터 통신에 사용할 변수들
    DataSet data;
    // 바이너리 모드로 파일 열기
    std::ifstream in{ argv[1], std::ios::binary };
    if (!in)
    {
        printf("파일 열기 오류\n");
        return 1;
    }

    // 파일 크기 계산: 끝으로 이동 -> 위치값(tellg) -> 처음으로 다시 이동
    in.seekg(0, std::ios::end);
    data.len = in.tellg(); // 전송할 전체 바이트 수
    in.seekg(0, std::ios::beg);

    // 파일 이름 채우기
    char* pFileName = argv[1];
    (pFileName) ? strcpy(data.name, pFileName + 1) : strcpy(data.name, argv[1]);

    // 파일 길이 전송
    retval = send(sock, (char*)&data.len, sizeof(int), 0);
    if (retval == SOCKET_ERROR) {
        err_display("send()");
    }

    // 파일 이름 전송
    retval = send(sock, data.name, sizeof(data.name), 0);
    if (retval == SOCKET_ERROR) {
        err_display("send()");
    }

    // 파일 본문 전송
    while (in.read(data.buf, BUFSIZE))
    {
        // BUFSIZE만큼 읽혔을 때마다 전송
        retval = send(sock, data.buf, BUFSIZE, 0);
        if (retval == SOCKET_ERROR) {
            err_display("send()");
            break;
        }
    }

    // 마지막 남은 바이트(파일 크기 % BUFSIZE) 전송
    retval = send(sock, data.buf, data.len % BUFSIZE, 0);
    if (retval == SOCKET_ERROR) {
        err_display("send()");
    }

    // 전송 결과 출력 (길이 정보 4바이트 + 파일 데이터 길이)
    printf("[TCP 클라이언트] %d+%d바이트를 "
        "보냈습니다.\n", (int)sizeof(int), data.len);

    // 소켓 닫기
    closesocket(sock);

    // 윈속 종료
    WSACleanup();
    return 0;
}