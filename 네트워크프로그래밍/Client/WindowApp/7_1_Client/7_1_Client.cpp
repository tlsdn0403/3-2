#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기
//#include <windows.h>
//#include <tchar.h>
//#include <stdio.h>
#include "..\..\Common.h"
#include "resource.h"
#include <shobjidl.h>
#include <fstream>

char* SERVERIP = (char*)"192.168.0.3";
#define SERVERPORT 9000
#define BUFSIZE 512

#pragma pack(1)
struct DataSet {
	int len = 0;	// 데이터 길이
	char name[50] = "\0";	// 데이터 파일 이름
	char buf[BUFSIZE + 1];	// 데이터 버퍼
};
#pragma pack()

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...);
void DisplayFileName();
HANDLE hReadEvent, hWriteEvent; // 이벤트
HWND hSelectButton; // 보내기 버튼
HWND hEdit1, hEdit2; // 에디트 컨트롤
HWND hWnd;


HWND hProgress;      // 프로그레스 바 핸들

// 데이터 통신에 사용할 변수
DataSet data;
char pFilePath[500];

// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL);

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// 대화상자 생성
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_INITDIALOG:
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hSelectButton = GetDlgItem(hDlg, IDSELECT);
		return TRUE;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDSELECT:
			DisplayFileName();
			SetFocus(hEdit1); // 키보드 포커스 전환
			SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0);
			return TRUE;
		case IDOK:
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 대기
			EnableWindow(hSelectButton, FALSE); // 보내기 버튼 비활성화
			SetEvent(hWriteEvent); // 쓰기 완료 알림				
			return TRUE;
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
			return TRUE;
			// DlgProc 함수 내부
		case WM_INITDIALOG:
			hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
			hSelectButton = GetDlgItem(hDlg, IDSELECT);
			hProgress = GetDlgItem(hDlg, IDC_PROGRESS1); // 프로그레스 바 핸들 얻기

			// 프로그레스 바의 범위를 0에서 100으로 설정
			SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));
			SendMessage(hProgress, PBM_SETSTEP, (WPARAM)1, 0); // 한 단계씩 증가하도록 설정
			return TRUE;
		}
		return FALSE;
	}
	return FALSE;
}

// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...)
{
	va_list arg;
	va_start(arg, fmt);
	char cbuf[BUFSIZE * 2];
	vsprintf(cbuf, fmt, arg);
	va_end(arg);

	int nLength = GetWindowTextLength(hEdit1);
	SendMessage(hEdit1, EM_SETSEL, nLength, nLength);
	SendMessageA(hEdit1, EM_REPLACESEL, FALSE, (LPARAM)cbuf);
}

void DisplayFileName()
{
	OPENFILENAME ofn;       // common dialog box structure
	TCHAR szFile[260] = { 0 };       // if using TCHAR macros

	// Initialize OPENFILENAME
	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hWnd;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("All\0*.*\0Text\0*.TXT\0");
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetOpenFileName(&ofn) == TRUE)
	{
		// use ofn.lpstrFile
		wcstombs(pFilePath, ofn.lpstrFile, 500);
		SetWindowText(hEdit1, L"");

		char* FileName = strrchr(pFilePath, '\\');
		(FileName) ? strcpy(data.name, FileName + 1) : strcpy(data.name, pFilePath);

		DisplayText("%s\r\n", pFilePath);
	}
}

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

	while (1) {
		WaitForSingleObject(hWriteEvent, INFINITE); // 쓰기 완료 대기

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

		std::ifstream in;

		in.open(pFilePath, std::ios::binary);

		in.seekg(0, std::ios::end);
		data.len = in.tellg();
		in.seekg(0, std::ios::beg);

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

		EnableWindow(hSelectButton, TRUE); // 보내기 버튼 활성화

		in.close();

		closesocket(sock); // 소켓 닫기
		SetEvent(hReadEvent); // 읽기 완료 알림
	}

	return 0;
}