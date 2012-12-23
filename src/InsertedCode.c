/*
 * InsertedCode.c
 *
 *  Created on: 23 Aug 2012
 *      Author: Zerith
 */
#include "InsertedCode.h"

#define EXPORT_DIRECTORY_INDEX 0x00

__attribute__((section ("LDRDATA"))) struct LdrData LDR_DATA;
//
//	Function definitions
//
__attribute__((inline)) BOOL StringCompare(const char *s1, const char *s2)
{
	while(*s1 != '\0')
	{
		if (*s1++ != *s2++)
			return FALSE;
	}
	return TRUE;
}

__attribute__((inline)) VOID RtlInitUnicodeString(PUNICODE_STRING pDest, PWSTR pSrc)
{
	USHORT wstrlen = 0;

	pDest->Buffer = pSrc;
	//	Compute strlen:
	while (*(WCHAR *)pSrc++ != L'\0')
		wstrlen++;
	wstrlen *= sizeof(WCHAR);

	pDest->Length			= wstrlen;
	pDest->MaximumLength	= wstrlen + sizeof(WCHAR);
}

__attribute((inline)) BOOL RtlCompareUnicodeString(UNICODE_STRING s1, UNICODE_STRING s2)
{
	PWSTR	ps1		= s1.Buffer;
	PWSTR	ps2 	= s2.Buffer;
	USHORT wstrlen	= s1.Length / sizeof(WCHAR);

	if (s1.Length != s2.Length)
		return FALSE;

	while (wstrlen--)
		if (*ps1++ != *ps2++)
			return FALSE;

	return TRUE;
}

__attribute__((inline)) PVOID GetModuleBase(PWSTR szModule)
{
	struct PEB		*pPEB;
	LDR_MODULE		*Module;
	UNICODE_STRING	wcszModule;

	__asm volatile("movl %%fs:0x30, %0" : "=r"(pPEB) : : );
	Module = (LDR_MODULE *)pPEB->LoaderData->InLoadOrderModuleList.Flink;
	RtlInitUnicodeString(&wcszModule, szModule);
	while (Module->InLoadOrderModuleList.Flink)
	{
		if (RtlCompareUnicodeString(Module->BaseDllName, wcszModule) == TRUE)
			return Module->BaseAddress;

		Module = (LDR_MODULE *)Module->InLoadOrderModuleList.Flink;
	}

	return	NULL;
}

__attribute__((inline)) FARPROC GetProcedureAddress(PVOID ModuleBase, const char *szProcName)
{
	DWORD					ImageBase;
	IMAGE_NT_HEADERS		*ntHeaders;
	IMAGE_DOS_HEADER		*dosHeader;
	IMAGE_EXPORT_DIRECTORY	*ExportDir;
	DWORD					*AddressOfNames;
	UINT16					*AddressOfNameOrdinals;
	DWORD					*AddressOfFunctions;
	//
	//	Navigate memory to find the Export Descriptors of the given module:
	//
	ImageBase	= (DWORD)ModuleBase;
	dosHeader	= (IMAGE_DOS_HEADER *)ImageBase;
	ntHeaders	= (IMAGE_NT_HEADERS *)(ImageBase + dosHeader->e_lfanew);
	ExportDir	= (IMAGE_EXPORT_DIRECTORY *)
			(ntHeaders->OptionalHeader.DataDirectory[EXPORT_DIRECTORY_INDEX].VirtualAddress +
			ImageBase);
	AddressOfNames 			= (DWORD *)(ExportDir->AddressOfNames + ImageBase);
	AddressOfNameOrdinals	= (UINT16 *)(ExportDir->AddressOfNameOrdinals + ImageBase);
	AddressOfFunctions		= (DWORD *)(ExportDir->AddressOfFunctions + ImageBase);
	//
	//	Find the specified function through the Export Table of the given module
	//
	for (DWORD i = ExportDir->NumberOfNames - 1; i > 0; i--)
	{
		const char *pmodulename = (char *)(AddressOfNames[i] + ImageBase);
		if (StringCompare(pmodulename, szProcName) == TRUE)
		{
			return (FARPROC)(AddressOfFunctions[AddressOfNameOrdinals[i]] + ImageBase);
		}
	}
	return (FARPROC)NULL;
}

__attribute__((section ("LDRCODE"))) __attribute__((used)) void Ldr_main()
{
	LoadLibraryPtr	pLoadLibrary;
	struct LdrData	*LoaderData;
	DWORD			dwOEP;
	const char		LdrDataSection[] = {'L', 'D', 'R', 'D', 'A', 'T', 'A', '\0'};

	//
	//	Traverse the section table to find the 'LDRDATA' section, then extract DLL name:
	//
	IMAGE_DOS_HEADER 		*dosHeader;
	IMAGE_SECTION_HEADER	*section;
	struct PEB				*pPEB;

	__asm volatile("movl %%fs:0x30, %0" : "=r"(pPEB) : : );
	dosHeader	= pPEB->ImageBaseAddress;
	section		= (IMAGE_SECTION_HEADER *)
			(dosHeader->e_lfanew + (LONG)pPEB->ImageBaseAddress + sizeof(IMAGE_NT_HEADERS));
	while (StringCompare((const char *)section->Name, LdrDataSection) != TRUE)
		section++;

	LoaderData = (struct LdrData *)(section->VirtualAddress + (DWORD)pPEB->ImageBaseAddress);
	//
	//	Load the DLL:
	//
	pLoadLibrary = (LoadLibraryPtr)GetProcedureAddress(
					GetModuleBase(LoaderData->wcszKernel32),
					LoaderData->szLoadLibrary);

	pLoadLibrary(LoaderData->szDLL_Name);
	// call OEP
	dwOEP = LoaderData->dwOriginalEntryPoint + (DWORD)pPEB->ImageBaseAddress;
	((void (*)())dwOEP)();
}


