//----------------------------------------------------
//
// SA:MP Multiplayer Modification For GTA:SA
// Copyright 2004-2005 SA:MP team
//
// Version: $Id: exceptions.cpp,v 1.11 2006/05/08 17:35:55 kyeman Exp $
//
//----------------------------------------------------

#include "main.h"
#include "resource.h"
#include <Tlhelp32.h>

extern HINSTANCE hInstance;
extern DWORD dwScmOpcodeDebug;
extern bool bScmLocalDebug;
extern GAME_SCRIPT_THREAD* gst;

static PEXCEPTION_POINTERS pExceptionPtrs = NULL;
static char szCrashInfoFile[50] = { 0 };
static DWORD dwExcWarningCount = 0;

// turns a raw address into module+offset, a bare address is useless in a report
// because the relocated bases differ on every run
static const char* ResolveAddress(PVOID pAddress, char* szOut, size_t nOutSize)
{
	HANDLE hSnap;
	MODULEENTRY32 me32;
	BYTE* pTarget = (BYTE*)pAddress;

	strcpy_s(szOut, nOutSize, "unknown module");

	hSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());
	if (hSnap == INVALID_HANDLE_VALUE)
		return szOut;

	me32.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(hSnap, &me32) == TRUE) {
		do {
			if (pTarget >= me32.modBaseAddr &&
				pTarget < (me32.modBaseAddr + me32.modBaseSize))
			{
				sprintf_s(szOut, nOutSize, "%s+0x%X", me32.szModule,
					(unsigned int)(pTarget - me32.modBaseAddr));
				break;
			}
		} while (Module32Next(hSnap, &me32));
	}

	CloseHandle(hSnap);
	return szOut;
}

// something writes a float into CPedIntelligence::m_pPed and gta faults reading it
// back, so watch the dword with a debug register and log whoever writes it
static PVOID pWatchHandler = NULL;
static DWORD dwWatchAddress = 0;
static DWORD dwWatchExpect = 0;

static void DisarmWatch()
{
	CONTEXT ctx;

	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if (GetThreadContext(GetCurrentThread(), &ctx)) {
		ctx.Dr0 = 0;
		ctx.Dr7 = 0;
		SetThreadContext(GetCurrentThread(), &ctx);
	}
	if (pWatchHandler) {
		RemoveVectoredExceptionHandler(pWatchHandler);
		pWatchHandler = NULL;
	}
}

static LONG CALLBACK WatchHandler(PEXCEPTION_POINTERS pExc)
{
	char szModule[MAX_PATH + 32];
	FILE* f;

	if (pExc->ExceptionRecord->ExceptionCode != EXCEPTION_SINGLE_STEP)
		return EXCEPTION_CONTINUE_SEARCH;

	// clear the trap bit or the same write keeps re-raising, and stay armed for
	// writes that put the ped back where it belongs
	pExc->ContextRecord->Dr6 = 0;

	if (*(DWORD*)dwWatchAddress == dwWatchExpect)
		return EXCEPTION_CONTINUE_EXECUTION;

	if (fopen_s(&f, "samp-watchpoint.txt", "w") == 0) {
		fprintf_s(f, "write to 0x%08X from 0x%08X (%s)\n"
			"EAX: 0x%08X\tEBX: 0x%08X\tECX: 0x%08X\tEDX: 0x%08X\n"
			"ESI: 0x%08X\tEDI: 0x%08X\tEBP: 0x%08X\tESP: 0x%08X\n"
			"now holds 0x%08X\n\nStack:\n",
			dwWatchAddress, pExc->ContextRecord->Eip,
			ResolveAddress((PVOID)pExc->ContextRecord->Eip, szModule, sizeof(szModule)),
			pExc->ContextRecord->Eax, pExc->ContextRecord->Ebx,
			pExc->ContextRecord->Ecx, pExc->ContextRecord->Edx,
			pExc->ContextRecord->Esi, pExc->ContextRecord->Edi,
			pExc->ContextRecord->Ebp, pExc->ContextRecord->Esp,
			*(DWORD*)dwWatchAddress);

		for (int i = 0; i < 0x40; i += 4) {
			DWORD dwSlot = *(DWORD*)(pExc->ContextRecord->Esp + i);
			fprintf_s(f, "+%04X: 0x%08X  %s\n", i, dwSlot,
				ResolveAddress((PVOID)dwSlot, szModule, sizeof(szModule)));
		}
		fclose(f);
	}

	DisarmWatch();
	return EXCEPTION_CONTINUE_EXECUTION;
}

