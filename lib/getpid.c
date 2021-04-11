
/*
Copyright (c) 2021, Stephen P. Shoecraft
All rights reserved.

This source code is licensed under the BSD-style license found in the
LICENSE file in the root directory of this source tree.
*/

#ifndef _WINDOWS
#include <sys/types.h>
#include <dirent.h>
#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdio.h>

int getProcIdByName(char *procName) {
	int pid = -1;
	struct dirent *dirp;
	char cmdPath[512];

    // Open the /proc directory
    DIR *dp = opendir("/proc");
    if (dp != NULL) {
        // Enumerate all entries in directory until process found
        while (pid < 0 && (dirp = readdir(dp))) {
            // Skip non-numeric entries
            int id = atoi(dirp->d_name);
		printf("id: %d\n", id);
            if (id > 0) {
                // Read contents of virtual /proc/{pid}/cmdline file
		sprintf(cmdPath,"cat /proc/%s/cmdline",dirp->d_name);
		printf("cmdPath: %s\n", cmdPath);
		system(cmdPath);
#if 0
                getline(cmdFile, cmdLine);
                if (!cmdLine.empty())
                {
                    // Keep first cmdline item which contains the program path
                    size_t pos = cmdLine.find('\0');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(0, pos);
                    // Keep program name only, removing the path
                    pos = cmdLine.rfind('/');
                    if (pos != string::npos)
                        cmdLine = cmdLine.substr(pos + 1);
                    // Compare against requested process name
                    if (procName == cmdLine)
                        pid = id;
                }
#endif
            }
        }
    }

    closedir(dp);

    return pid;
}
#else
DWORD FindProcessId(const char *processname)
{
    HANDLE hProcessSnap;
    PROCESSENTRY32 pe32;
    DWORD result = NULL;

    // Take a snapshot of all processes in the system.
    hProcessSnap = CreateToolhelp32Snapshot(TH32CS_SNAPPROCESS, 0);
    if (INVALID_HANDLE_VALUE == hProcessSnap) return(FALSE);

    pe32.dwSize = sizeof(PROCESSENTRY32); // <----- IMPORTANT

    // Retrieve information about the first process,
    // and exit if unsuccessful
    if (!Process32First(hProcessSnap, &pe32))
    {
        CloseHandle(hProcessSnap);          // clean the snapshot object
        printf("!!! Failed to gather information on system processes! \n");
        return(NULL);
    }

    do
    {
        printf("Checking process %ls\n", pe32.szExeFile);
        if (0 == strcmp(processname, pe32.szExeFile))
        {
            result = pe32.th32ProcessID;
            break;
        }
    } while (Process32Next(hProcessSnap, &pe32));

    CloseHandle(hProcessSnap);

    return result;
}
#endif
