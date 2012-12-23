/*
 ============================================================================
 Name        :	PE.c
 Author      :	Zerith
 Version     :
 Copyright   :	Your copyright notice
 Description :	This file contains functions used to interpret the targets PE header,
 	 	 	 	modify the PE, and append code to the target file.
 ============================================================================
 */
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <windows.h>

#define SECTION_ALIGNMENT	0x1000
#define FILE_ALIGNMENT		0x200
#define SECTION_ALIGN(x)	(((x) + (SECTION_ALIGNMENT - 1)) & (~SECTION_ALIGNMENT + 1))
#define FILE_ALIGN(x) 		(((x) + (FILE_ALIGNMENT - 1)) & (~FILE_ALIGNMENT + 1))

struct LdrData
{
	char		szDLL_Name[MAX_PATH];
	WCHAR		wcszKernel32[20];
	char		szLoadLibrary[20];
	DWORD		dwOriginalEntryPoint;

};
extern struct LdrData	LDR_DATA;

/*
 * @name GetSectionData
 * Obtains a pointer to the IMAGE_SECTION_HEADER structure of the section specified by the
 * name. The section should be contained within Integrator file.
 * @parameter SectionName
 *            The name of section whose IMAGE_SECTION_HEADER is requested.
 * @return Returns a pointer to the IMAGE_SECTION_HEADER of a specified section
 */
IMAGE_SECTION_HEADER GetSectionData(char *SectionName)
{
	IMAGE_DOS_HEADER		*ModuleBase;
	IMAGE_SECTION_HEADER	*pSection;
	IMAGE_SECTION_HEADER	SectionData;
	USHORT					i;

	ModuleBase			= (IMAGE_DOS_HEADER *)GetModuleHandle(NULL);
	pSection			= (IMAGE_SECTION_HEADER *)	(ModuleBase->e_lfanew +
													(DWORD)ModuleBase +
													sizeof(IMAGE_NT_HEADERS));
	//	Traverse the IMAGE_SECTION_HEADER array and find
	//	the entry corresponding to the given section name:
	for (i = 0; strncmp(SectionName, (char *)pSection[i].Name, sizeof(pSection[i].Name)); i++);

	memcpy(&SectionData, &pSection[i], sizeof(IMAGE_SECTION_HEADER));
	return SectionData;
}
/*
 * @name		Integrator_UpdateFile
 * Updates the PE-File's internal structures to contain the inserted section.
 * @parameter	File
 * 				A FILE * to the file to be updated.
 * @parameter	Section
 *				The section that has been inserted and the file needs to be aware about.
 */
VOID Integrator_UpdateFile(char	*szFilePath, IMAGE_SECTION_HEADER *Section)
{
	IMAGE_DOS_HEADER 		DosHeader;
	IMAGE_SECTION_HEADER	LastSection;
	WORD					NumberOfSections;
	DWORD					SizeOfImage;
	FILE					*File;
	//	File Location Pointers:
	long int				ntHeadersFilePtr, nSectionsFilePtr, SizeOfImageFilePtr;

	File = fopen(szFilePath, "r+b");
	fread((void *)&DosHeader, sizeof(DosHeader), 1, File);
	fseek(File, DosHeader.e_lfanew, SEEK_SET);
	//	At this point, file pointer points at IMAGE_NT_HEADERS.
	ntHeadersFilePtr	= ftell(File);
	fseek(File, offsetof(IMAGE_NT_HEADERS, FileHeader.NumberOfSections), SEEK_CUR);
	nSectionsFilePtr	= ftell(File);
	fread(&NumberOfSections, sizeof(NumberOfSections), 1, File);
	//
	//	Modify the Code&Data section's virtual address to valid values.
	//	then append the section descriptors to the end of the IMAGE_SECTION_HEADER array:
	//
	fseek(File, ntHeadersFilePtr + sizeof(IMAGE_NT_HEADERS), SEEK_SET);
	fseek(File, (NumberOfSections - 1) * sizeof(IMAGE_SECTION_HEADER), SEEK_CUR);
	fread(&LastSection, sizeof(LastSection), 1, File);
	Section->VirtualAddress =	SECTION_ALIGN(LastSection.VirtualAddress +
								LastSection.Misc.VirtualSize);
	fseek(File, 0, SEEK_CUR);
	fwrite(Section, sizeof(*Section), 1, File);
	//
	//	Update the NumberOfSections:
	//
	fseek(File, nSectionsFilePtr, SEEK_SET);
	NumberOfSections++;
	fwrite(&NumberOfSections, sizeof(NumberOfSections), 1, File);
	//
	//	Update SizeOfImage:
	//
	fseek(File, ntHeadersFilePtr, SEEK_SET);
	fseek(File, offsetof(IMAGE_NT_HEADERS, OptionalHeader.SizeOfImage), SEEK_CUR);
	SizeOfImageFilePtr	= ftell(File);
	fread(&SizeOfImage, sizeof(SizeOfImage), 1, File);
	SizeOfImage	+=	SECTION_ALIGN(Section->Misc.VirtualSize);
	fseek(File, SizeOfImageFilePtr, SEEK_SET);
	fwrite(&SizeOfImage, sizeof(SizeOfImage), 1, File);

	fclose(File);
}
/*
 * @name	TransferSection
 * 			Copies a section contained in this file and 'inserts' it in the target game,
 * 			then updates the target game file so it is aware of the changes.
 * @parameter	szSectionName
 * 				The name of the section to be transferred
 * @parameter	szGamePath
 * 				The name of the target game file.
 * @returns		The return value is the position within the file at which the
 * 				section was inserted.
 */