// one shot, the field is only written legitimately by the CPedIntelligence ctor so
// anything caught after this point is the bug
void ArmPedIntelligenceWatch(DWORD dwAddress)
{
	CONTEXT ctx;

	if (dwWatchAddress || !dwAddress)
		return;

	dwWatchAddress = dwAddress;
	dwWatchExpect = *(DWORD*)dwAddress;
	pWatchHandler = AddVectoredExceptionHandler(1, WatchHandler);

	ctx.ContextFlags = CONTEXT_DEBUG_REGISTERS;
	if (!GetThreadContext(GetCurrentThread(), &ctx))
		return;

	ctx.Dr0 = dwAddress;
	// L0, and for slot 0 RW=01 (write) with LEN=11 (four bytes)
	ctx.Dr7 = (ctx.Dr7 & ~0xFUL) | 1UL | (1UL << 16) | (3UL << 18);
	ctx.Dr6 = 0;
	SetThreadContext(GetCurrentThread(), &ctx);
}

static void DumpLoadedModules(FILE* f)
{
	HANDLE hModuleSnap;
	MODULEENTRY32 me32;

	hModuleSnap = CreateToolhelp32Snapshot(TH32CS_SNAPMODULE, GetCurrentProcessId());

	fputs("\nLoaded Modules:\n", f);
	fflush(f);

	if (hModuleSnap == INVALID_HANDLE_VALUE) {
		fputs("-FailedCreate-\n", f);
		fflush(f);
		return;
	}

	me32.dwSize = sizeof(MODULEENTRY32);

	if (Module32First(hModuleSnap, &me32) == FALSE) {
		fputs("-FailedFirst-\n", f);
		fflush(f);
		CloseHandle(hModuleSnap);
		return;
	}
	
	do {
		fprintf_s(f, "%s\tB: 0x%p\tS: 0x%08X\t(%s)\n",
			me32.szModule, me32.modBaseAddr, me32.modBaseSize, me32.szExePath);
		fflush(f);
	} while (Module32Next(hModuleSnap, &me32));

	CloseHandle(hModuleSnap);
}

static void DumpMemory(FILE* f, BYTE* pData, DWORD dwCount, BOOL bAsDWords = FALSE)
{
	if (bAsDWords) {
		for (DWORD i = 0; i < dwCount; i += 16) {
			fprintf_s(f, "+%04X: 0x%08X 0x%08X 0x%08X 0x%08X\n", i,
				*(DWORD*)(pData + i + 0), *(DWORD*)(pData + i + 4),
				*(DWORD*)(pData + i + 8), *(DWORD*)(pData + i + 12));
			fflush(f);
		}
	} else {
		for (DWORD i = 0; i < dwCount; i += 16) {
			fprintf_s(f, "+%04X: %02X %02X %02X %02X  %02X %02X %02X %02X  "
				"%02X %02X %02X %02X  %02X %02X %02X %02X\n", i,
				pData[i + 0], pData[i + 1], pData[i + 2], pData[i + 3],
				pData[i + 4], pData[i + 5], pData[i + 6], pData[i + 7],
				pData[i + 8], pData[i + 9], pData[i + 10], pData[i + 11],
				pData[i + 12], pData[i + 13], pData[i + 14], pData[i + 15]);
			fflush(f);
		}
	}
}

static BOOL IsReadable(const void* p, DWORD dwSize)
{
	MEMORY_BASIC_INFORMATION mbi;

	if (!p || VirtualQuery(p, &mbi, sizeof(mbi)) == 0)
		return FALSE;
	if (mbi.State != MEM_COMMIT || (mbi.Protect & PAGE_NOACCESS) || (mbi.Protect & PAGE_GUARD))
		return FALSE;

	return ((BYTE*)p + dwSize) <= ((BYTE*)mbi.BaseAddress + mbi.RegionSize);
}

