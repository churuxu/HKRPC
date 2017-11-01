#include "../src/pch.h"

#include "../src/HKHook.h"
#include "../src/HKManager.h"


static LRESULT CALLBACK MainWndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
	if (msg == WM_LBUTTONUP) {
		MessageBoxA(hwnd, "hello","world",MB_OK);
	}else if (msg == WM_RBUTTONUP) {
		MessageBoxW(hwnd, L"你好", L"世界", MB_OK);
	}
	else {
		return DefWindowProc(hwnd, msg, wp, lp);
	}
	return 1;
}


int main() {

	auto hinst = GetModuleHandle(NULL);
	WNDCLASS wc = { 0 };
	wc.lpszClassName = _T("aaaaa");
	wc.lpfnWndProc = MainWndProc;
	wc.hInstance = hinst;
	wc.hbrBackground =(HBRUSH) GetStockObject(WHITE_BRUSH);	
	RegisterClass(&wc);
	HWND wnd = CreateWindowEx(0,_T("aaaaa"), _T("click or rclick"), WS_OVERLAPPEDWINDOW, 0, 0, 300, 250, NULL, NULL, hinst, NULL);
	ShowWindow(wnd, SW_SHOW);
	UpdateWindow(wnd);
		
	//启动服务入口
	HKManager::init();
	
	MSG msg;
	while (GetMessage(&msg, NULL, 0, 0)) {
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}