LONG Integrator_TransferSection(char *szSectionName, char *szGamePath)
{
	IMAGE_SECTION_HEADER	SectionHeader;
	LPVOID					pSectionData;
	FILE					*fGame;

	fGame			= fopen(szGamePath, "ab");
	SectionHeader	= GetSectionData(szSectionName);
	pSectionData	= (void *)(SectionHeader.VirtualAddress + (DWORD)GetModuleHandle(NULL));

	fseek(fGame, 0, SEEK_END);
	SectionHeader.PointerToRawData = FILE_ALIGN(ftell(fGame));
	//	Pad the file with zeroes so the section is placed on File Alignment.
	while (ftell(fGame) < (long int)SectionHeader.PointerToRawData)
		fputc(0x00, fGame);

	fwrite(pSectionData, SectionHeader.SizeOfRawData, 1, fGame);
	fclose(fGame);
	Integrator_UpdateFile(szGamePath, &SectionHeader);

	return SectionHeader.VirtualAddress;
}
/*
 * @name IntegrateCode
 * Inserts the two sections used to load the DLL, at the end of the target file.
 * Then, updates the PE Section structures and points the Entry Point to
 * the '.Ldr_code' section.
 */
void IntegrateCode(char *szDLL_Name, char *szGamePath)
{
	DWORD				dwNewEntryPoint;

	dwNewEntryPoint = Integrator_TransferSection("LDRCODE", szGamePath);
	//
	//	Update the Entry Point of the File to point to the 'LDRCODE' section:
	//
	IMAGE_DOS_HEADER	dosHeader;
	IMAGE_NT_HEADERS	ntHeaders;
	FILE				*fGame;

	fGame = fopen(szGamePath, "r+b");
	fread(&dosHeader, sizeof(dosHeader), 1, fGame);
	fseek(fGame, dosHeader.e_lfanew, SEEK_SET);
	fread(&ntHeaders, sizeof(ntHeaders), 1, fGame);

	fflush(fGame);	//	Make sure read operation has performed
	fseek(fGame, dosHeader.e_lfanew, SEEK_SET);
	fseek(fGame, offsetof(IMAGE_NT_HEADERS, OptionalHeader.AddressOfEntryPoint), SEEK_CUR);
	fwrite(&dwNewEntryPoint, sizeof(dwNewEntryPoint), 1, fGame);
	fflush(fGame);	//	Make sure everything has been written out
	//
	//	Initialize the LDR_DATA structure:
	//
	LDR_DATA.dwOriginalEntryPoint = ntHeaders.OptionalHeader.AddressOfEntryPoint;
	strncpy(LDR_DATA.szDLL_Name, szDLL_Name, sizeof(LDR_DATA.szDLL_Name));
	wcsncpy(LDR_DATA.wcszKernel32, L"kernel32.dll", wcslen(L"kernel32.dll"));
	strncpy(LDR_DATA.szLoadLibrary, "LoadLibraryA", sizeof(LDR_DATA.szLoadLibrary));

	//	Transfer the LDRDATA section, now that it is initialized:
	Integrator_TransferSection("LDRDATA", szGamePath);
}