// CPools keeps its pool pointers in one table, and a pointer that should be a pool
// slot but isn't tells us the difference between corruption and a stale pointer
static void DumpGamePools(FILE* f)
{
	static const struct { DWORD dwPtr; const char* szName; } pools[] = {
		{ 0xB74484, "PtrNodeSingle"   }, { 0xB74488, "PtrNodeDouble"  },
		{ 0xB7448C, "EntryInfoNode"   }, { 0xB74490, "Ped"            },
		{ 0xB74494, "Vehicle"         }, { 0xB74498, "Building"       },
		{ 0xB7449C, "Object"          }, { 0xB744A0, "Dummy"          },
		{ 0xB744A4, "ColModel"        }, { 0xB744A8, "Task"           },
		{ 0xB744AC, "Event"           }, { 0xB744C0, "PedIntelligence"},
		{ 0xB744C4, "PedAttractor"    },
	};

	fputs("\nGame Pools:\n", f);

	for (int i = 0; i < (int)(sizeof(pools) / sizeof(pools[0])); i++) {
		DWORD* pPool = *(DWORD**)pools[i].dwPtr;

		if (!IsReadable(pPool, 0x14)) {
			fprintf_s(f, "%-16s P: 0x%08X\t-unreadable-\n", pools[i].szName, (DWORD)pPool);
			continue;
		}
		fprintf_s(f, "%-16s P: 0x%08X\tO: 0x%08X\tM: 0x%08X\tN: %d\tF: %d\n",
			pools[i].szName, (DWORD)pPool, pPool[0], pPool[1], pPool[2], pPool[3]);
	}
	fflush(f);
}

static void DumpLocalPed(FILE* f)
{
	DWORD dwPed = *(DWORD*)0xB7CD98;

	fputs("\nLocal Ped:\n", f);

	if (!IsReadable((void*)dwPed, 0x7A4)) {
		fprintf_s(f, "CWorld::Players[0].m_pPed: 0x%08X\t-unreadable-\n", dwPed);
		fflush(f);
		return;
	}

	DWORD dwIntel = *(DWORD*)(dwPed + 0x47C);

	fprintf_s(f, "ped: 0x%08X\tvtbl: 0x%08X\ttype: %d\tintelligence: 0x%08X\n",
		dwPed, *(DWORD*)dwPed, *(DWORD*)(dwPed + 0x598), dwIntel);

	if (IsReadable((void*)dwIntel, 0x40)) {
		fprintf_s(f, "intelligence->m_pPed: 0x%08X\nintelligence dump:\n",
			*(DWORD*)dwIntel);
		DumpMemory(f, (BYTE*)dwIntel, 0x40, TRUE);
	} else {
		fputs("intelligence -unreadable-\n", f);
	}
	fflush(f);
}

