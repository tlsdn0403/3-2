#include "Common.h"
#define SERVERPORT 9000
#define BUFSIZE    8192

BOOL SearchFile(const char* filename)
{
    wchar_t wpath[MAX_PATH];
    // ANSI 입력이면 CP_ACP, UTF-8 입력이면 CP_UTF8 선택
    if (MultiByteToWideChar(CP_ACP, 0, filename, -1, wpath, MAX_PATH) == 0)
        return FALSE;

    WIN32_FIND_DATAW ffd;
    HANDLE hFind = FindFirstFileW(wpath, &ffd); // W 버전 호출
    if (hFind == INVALID_HANDLE_VALUE)
        return FALSE;
    FindClose(hFind);
    return TRUE;
}
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
int main()
{
    int retval;

    WSADATA wsa;
    if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
        return -1;

    SOCKET listen_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (listen_sock == INVALID_SOCKET) err_quit("socket()");

    SOCKADDR_IN serveraddr;
    ZeroMemory(&serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_port = htons(SERVERPORT);
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    retval = bind(listen_sock, (SOCKADDR*)&serveraddr, sizeof(serveraddr));
    if (retval == SOCKET_ERROR) err_quit("bind()");

    retval = listen(listen_sock, SOMAXCONN);
    if (retval == SOCKET_ERROR) err_quit("listen()");

    SOCKET client_sock;
    SOCKADDR_IN clientaddr;
    int addrlen;

    int size;
    char buf[BUFSIZE];

    while (1)
    {
        addrlen = sizeof(clientaddr);
        client_sock = accept(listen_sock, (SOCKADDR*)&clientaddr, &addrlen);
        if (client_sock == INVALID_SOCKET)
        {
            err_display("accept()");
            continue;
        }
        printf("\nFileSender 접속: IP 주소=%s, 포트 번호=%d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));

        // 파일 이름 받기
        char fileName[256];
        ZeroMemory(fileName, 256);

        // (1) 파일명 길이 수신
        retval = recvn(client_sock, (char*)&size, sizeof(int));
        if (retval == SOCKET_ERROR) { err_display("recv()"); closesocket(client_sock); continue; }
        else if (retval == 0) { closesocket(client_sock); continue; }

        // (2) 파일명 수신
        retval = recvn(client_sock, fileName, size);
        if (retval == SOCKET_ERROR) { err_display("recv()"); closesocket(client_sock); continue; }
        else if (retval == 0) { closesocket(client_sock); continue; }

        printf("-> 받을 파일 이름: %s\n", fileName);

        // 기존 파일 크기(Resume)
        int currbytes = 0;
        if (SearchFile(fileName))
        {
            FILE* fp_tmp = fopen(fileName, "rb");
            if (fp_tmp == NULL)
            {
                perror("파일 입출력 오류");
                closesocket(client_sock);
                continue;
            }
            fseek(fp_tmp, 0, SEEK_END);
            currbytes = ftell(fp_tmp);
            fclose(fp_tmp);
        }

        // (3) 받을 위치(기존 크기) 보내기
        retval = send(client_sock, (char*)&currbytes, sizeof(currbytes), 0);
        if (retval == SOCKET_ERROR)
        {
            err_display("send()");
            closesocket(client_sock);
            continue;
        }

        // (4) 이번에 받을 데이터 총 크기 수신(세션 기준)
        int totalbytes = 0;
        retval = recvn(client_sock, (char*)&totalbytes, sizeof(totalbytes));
        if (retval == SOCKET_ERROR) { err_display("recv()"); closesocket(client_sock); continue; }
        else if (retval == 0) { closesocket(client_sock); continue; }
        printf("-> 받을 데이터 크기(세션): %d bytes\n", totalbytes);

        // 파일 열기 (append)
        FILE* fp = fopen(fileName, "ab");
        if (fp == NULL)
        {
            perror("파일 입출력 오류");
            closesocket(client_sock);
            continue;
        }

        // 진행률 준비
        int numtotal = 0; // 이번 세션에서 누적 수신
        const int overall_total = currbytes + totalbytes; // 최종 파일 크기
        int prev_percent_session = -1;
        int prev_percent_overall = -1;

        // (5) 데이터 수신 루프
        while (1)
        {
            // 세션 완료 시 루프 종료
            if (numtotal >= totalbytes) break;

            // 청크 크기 수신
            retval = recvn(client_sock, (char*)&size, sizeof(int));
            if (retval == SOCKET_ERROR) { err_display("recv(size)"); break; }
            else if (retval == 0) { break; }

            // 방어: 청크가 너무 큰 경우 분할로 받기
            int remain = size;
            while (remain > 0)
            {
                int toRead = (remain > BUFSIZE) ? BUFSIZE : remain;
                int r = recvn(client_sock, buf, toRead);
                if (r == SOCKET_ERROR) { err_display("recv(chunk)"); remain = -1; break; }
                else if (r == 0) { remain = -1; break; }

                // 파일에 기록
                size_t w = fwrite(buf, 1, r, fp);
                if (w != (size_t)r || ferror(fp))
                {
                    perror("파일 입출력 오류");
                    remain = -1;
                    break;
                }

                numtotal += r;
                remain -= r;

                // 진행률 계산 및 한 줄 갱신 출력
                if (totalbytes > 0) {
                    int p_session = (int)((long long)numtotal * 100 / totalbytes);
                    int p_overall = (overall_total > 0)
                        ? (int)((long long)(currbytes + numtotal) * 100 / overall_total)
                        : 100;

                    // 너무 자주 출력하지 않도록 변경 시에만 갱신
                    if (p_session != prev_percent_session || p_overall != prev_percent_overall) {
                        printf("\r[수신률] 세션 %3d%% (%d/%d) | 전체 %3d%% (%d/%d)",
                            p_session, numtotal, totalbytes,
                            p_overall, currbytes + numtotal, overall_total);
                        fflush(stdout);
                        prev_percent_session = p_session;
                        prev_percent_overall = p_overall;
                    }
                }
            }

            if (remain < 0) break;

            // 세션 완료면 종료
            if (numtotal >= totalbytes) break;
        }

        // 100% 라인 마무리(줄바꿈)
        if (totalbytes > 0) {
            int p_session = (numtotal >= totalbytes) ? 100 : (int)((long long)numtotal * 100 / totalbytes);
            int p_overall = (overall_total > 0)
                ? (int)((long long)(currbytes + numtotal) * 100 / overall_total)
                : 100;

            printf("\r[수신률] 세션 %3d%% (%d/%d) | 전체 %3d%% (%d/%d)\n",
                p_session, numtotal, totalbytes,
                p_overall, currbytes + numtotal, overall_total);
        }

        fclose(fp);

        // 전송 결과 출력
        if (numtotal == totalbytes)
            printf("-> 파일 전송 완료!\n");
        else
            printf("-> 파일 전송 실패! (수신 %d / 기대 %d)\n", numtotal, totalbytes);

        closesocket(client_sock);
        printf("FileSender 종료: IP 주소=%s, 포트 번호=%d\n",
            inet_ntoa(clientaddr.sin_addr), ntohs(clientaddr.sin_port));
    }

    closesocket(listen_sock);
    WSACleanup();
    return 0;
}