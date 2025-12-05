#include "..\\..\\Common.h"
#include "resource.h"
#include <shobjidl.h>
#include <fstream>
#include <commctrl.h>
#pragma comment(lib, "comctl32.lib")

#define _CRT_SECURE_NO_WARNINGS // 구형 C 함수 사용 시 경고 끄기

char* SERVERIP = (char*)"172.30.1.72";
#define SERVERPORT 9000
#define BUFSIZE 512

struct DataSet {
	int len = 0;	// 데이터 길이
	char name[50] = "\0";	// 데이터 파일 이름
	char buf[BUFSIZE + 1];	// 데이터 버퍼
};


// 진행률 업데이트	
#define WM_UPDATE_PROGRESS (WM_USER + 101)

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND, UINT, WPARAM, LPARAM);
// 에디트 컨트롤 출력 함수
void DisplayText(const char* fmt, ...);
void DisplayFileName();

// 이벤트 및 윈도우 핸들
HANDLE hReadEvent, hWriteEvent; // 이벤트
HWND hSelectButton; // 보내기 버튼
HWND hEdit1, hEdit2; // 에디트 컨트롤
HWND hDlgMain = NULL; // 대화상자 핸들

HWND hProgress;      // 프로그레스 바 핸들

// 데이터 통신에 사용할 변수
DataSet data;
char pFilePath[500];

// 소켓 통신 스레드 함수
DWORD WINAPI ClientMain(LPVOID arg);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance,
	LPSTR lpCmdLine, int nCmdShow)
{
	// common controls 초기화 (프로그레스바 사용 위해)
	INITCOMMONCONTROLSEX icc;
	icc.dwSize = sizeof(icc);
	icc.dwICC = ICC_PROGRESS_CLASS;
	InitCommonControlsEx(&icc);

	// 윈속 초기화
	WSADATA wsa;
	if (WSAStartup(MAKEWORD(2, 2), &wsa) != 0)
		return 1;

	// 이벤트 생성
	hReadEvent = CreateEvent(NULL, FALSE, TRUE, NULL);  //신호로 생성 , 얘는 작업이 끝났다는 것을 의미하는 이벤트
	hWriteEvent = CreateEvent(NULL, FALSE, FALSE, NULL); // 비신호로 생성 애는 작업 시작을 의미하는 이벤트이다.

	// 소켓 통신 스레드 생성
	CreateThread(NULL, 0, ClientMain, NULL, 0, NULL);

	// 대화상자 생성 (인스턴스 핸들 , 대화상자 템플릿 id , 부모 윈도우 핸들 , 대화상자 프로시저)
	DialogBox(hInstance, MAKEINTRESOURCE(IDD_DIALOG1), NULL, DlgProc);

	// 사용 끝난 이벤트 제거
	CloseHandle(hReadEvent);
	CloseHandle(hWriteEvent);

	// 윈속 종료
	WSACleanup();
	return 0;
}

