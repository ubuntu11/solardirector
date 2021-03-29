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
		char cmdPath[256];
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

int main(int argc, char* argv[])
{
    // Fancy command line processing skipped for brevity
    int pid = getProcIdByName(argv[1]);
    return 0;
}
