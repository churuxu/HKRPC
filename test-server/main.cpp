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

template <class T>
struct is_string :public std::integral_constant<int, 0> {};

template <>
struct is_string<const char*> :public std::integral_constant<int, 1> {};

template <>
struct is_string<std::string> :public std::integral_constant<int, 1> {};

template <>
struct is_string<char[]> :public std::integral_constant<int, 1> {};

template <typename T>
void showarg(T t) {
	printf("is_string:%d  ", is_string<T>::value);
}

template <class T>
void testvarg(T t) {
	showarg(t);
}

template <class T, class... ARGS>
void testvarg(T t, ARGS... arg) {
	showarg(t);
	testvarg(arg...);	
}

template <class T>
size_t memlen(T t) {
	return sizeof(T);
}

size_t memlen(const char* str) {
	return strlen(str) + 1;
}

size_t memlen(const wchar_t* str) {
	return wcslen(str) + sizeof(wchar_t);
}

template <class T, class... A>
size_t memlen(T t, A... a) {
	size_t ret = memlen(t);
	return ret + memlen(a...);
}


void test() {
	printf("%d\n", memlen(123));
	printf("%d\n", memlen("hello"));
	printf("%d\n", memlen("hello", "world"));
}


int main() {
	test();

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