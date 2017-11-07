#pragma once

#include "pch.h"


class Utils {
public:
	static String EncodeString(const String& str);

	static String ToUTF8(const WString& str);
	static String ToANSI(const WString& str);
	static WString ToUTF16(const String& str);

	static String GetExecutableName();

	static String GetExecutableFolder();

	static int DetourAttachAddress(PVOID* target, mhcode_context_handler func, PVOID* outctx, void* udata);

	static void DetourDetachAddress(PVOID* target, PVOID ctx);

	static int ReadIntBE(const void* buf);

	static void WriteIntBE(void* buf, int val);

	static void* GetModuleBaseAddress(const char* dllname);

	static void* GetFunctionAddress(const char* dllname, const char* funcname);

	//ctx, ebp+8
	static intptr_t GetMemoryValue(void* ctx, const char* exp);

	static String ToJsonFormat(intptr_t val, const String& type);

	static String GetModuleVersion(const char* dllname);

	static BOOL ExecuteOnGuiThread(PAPCFUNC func, LPARAM param);
};