static void DumpMain()
{
	SYSTEMTIME t;
	FILE* f;
	errno_t err;

	GetLocalTime(&t);
	t.wYear %= 100;
	sprintf_s(szCrashInfoFile, "samp-crashinfo-%02d%02d%02d%02d%02d%02d.txt",
		t.wDay, t.wMonth, t.wYear, t.wHour, t.wMinute, t.wSecond);

	err = fopen_s(&f, szCrashInfoFile, "w");
	if (err == 0) {
		DWORD* pdwStack;
		
		char szFaultModule[MAX_PATH + 32];

		fprintf_s(f, "SA-MP " SAMP_VERSION " (" __DATE__ " " __TIME__ ")\n"
			"Exception At Address: 0x%p (%s)\n",
			pExceptionPtrs->ExceptionRecord->ExceptionAddress,
			ResolveAddress(pExceptionPtrs->ExceptionRecord->ExceptionAddress,
				szFaultModule, sizeof(szFaultModule)));
		fflush(f);

		switch (pExceptionPtrs->ExceptionRecord->ExceptionCode) {
		case EXCEPTION_ACCESS_VIOLATION:
			fprintf_s(f, "Exception Code: ACCESS_VIOLATION T: %d A: 0x%08x\n\n",
				pExceptionPtrs->ExceptionRecord->ExceptionInformation[0],
				pExceptionPtrs->ExceptionRecord->ExceptionInformation[1]);
			break;
		case EXCEPTION_IN_PAGE_ERROR:
			fprintf_s(f, "Exception Code: IN_PAGE_ERROR T: %d A: 0x%08x NTSTATUS: 0x%08x\n\n",
				pExceptionPtrs->ExceptionRecord->ExceptionInformation[0],
				pExceptionPtrs->ExceptionRecord->ExceptionInformation[1],
				pExceptionPtrs->ExceptionRecord->ExceptionInformation[2]);
			break;
		case EXCEPTION_ARRAY_BOUNDS_EXCEEDED:
			fputs("Exception Code: ARRAY_BOUNDS_EXCEEDED\n\n", f);
			break;
		case EXCEPTION_BREAKPOINT:
			fputs("Exception Code: BREAKPOINT\n\n", f);
			break;
		case EXCEPTION_DATATYPE_MISALIGNMENT:
			fputs("Exception Code: DATATYPE_MISALIGNMENT\n\n", f);
			break;
		case EXCEPTION_FLT_DENORMAL_OPERAND:
			fputs("Exception Code: FLT_DENORMAL_OPERAND\n\n", f);
			break;
		case EXCEPTION_FLT_DIVIDE_BY_ZERO:
			fputs("Exception Code: FLT_DIVIDE_BY_ZERO\n\n", f);
			break;
		case EXCEPTION_FLT_INEXACT_RESULT:
			fputs("Exception Code: FLT_INEXACT_RESULT\n\n", f);
			break;
		case EXCEPTION_FLT_INVALID_OPERATION:
			fputs("Exception Code: FLT_INVALID_OPERATION\n\n", f);
			break;
		case EXCEPTION_FLT_OVERFLOW:
			fputs("Exception Code: FLT_OVERFLOW\n\n", f);
			break;
		case EXCEPTION_FLT_STACK_CHECK:
			fputs("Exception Code: FLT_STACK_CHECK\n\n", f);
			break;
		case EXCEPTION_FLT_UNDERFLOW:
			fputs("Exception Code: FLT_UNDERFLOW\n\n", f);
			break;
		case EXCEPTION_ILLEGAL_INSTRUCTION:
			fputs("Exception Code: ILLEGAL_INSTRUCTION\n\n", f);
			break;
		case EXCEPTION_INT_DIVIDE_BY_ZERO:
			fputs("Exception Code: INT_DIVIDE_BY_ZERO\n\n", f);
			break;
		case EXCEPTION_INT_OVERFLOW:
			fputs("Exception Code: INT_OVERFLOW\n\n", f);
			break;
		case EXCEPTION_INVALID_DISPOSITION:
			fputs("Exception Code: INVALID_DISPOSITION\n\n", f);
			break;
		case EXCEPTION_NONCONTINUABLE_EXCEPTION:
			fputs("Exception Code: NONCONTINUABLE_EXCEPTION\n\n", f);
			break;
		case EXCEPTION_PRIV_INSTRUCTION:
			fputs("Exception Code: PRIV_INSTRUCTION\n\n", f);
			break;
		case EXCEPTION_SINGLE_STEP:
			fputs("Exception Code: SINGLE_STEP\n\n", f);
			break;
		case EXCEPTION_STACK_OVERFLOW:
			fputs("Exception Code: STACK_OVERFLOW\n\n", f);
			break;
		case DBG_CONTROL_C:
			fputs("Exception Code: DBG_CONTROL_C\n\n", f);
			break;
		}
		fflush(f);

		fprintf_s(f, "Registers:\n"
			"EAX: 0x%08X\tEBX: 0x%08X\tECX: 0x%08X\tEDX: 0x%08X\n"
			"ESI: 0x%08X\tEDI: 0x%08X\tEBP: 0x%08X\tESP: 0x%08X\n"
			"EIP: 0x%08X\tEFS: 0x%p\tEFLAGS: 0x%08X\n\nStack:\n",
			pExceptionPtrs->ContextRecord->Eax, pExceptionPtrs->ContextRecord->Ebx,
			pExceptionPtrs->ContextRecord->Ecx, pExceptionPtrs->ContextRecord->Edx,
			pExceptionPtrs->ContextRecord->Esi, pExceptionPtrs->ContextRecord->Edi,
			pExceptionPtrs->ContextRecord->Ebp, pExceptionPtrs->ContextRecord->Esp,
			pExceptionPtrs->ContextRecord->Eip, hInstance,
			pExceptionPtrs->ContextRecord->EFlags);
		fflush(f);

		pdwStack = (DWORD*)pExceptionPtrs->ContextRecord->Esp;
		DumpMemory(f, reinterpret_cast<BYTE*>(pdwStack), 320, TRUE);

		fprintf_s(f, "\nSCM Op: 0x%X, L: %d, Dump:\n", dwScmOpcodeDebug, bScmLocalDebug);
		if (gst)
			DumpMemory(f, reinterpret_cast<BYTE*>(gst), sizeof(GAME_SCRIPT_THREAD));
		else
			fputs("-Not initialized-\n", f);
		fflush(f);

		DumpGamePools(f);
		DumpLocalPed(f);

		if(iGtaVersion == GTASA_VERSION_USA10)
			fputs("\nGame Version: US 1.0\n", f);
		else if (iGtaVersion == GTASA_VERSION_EU10)
			fputs("\nGame Version: EU 1.0\n", f);
		else
			fputs("\nGame Version: UNKNOWN\n", f);
		fflush(f);

		DumpLoadedModules(f);

		fclose(f);
	}
}

