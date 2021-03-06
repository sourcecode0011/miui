// win32.cpp : 定义应用程序的入口点。
//
#include <windows.h>
#include <io.h>
#include <fcntl.h>
#include <stdio.h>

extern "C"
{
	extern void winKey(int code, unsigned int value);
	extern int appInit();
	extern void appDeinit();
	extern int appRun();
	extern int appRepaint(HDC hdc);
}
#ifdef __cplusplus
extern "C"{
#endif

	BOOL WINAPI CtrlHandler(DWORD CtrlType)
	{
		if (CTRL_CLOSE_EVENT == CtrlType)
		{
			FreeConsole();
		}
		return TRUE;
	}

	void OpenConsoleWindow()
	{
		AllocConsole();
		SetConsoleCtrlHandler(CtrlHandler, TRUE);

		int hCrt = _open_osfhandle((intptr_t)GetStdHandle(STD_OUTPUT_HANDLE), _O_TEXT);
		FILE * hf = _fdopen(hCrt, "w");
		*stdout = *hf;
		setvbuf(stdout, NULL, _IONBF, 0);

		// Console Window ...
		printf("Debug Message ... \n\n");
	}

	void CloseConsoleWindow()
	{
		FreeConsole();
	}

	void ShowConsoleWindow()
	{
		OpenConsoleWindow();
	}

	void HideConsoleWindow()
	{
		HWND hwnd = ::GetConsoleWindow();
		if (hwnd != NULL)
		{
			CloseConsoleWindow();
		}
	}

#ifdef __cplusplus
}
#endif

