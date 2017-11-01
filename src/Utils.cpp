
#include "Utils.h"
#include <tlhelp32.h>


String Utils::EncodeString(const String& str) {
	String result;
	result.reserve(str.length() + 16);
	const char* pstr = str.c_str();
	unsigned char ch = *pstr;
	while (ch) {
		switch (ch) {
			//转义字符替换
		case '\\': result += "\\\\"; break;
		case '\"': result += "\\\""; break;
		case '\r': result += "\\r"; break;
		case '\n': result += "\\n"; break;
		case '\t': result += "\\t"; break;
		case '\f': result += "\\f"; break;
		case '\b': result += "\\b"; break;
		default:
			//ascii 32以下特殊字符替换
			if (ch<32) {
				char buf[8]; buf[0] = 0;
				snprintf(buf, 8, "\\u00%.2x", (int)ch);
				result += buf;
			}
			else {
				result += ch;
			}
			break;
		}
		pstr++;
		ch = *pstr;
	}
	return result;
}


static String WCS2MBS(const wchar_t* str, int len, int codepage= CP_UTF8) {
	String result;
	if (len>0) {
		int buflen = len * 3 + 1;
		std::unique_ptr<char[]> buf(new char[buflen]);
		int ret = ::WideCharToMultiByte(codepage, 0, str, len, buf.get(), buflen, NULL, NULL);
		if (ret>0) {
			result.assign(buf.get(), ret);
		}
		else {
			//Win32ThrowLast("WideCharToMultiByte");
		}
	}
	return result;
}

static WString MBS2WCS(const char* str, int len) {
	WString result;
	if (len>0) {
		int buflen = len + 1;
		std::unique_ptr<wchar_t[]> buf(new wchar_t[buflen]);
		int ret = ::MultiByteToWideChar(CP_UTF8, 0, str, len, buf.get(), buflen);
		if (ret>0) {
			result.assign(buf.get(), ret);
		}
		else {
			//Win32ThrowLast("MultiByteToWideChar");
		}
	}
	return result;
}

String Utils::ToANSI(const WString& str) {
	return ::WCS2MBS(str.c_str(), str.length(),CP_ACP);
}

String Utils::ToUTF8(const WString& str) {
	return ::WCS2MBS(str.c_str(), str.length(),CP_UTF8);
}

WString Utils::ToUTF16(const String& str) {
	return ::MBS2WCS(str.c_str(), str.length());
}



int Utils::DetourAttachAddress(PVOID* target, mhcode_context_handler func, PVOID* outctx, void* udata) {
	LONG ret = 0;
	int offset = 0;
	PDETOUR_TRAMPOLINE trampoline = NULL;
	void* code = mhcode_malloc(64);
	offset = mhcode_make_context_handler(code, func, udata);
	ret = DetourAttachEx(target, code, &trampoline, NULL, NULL);
	if (ret)return ret;
	mhcode_make_jmp((char*)code + offset, trampoline);
	*outctx = code;
	return 0;
}

void Utils::DetourDetachAddress(PVOID* target, PVOID ctx) {
	DetourDetach(target, ctx);
	mhcode_free(ctx);
}


String Utils::GetExecutableName() {
	WString wresult;
	WCHAR buf[MAX_PATH];
	int ret = GetModuleFileName(NULL, buf, MAX_PATH);
	if (ret>0) {
		WString wfile(buf, ret);
		int idx = wfile.rfind('\\');
		wresult = wfile.substr(idx + 1);
	}
	else {
		wresult = L"";
	}
	return Utils::ToUTF8(wresult);
}

String Utils::GetExecutableFolder() {
	WString wresult;
	WCHAR buf[MAX_PATH];
	int ret = GetModuleFileName(NULL, buf, MAX_PATH);
	if (ret>0) {
		WString wfile(buf, ret);
		int idx = wfile.rfind('\\');
		wresult = wfile.substr(0, idx);
	}
	else {
		wresult = L".";
	}
	return Utils::ToUTF8(wresult);

}


int Utils::ReadIntBE(const void* buf) {
	unsigned int i = *(int*)buf;
	unsigned int j;
	j = (i << 24);
	j += (i << 8) & 0x00FF0000;
	j += (i >> 8) & 0x0000FF00;
	j += (i >> 24);
	return j;	
}

void Utils::WriteIntBE(void* vbuf, int val) {
	unsigned char* buf = (unsigned char*)vbuf;
	buf[0] = (val & 0xff000000) >> 24;
	buf[1] = (val & 0x00ff0000) >> 16;
	buf[2] = (val & 0x0000ff00) >> 8;
	buf[3] = (val & 0x000000ff) ;
}