static void SetDlgItemTextFromFile(HWND hDlg)
{
	HANDLE hFile = CreateFileA(szCrashInfoFile, GENERIC_READ,
		FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
	if (hFile != INVALID_HANDLE_VALUE) {
		DWORD dwSize = GetFileSize(hFile, NULL);
		if (dwSize != INVALID_FILE_SIZE) {
			char* szOutput = (char*)malloc(dwSize + 1);
			if (szOutput != NULL) {
				if (ReadFile(hFile, szOutput, dwSize, NULL, NULL) == TRUE) {
					szOutput[dwSize] = '\0';
					SetDlgItemText(hDlg, IDC_REPORT_OUTPUT, szOutput);
				}
				free(szOutput);
			}
		}
		CloseHandle(hFile);
	}
}

static INT_PTR CALLBACK GuiDlgProcMain(HWND hDlg, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	switch (uMsg) {
	case WM_MOUSEMOVE:
		ShowCursor(TRUE);
		break;
	case WM_INITDIALOG:
		SetCursor(LoadCursorA(NULL, IDC_ARROW));
		ShowCursor(TRUE);

		DumpMain();

		SetDlgItemTextFromFile(hDlg);
		SetForegroundWindow(GetDlgItem(hDlg, IDD_EXCEPTION));
		SetFocus(GetDlgItem(hDlg, IDCLOSE));
		break;
	case WM_DESTROY:
		EndDialog(hDlg, TRUE);
		break;
	case WM_COMMAND:
		switch (LOWORD(wParam)) {
		case IDCLOSE:
			EndDialog(hDlg, TRUE);
			break;
		}
		break;
	}
	return FALSE;
}

LONG WINAPI exc_handler(_EXCEPTION_POINTERS* exc_inf)
{
	// a stray debug register trap is not a crash, never turn one into a dump
	if (exc_inf->ExceptionRecord->ExceptionCode == EXCEPTION_SINGLE_STEP)
		return EXCEPTION_CONTINUE_EXECUTION;

	pExceptionPtrs = exc_inf;

	if (pGame) {
		HWND hwnd = pGame->GetMainWindowHwnd();
		ShowWindow(hwnd, SW_MINIMIZE);
		DialogBoxA(hInstance, MAKEINTRESOURCE(IDD_EXCEPTION), hwnd, GuiDlgProcMain);
	} else {
		DumpMain();
	}
	return EXCEPTION_CONTINUE_SEARCH;
}

// https://docs.microsoft.com/en-us/cpp/cpp/try-except-statement?view=msvc-160
int exc_filter(unsigned int code, struct _EXCEPTION_POINTERS* ep, char* type)
{
	if (pChatWindow)
	{
		if (!memcmp(type, "opcode", 7))
		{
			if (dwScmOpcodeDebug == 0x6E7)
			{
				pChatWindow->AddDebugMessage("Warning(add_car_component %u): Exception 0x%X at 0x%X",
					wLastVehicleComponent, code, ep->ContextRecord->Eip);

				return EXCEPTION_EXECUTE_HANDLER;
			}

			pChatWindow->AddDebugMessage("Warning(opcode 0x%X): Exception 0x%X at 0x%X",
				dwScmOpcodeDebug, code, ep->ContextRecord->Eip);
		}
		else
		{
			pChatWindow->AddDebugMessage("Warning(%s): Exception 0x%X at 0x%X",
				type, code, ep->ContextRecord->Eip);
		}
	}
	if (dwExcWarningCount >= 10)
	{
		return EXCEPTION_CONTINUE_SEARCH;
	}
	dwExcWarningCount++;
	return EXCEPTION_EXECUTE_HANDLER;
}