LRESULT CALLBACK WndProc(HWND, UINT, WPARAM, LPARAM);

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, PSTR szCmdLine, int iCmdShow)
{
	static TCHAR    szAppName[] = TEXT("ui");
	HWND            hwnd;
	MSG             msg;
	WNDCLASS        wndclass;

	wndclass.style = CS_HREDRAW | CS_VREDRAW;
	wndclass.lpfnWndProc = WndProc;
	wndclass.cbClsExtra = 0;
	wndclass.cbWndExtra = 0;
	wndclass.hInstance = hInstance;
	wndclass.hIcon = NULL;// LoadIcon(NULL, IDI_APPLICATION);
	wndclass.hCursor = NULL;// LoadCursor(NULL, IDC_ARROW);
	wndclass.hbrBackground = (HBRUSH)GetStockObject(BLACK_BRUSH);
	wndclass.lpszMenuName = NULL;
	wndclass.lpszClassName = szAppName;
	ShowConsoleWindow();
	if (!RegisterClass(&wndclass))
	{
		MessageBox(NULL, TEXT("This program requires windows NT!"), szAppName, MB_ICONERROR);
		return 0;
	}

	hwnd = CreateWindow(szAppName,                                      // window class name  
		TEXT("Keyboard Message Viewer #1"),             // window caption  
		WS_OVERLAPPEDWINDOW,                            // window style  
		CW_USEDEFAULT,                                  // initial x position  
		CW_USEDEFAULT,                                  // initial y position  
		1024,                                  // initial x size  
		768,                                  // initial y size  
		NULL,                                           // parent window handle  
		NULL,                                           // window menu handle  
		hInstance,                                      // program instance handle  
		NULL);                                          // creation parameters  

	ShowWindow(hwnd, iCmdShow);
	UpdateWindow(hwnd);

	

	while (GetMessage(&msg, NULL, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return msg.wParam;
}
unsigned char pic[1024 * 768 * 4] = {0};
void initpic()
{
	int i = 0;
	int j = 0;
	unsigned int *p = (unsigned int *)pic;
	for (i = 0; i < 368; i++)
		for (j = 0; j < 1024; j++)
			*p++ = 0xff000000 | 0x000000ff;//argb
}


LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam)
{
	// cxChar平均字符宽度，cyChar字符的总高度（包括外部间距），cxCaps大写字符的平均宽度  
	// 等宽字体中，cxCaps等于cxChar，变宽字体中，cxCaps等于cxChar的1.5倍  
/*	static int      cxChar, cyChar, cxClient, cyClient, cxClientMax, cyClientMax;
	static int      cLinesMax, cLines;
	static PMSG     pmsg;
	static RECT     rectScroll;
	static TCHAR szTop[] = TEXT("Message        Key       Char     ")
		TEXT("Repeat Scan Ext ALT Prev Tran");
	static TCHAR szUnd[] = TEXT("_______        ___       ____     ")
		TEXT("______ ____ ___ ___ ____ ____");

	static TCHAR*   szFormat[2] = {
		TEXT("%-13s %3d %-15s%c%6u %4d %3s %3s %4s %4s"),
		TEXT("%-13s            0x%04X%1s%c %6u %4d %3s %3s %4s %4s") };
	static TCHAR*   szYes = TEXT("Yes");
	static TCHAR*   szNo = TEXT("No");
	static TCHAR*   szDown = TEXT("Down");
	static TCHAR*   szUp = TEXT("Up");

	static TCHAR*   szMessage[] = {
		TEXT("WM_KEYDOWN"), TEXT("WM_KEYUP"),
		TEXT("WM_CHAR"), TEXT("WM_DEADCHAR"),
		TEXT("WM_SYSKEYDOWN"), TEXT("WM_SYSKEYUP"),
		TEXT("WM_SYSCHAR"), TEXT("WM_SYSDEADCHAR") };
		*/
	HDC         hdc;
	int         i, iType;
	PAINTSTRUCT ps;
	//TCHAR       szBuffer[128], szKeyName[32];
	//TEXTMETRIC  tm;
	//printf("mesg %d \n", message);
	switch (message)
	{
	case WM_CREATE:
		appInit();
		appRun();
		break;
	case WM_DISPLAYCHANGE:
	//	cxClientMax = GetSystemMetrics(SM_CXMAXIMIZED);
	//	cyClientMax = GetSystemMetrics(SM_CYMAXIMIZED);
		//initpic();
	/*	hdc = GetDC(hwnd);

		SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
		GetTextMetrics(hdc, &tm);       // 获取系统默认字体的尺寸  
		cxChar = tm.tmAveCharWidth;
		cyChar = tm.tmHeight;

		ReleaseDC(hwnd, hdc);

		if (pmsg)
			free(pmsg);

		cLinesMax = cyClientMax / cyChar;
		pmsg = (PMSG)malloc(cLinesMax * sizeof(MSG));
		cLines = 0;*/

		//return 0;  
		break;
	case WM_SIZE:
	/*	if (message == WM_SIZE)
		{
			cxClient = LOWORD(lParam);
			cyClient = HIWORD(lParam);
		}

		rectScroll.left = 0;
		rectScroll.right = cxClient;
		rectScroll.top = cyChar;
		rectScroll.bottom = cyChar * (cyClient / cyChar);*/

		InvalidateRect(hwnd, NULL, TRUE);

		break;

	case WM_KEYDOWN:
			winKey(wParam, 1);
			break;
	case WM_KEYUP:
		winKey(wParam, 0);
		
		break;
	case WM_CHAR:
	case WM_DEADCHAR:
	case WM_SYSKEYDOWN:
	case WM_SYSKEYUP:
	case WM_SYSCHAR:
	case WM_SYSDEADCHAR:
	/*	for (i = cLinesMax - 1; i > 0; i--)
		{
			pmsg[i] = pmsg[i - 1];
		}

		pmsg[0].hwnd = hwnd;
		pmsg[0].message = message;
		pmsg[0].wParam = wParam;
		pmsg[0].lParam = lParam;

		cLines = min(cLines + 1, cLinesMax);

		ScrollWindow(hwnd, 0, -cyChar, &rectScroll, &rectScroll);*/
		break;


	case WM_PAINT:
		hdc = BeginPaint(hwnd, &ps);
		appRepaint(hdc);
	/*	SelectObject(hdc, GetStockObject(SYSTEM_FIXED_FONT));
		SetBkMode(hdc, TRANSPARENT);
		TextOut(hdc, 0, 0, szTop, lstrlen(szTop));
		TextOut(hdc, 0, 0, szUnd, lstrlen(szUnd));

		for (i = 0; i < min(cLines, cyClient / cyChar - 1); i++)
		{
			iType = pmsg[i].message == WM_CHAR ||
				pmsg[i].message == WM_SYSCHAR ||
				pmsg[i].message == WM_DEADCHAR ||
				pmsg[i].message == WM_SYSDEADCHAR;

			GetKeyNameText(pmsg[i].lParam, szKeyName, sizeof(szKeyName) / sizeof(TCHAR));

			TextOut(hdc, 0, (cyClient / cyChar - 1 - i) * cyChar, szBuffer,
				wsprintf(szBuffer, szFormat[iType],
				szMessage[pmsg[i].message - WM_KEYFIRST],
				pmsg[i].wParam,
				(PTSTR)(iType ? TEXT(" ") : szKeyName),
				(TCHAR)(iType ? pmsg[i].wParam : ' '),
				LOWORD(pmsg[i].lParam),
				HIWORD(pmsg[i].lParam) & 0xFF,
				0x01000000 & pmsg[i].lParam ? szYes : szNo,
				0x20000000 & pmsg[i].lParam ? szYes : szNo,
				0x40000000 & pmsg[i].lParam ? szDown : szUp,
				0x80000000 & pmsg[i].lParam ? szUp : szDown));
		}
		*/

		EndPaint(hwnd, &ps);
		break;

	case WM_DESTROY:
		appDeinit();
		HideConsoleWindow();
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, message, wParam, lParam);
}