// 대화상자 프로시저
INT_PTR CALLBACK DlgProc(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam) //운영체제가 호출하는 함수
{
	switch (uMsg) {
		//대화상자 생성초기
	case WM_INITDIALOG:
		hDlgMain = hDlg; // 스레드에서 UI 갱신용으로 사용
		hEdit1 = GetDlgItem(hDlg, IDC_EDIT1);
		hSelectButton = GetDlgItem(hDlg, IDSELECT);  //전역변수에 핸들값을 저장
		hProgress = GetDlgItem(hDlg, IDC_PROGRESS1);

		// 프로그레스 바 범위 0..100
		if (hProgress) {
			SendMessage(hProgress, PBM_SETRANGE, 0, MAKELPARAM(0, 100));  //포지션
			SendMessage(hProgress, PBM_SETPOS, 0, 0);         //범위
			SendMessage(hProgress, PBM_SETSTEP, (WPARAM)1, 0);  //단계
		}
		return TRUE;

	case WM_UPDATE_PROGRESS:
	{
		int percent = (int)wParam;
		int ctrlId = (int)lParam; // IDC_PROGRESS1 혹은 IDC_PROGRESS2
		if (percent < 0) percent = 0;
		if (percent > 100) percent = 100;
		HWND hProg = GetDlgItem(hDlg, ctrlId);
		if (hProg) SendMessage(hProg, PBM_SETPOS, (WPARAM)percent, 0);
		return TRUE;
	}
	// 컨트롤에서 발생하는 메세지
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
			//파일선택
		case IDSELECT:
			DisplayFileName();
			SetFocus(hEdit1); // 키보드 포커스 전환
			SendMessage(hEdit1, EM_SETLIMITTEXT, BUFSIZE, 0); //bufzie 만큼 텍스트 길이 제한할거다
			return TRUE;
			// 전송할 파일 경로를 응용 프로그램 버퍼에 저장을 해놓음
			//전송
		case IDOK:
			EnableWindow(hSelectButton, FALSE); // 보내기 버튼 비활성화 2번 연속 x 
			WaitForSingleObject(hReadEvent, INFINITE); // 읽기 완료 대기
			SetEvent(hWriteEvent); // 파일 선택 버튼이 비활성화			
			return TRUE;
			//취소
		case IDCANCEL:
			EndDialog(hDlg, IDCANCEL);
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
	TCHAR szFile[260] = { 0 };       //  파일 경로가 저장될 버퍼

	// 구조체 0으로 초기화
	ZeroMemory(&ofn, sizeof(ofn));

	ofn.lStructSize = sizeof(ofn);
	ofn.hwndOwner = hDlgMain;
	ofn.lpstrFile = szFile;
	ofn.nMaxFile = sizeof(szFile);
	ofn.lpstrFilter = _T("All\0*.*\0Text\0*.TXT\0"); //파일 필터 어떤 파일 보이게 할건지
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
	ofn.Flags = OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;  //반드시 존재하는 경로 , 파일

	// 파일 열기 대화상자 눌렀는지	
	if (GetOpenFileName(&ofn) == TRUE)
	{
		//응용 프로그램 버퍼에 파일 주소를 저장을한다.
		wcstombs(pFilePath, ofn.lpstrFile, 500); //lpstrFile에 char* 형식으로 복사
		SetWindowText(hEdit1, L"");  //이전에있던 텍스트 지우기

		
		char* FileName = strrchr(pFilePath, '\\');//순수 파일 이름만 추출
		(FileName) ? strcpy(data.name, FileName + 1) : strcpy(data.name, pFilePath);

		DisplayText("%s\r\n", pFilePath);
	}
}

// TCP 클라이언트 시작 부분
DWORD WINAPI ClientMain(LPVOID arg)
{
	int retval;

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
	if (retval == SOCKET_ERROR) {
		err_display("connect()");
	}

	while (1) {
		WaitForSingleObject(hWriteEvent, INFINITE); // 신호 상태가 되기를 대기 -> 입력 데이터 응용 프로그램 버퍼에 저장됨


		std::ifstream in;
		in.open(pFilePath, std::ios::binary);
		if (!in.is_open()) {
			err_display("open file");
			EnableWindow(hSelectButton, TRUE); // 보내기 버튼 활성화
			SetEvent(hReadEvent);
			closesocket(sock);
			continue;
		}

		// 파일 크기 구하기
		in.seekg(0, std::ios::end);
		long long totalBytes = in.tellg();
		in.seekg(0, std::ios::beg);
		data.len = (int)totalBytes; // 프로토콜이 int라면 주의: 큰 파일은 오버플로우

		// 파일 길이와 이름 전송
		retval = send(sock, (char*)&data.len, sizeof(int), 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}
		retval = send(sock, (char*)&data.name, 50, 0);
		if (retval == SOCKET_ERROR) {
			err_display("send()");
			break;
		}

		// 프로그레스 초기화(0%)
		if (hDlgMain) PostMessage(hDlgMain, WM_UPDATE_PROGRESS, 0, (LPARAM)IDC_PROGRESS1);

		long long sentBytes = 0;
		const int blockSize = BUFSIZE;
		char buffer[BUFSIZE];

		// 파일을 블록 단위로 읽어 전송
		while (in) {
			in.read(buffer, blockSize);
			std::streamsize n = in.gcount();
			if (n <= 0) break;

			int toSend = (int)n;
			int offset = 0;
			while (toSend > 0) {
				int s = send(sock, buffer + offset, toSend, 0);
				if (s == SOCKET_ERROR) {
					err_display("send()");
					goto send_cleanup;
				}
				sentBytes += s;
				offset += s;
				toSend -= s;

				// 진행률 계산 및 UI 갱신 (PostMessage로 안전하게)
				int percent = (int)((sentBytes * 100) / (totalBytes ? totalBytes : 1));
				if (hDlgMain) PostMessage(hDlgMain, WM_UPDATE_PROGRESS, (WPARAM)percent, (LPARAM)IDC_PROGRESS1);
			}
		}

		// 완료 표시 (100%)
		if (hDlgMain) PostMessage(hDlgMain, WM_UPDATE_PROGRESS, 100, (LPARAM)IDC_PROGRESS1);

	send_cleanup:
		EnableWindow(hSelectButton, TRUE); // 보내기 버튼 활성화
		in.close();
		closesocket(sock); // 소켓 닫기
		SetEvent(hReadEvent); // 읽기 완료 알림
		// Reset hWriteEvent는 DlgProc 또는 다른 로직에서 처리
	}

	return 0;
}

//문제점 A: 데이터 덮어쓰기 (전송할 파일이 바뀌는 문제)

//사용자: '파일 A'를 선택하고 '전송' 버튼을 누릅니다. (pFilePath에 "A" 저장)
//파일 전송 스레드 : 작업을 시작하려 합니다.
//사용자 : 전송이 시작되기 전, 재빨리 '파일 선택' 버튼을 눌러 * *'파일 B' * *를 선택합니다. (pFilePath가 "B"로 덮어써짐)
//파일 전송 스레드 : 뒤늦게 pFilePath를 읽어 파일을 엽니다.이미 "B"로 바뀌었으므로, 사용자는 "A"를 보냈다고 생각하지만 실제로는 "B"가 전송됩니다.
//문제점 B : 중복 전송(똑같은 파일을 여러 번 보내는 문제)
//
//사용자 : 인터넷이 느린 것 같아 '전송' 버튼을 두 번 빠르게 누릅니다.
//파일 전송 스레드 : 첫 번째 클릭에 대한 신호를 받아 작업을 시작합니다.
//파일 전송 스레드 : 두 번째 클릭에 대한 신호를 또 받아서, 첫 번째 전송이 끝나기도 전에 똑같은 파일에 대해 두 번째 전송을 또 시작하려 합니다.이는 예기치 않은 오류를 유발합니다