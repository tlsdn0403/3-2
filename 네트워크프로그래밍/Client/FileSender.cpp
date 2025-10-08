#include "Common.h"

char* SERVERIP = (char*)"192.168.233.49";
#define SERVERPORT 9000

static int recvn(SOCKET s, char* buf, int len) {
    int got = 0;
    while (got < len) {
        int r = recv(s, buf + got, len - got, 0);
        if (r == 0) return 0;
        if (r == SOCKET_ERROR) return -1;
        got += r;
    }
    return got;
}

static int sendn(SOCKET s, const char* buf, int len) {
    int sent = 0;
    while (sent < len) {
        int r = send(s, buf + sent, len - sent, 0);
        if (r == SOCKET_ERROR) return -1;
        sent += r;
    }
    return sent;
}

int main(int argc, char* argv[]) {
    if (argc > 1) SERVERIP = argv[1];

    // 윈속 초기화
    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    // 소켓 생성
    SOCKET sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == INVALID_SOCKET) err_quit("socket()");

    // connect()
    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    inet_pton(AF_INET, SERVERIP, &serveraddr.sin_addr);
    serveraddr.sin_port = htons(SERVERPORT);
    if (connect(sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr)) == SOCKET_ERROR)
        err_quit("connect()");

    const char* messages[3] = {
        "클라이언트 메시지 1",
        "클라이언트 메시지 2",
        "클라이언트 메시지 3"
    };

    for (int round = 0; round < 3; ++round) {
        // 1) 먼저 보냄: 길이(4바이트) → 본문
        const char* msg = messages[round];
        uint32_t len = (uint32_t)strlen(msg);
        uint32_t netlen = htonl(len);
        if (sendn(sock, (const char*)&netlen, 4) == -1 ||
            sendn(sock, msg, (int)len) == -1) {
            err_display("send(message)");
            break;
        }
        printf("[CLIENT] (%d/3) send: %s\n", round + 1, msg);

        // 2) 서버 응답 수신: 길이(4바이트) → 본문
        uint32_t rnet = 0;
        if (recvn(sock, (char*)&rnet, 4) <= 0) { err_display("recv(length)"); break; }
        uint32_t rlen = ntohl(rnet);
        if (rlen > (1u << 20)) { printf("[CLIENT] Too large: %u\n", rlen); break; }

        char* inbuf = (char*)malloc(rlen + 1);
        if (!inbuf) { printf("[CLIENT] OOM\n"); break; }
        if (recvn(sock, inbuf, (int)rlen) <= 0) { free(inbuf); err_display("recv(body)"); break; }
        inbuf[rlen] = '\0';
        printf("[CLIENT] (%d/3) recv: %s\n", round + 1, inbuf);
        free(inbuf);
    }

    closesocket(sock);
    WSACleanup();
    return 0;
}