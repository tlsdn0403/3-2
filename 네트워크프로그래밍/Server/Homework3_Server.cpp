#include"Common.h"

#define SERVERPORT 9000

static int recvn(SOCKET s, char* buf, int len) {
    int received = 0;
    while (received < len) {
        int r = recv(s, buf + received, len - received, 0);
        if (r == 0) return 0;               // peer closed
        if (r == SOCKET_ERROR) return -1;    // error
        received += r;
    }
    return received;
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

int main(void) {
    int retval;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0) return 1;

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    struct sockaddr_in serveraddr;
    memset(&serveraddr, 0, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons(SERVERPORT);

    retval = bind(listen_sock, (struct sockaddr*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");
    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    struct sockaddr_in clientaddr;
    int addrlen = sizeof(clientaddr);
    SOCKET client_sock = accept(listen_sock, (struct sockaddr*)&clientaddr, &addrlen);
    if (client_sock == INVALID_SOCKET) err_quit("accept()");

    char addr[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &clientaddr.sin_addr, addr, sizeof(addr));
    printf("[SERVER] Connected: %s:%d\n", addr, ntohs(clientaddr.sin_port));

    const char* replies[3] = {
        "서버에서 첫 번째 응답입니다.",
        "서버에서 두 번째 응답입니다.",
        "서버에서 세 번째 응답입니다."
    };

    for (int round = 0; round < 3; ++round) {
        // 1) 먼저 클라이언트로부터 길이(4바이트) 수신
        uint32_t netlen = 0;
        if (recvn(client_sock, (char*)&netlen, 4) <= 0) {
            err_display("recv(length)");
            break;
        }
        uint32_t len = ntohl(netlen);
        if (len > 1 << 20) { // 1MB 초과 방어
            printf("[SERVER] Too large message: %u bytes\n", len);
            break;
        }

        // 2) 본문 수신
        char* inbuf = (char*)malloc(len + 1);
        if (!inbuf) { printf("[SERVER] OOM\n"); break; }
        if (recvn(client_sock, inbuf, (int)len) <= 0) {
            free(inbuf);
            err_display("recv(body)");
            break;
        }
        inbuf[len] = '\0';
        printf("[SERVER] (%d/3) recv: %s\n", round + 1, inbuf);

        // 3) 서버 응답 전송(길이 → 본문)
        const char* msg = replies[round];
        uint32_t slen = (uint32_t)strlen(msg);
        uint32_t snet = htonl(slen);
        if (sendn(client_sock, (const char*)&snet, 4) == -1 ||
            sendn(client_sock, msg, (int)slen) == -1) {
            free(inbuf);
            err_display("send(reply)");
            break;
        }
        printf("[SERVER] (%d/3) send: %s\n", round + 1, msg);
        free(inbuf);
    }

    closesocket(client_sock);
    closesocket(listen_sock);
    WSACleanup();
    return 0;
}