void* Utils::GetModuleBaseAddress(const char* dllname) {
	if (!dllname)return NULL;
	HANDLE hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (!hModuleSnap)return NULL;
	WString wdllname = Utils::ToUTF16(dllname);
	MODULEENTRY32 me32;
	me32.dwSize = sizeof(MODULEENTRY32);
	if (!Module32First(hModuleSnap, &me32)) {
		CloseHandle(hModuleSnap);
		return(FALSE);
	}

	do {
		if (_wcsicmp(me32.szModule, wdllname.c_str()) == 0) {
			CloseHandle(hModuleSnap);
			return me32.modBaseAddr;
		}
	} while (Module32Next(hModuleSnap, &me32));
	CloseHandle(hModuleSnap);
	return NULL;
}

void* Utils::GetFunctionAddress(const char* dllname, const char* funcname) {
	HMODULE mod = GetModuleHandleA(dllname);
	if (mod) {		
		return GetProcAddress(mod, funcname);
	}
	return NULL;
}


String Utils::ToJsonFormat(intptr_t val, const String& type) {
	String ret;	
	if (_stricmp(type.c_str(),"string")==0) {
		ret += "\"";
		if (val) {
			ret.append(Utils::EncodeString((char*)val));
		}
		ret += "\"";
	}
	else if (_stricmp(type.c_str(), "wstring") == 0) {
		ret += "\"";
		if (val) {
			ret.append(Utils::EncodeString(Utils::ToUTF8((wchar_t*)val)));
		}
		ret += "\"";
	}
	else if (_stricmp(type.c_str(), "float") == 0) {
		float* pv = (float*)&val;
		ret += std::to_string(*pv);
	}
	else if (_stricmp(type.c_str(), "double") == 0) {
		double* pv = (double*)&val;
		ret += std::to_string(*pv);
	}
	else {
		ret += std::to_string(val);
	}
	return ret;
}

String Utils::GetModuleVersion(const char* dllname) {
	String result;
	WString wname = Utils::ToUTF16(dllname);
	DWORD len = GetFileVersionInfoSize(wname.c_str(), NULL);
	if (len) {
		void* buf = malloc(len);
		if (buf) {
			if (GetFileVersionInfo(wname.c_str(), NULL, len, buf)) {
				VS_FIXEDFILEINFO* info;
				UINT outlen = sizeof(info);
				if (VerQueryValue(buf, _T("\\"), (PVOID*)&info, &outlen)) {
					int major = (info->dwFileVersionMS >> 16) & 0x0000FFFF;
					int minor = info->dwFileVersionMS & 0x0000FFFF;
					int build = (info->dwFileVersionLS >> 16) & 0x0000FFFF;
					int revision = info->dwFileVersionLS & 0x0000FFFF;
					
					result += std::to_string(major);
					result += ".";
					result += std::to_string(minor);
					result += ".";
					result += std::to_string(build);
					result += ".";
					result += std::to_string(revision);					
				}
			}
			free(buf);
		}
	}	
	return result;
}


struct EnumParam {
	int pid;
	HWND hwnd;
};
static BOOL CALLBACK EnumProcessWindowsProc(HWND hwnd, LPARAM lParam) {
	DWORD dwpid = 0;
	BOOL continueenum = TRUE;
	EnumParam* param = (EnumParam*)lParam;
	DWORD tpid = param->pid;
	GetWindowThreadProcessId(hwnd, &dwpid);
	if (tpid == dwpid) {
		param->hwnd = hwnd;
		continueenum = FALSE;
	}
	return continueenum;
}

static HWND FindProcessWinodw(int pid) {
	EnumParam param;
	param.pid = pid;
	param.hwnd = NULL;
	EnumWindows(EnumProcessWindowsProc, (LPARAM)&param);
	return param.hwnd;
}

struct ExecParam {
	PAPCFUNC func;
	LPARAM param;
};

static VOID  CALLBACK TimerCallback(HWND hwnd, UINT msg, UINT_PTR id, DWORD tick) {
	BOOL ret = KillTimer(hwnd, id);
	ExecParam* p = (ExecParam*)id;
	p->func(p->param);
	delete p;
}

BOOL Utils::ExecuteOnGuiThread(PAPCFUNC func, LPARAM param) {
	if (!func)return FALSE;
	HWND wnd = FindProcessWinodw(GetCurrentProcessId());
	DWORD pid = 0;
	DWORD thid = GetWindowThreadProcessId(wnd, &pid);
	//TCHAR buf[256];
	//GetWindowText(wnd, buf, 256);
	if (wnd) {
		ExecParam* p = new ExecParam;
		p->func = func;
		p->param = param;
		int tid = SetTimer(wnd, (UINT_PTR)p, 0, TimerCallback);
		if (tid) {
			return 1;
		}
		/*
		if (PostThreadMessage(thid, 0xf0f0f, (WPARAM)p, (LPARAM)TimerCallback)) {
		return 1;
		}*/
		int a = GetLastError();
		delete p;
	}
	//Win32Throw(GetLastError(), "ExecuteOnMainThread");
	return FALSE;
}
