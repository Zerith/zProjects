/*
 ============================================================================
 Name        :	Integrator.c
 Author      :	Zerith
 Version     :
 Copyright   :	Your copyright notice
 Description :	General purpose Integrator of loaders into protected games.
 	 	 	 	The integrator accepts a protected game and a loader DLL as input,
 	 	 	 	It then appends code to the file that loads the DLL into the game,
 	 	 	 	which will execute before OEP.



 ============================================================================
 */
#include <stdlib.h>
#include <stdio.h>
#include <windows.h>

static char szGamePath[MAX_PATH];
static char szDLLPath[MAX_PATH];
extern void IntegrateCode(char *szDLL_Name, char *szGamePath);

int main(int argc, char *argv[])
{
	char szBakFilename[MAX_PATH];

	if (argc < 2)
	{
		printf("\nUsage: %s <Game_name.exe> <DLL_name.dll>", argv[0]);
		printf("\n\tBoth of the files need to be in the current directory");
		getchar();
		exit(ERROR_INVALID_PARAMETER);
	}
	strncpy(szGamePath, argv[1], sizeof(szGamePath));
	strncpy(szDLLPath, argv[2], sizeof(szDLLPath));
	// Create a backup of the game file:
	sscanf(szGamePath, "%[^.]s", szBakFilename);
	strcat(szBakFilename, ".bak");
	CopyFileA(szGamePath, szBakFilename, TRUE);
	// Prepend the code:
	IntegrateCode(szDLLPath, szGamePath);
	return EXIT_SUCCESS;
}
