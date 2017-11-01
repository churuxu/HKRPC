#include "pch.h"
#include "HKManager.h"

#include "Utils.h"

#ifndef EXE_FILTER
#define EXE_FILTER ""
#endif

static void  CALLBACK Init(ULONG_PTR arg) {
	HKManager::init();
	printf("Threadid:%d\n", GetCurrentThreadId());
}

static BOOL IsFilterExecutable() {
	CHAR name[MAX_PATH];
	name[0] = 0;
	GetModuleFileNameA(NULL, name, MAX_PATH);
	if (strlen(EXE_FILTER)) {
		if (!strstr(name, EXE_FILTER))return FALSE;
	}
	return TRUE;
}

BOOL WINAPI DllMain(HINSTANCE hinstDLL,DWORD fdwReason,LPVOID lpReserved) {	
	static bool inited = false;
	switch (fdwReason){
	case DLL_PROCESS_ATTACH:
		if (!IsFilterExecutable())return FALSE;
		DisableThreadLibraryCalls(hinstDLL);		
		if (!Utils::ExecuteOnGuiThread(Init, NULL)) {
			Init(NULL);
		}
		break;
	case DLL_THREAD_ATTACH:			
		break;
	case DLL_THREAD_DETACH:	
		break;
	case DLL_PROCESS_DETACH:		
		break;
	}
	return TRUE;
}
