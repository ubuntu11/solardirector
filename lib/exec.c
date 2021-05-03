
#include "common.h"
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/stat.h>
#include <fcntl.h>

int solard_exec(char *prog, char *args[], char *log, int wait) {
	int logfd;
	pid_t pid;
	int i,status;

	dprintf(5,"prog: %s, args: %p, log: %s, wait: %d\n", prog, args, log, wait);
	if (log) {
		/* Open the logfile */
		logfd = open(log,O_WRONLY | O_CREAT | O_TRUNC | O_SYNC, 0644);
		if (logfd < 0) {
			log_write(LOG_SYSERR,"solard_exec: open(%s)",log);
			return -1;
		}
	}

#if DEBUG
	for(i=0; args[i]; i++) dprintf(5,"args[%d]: %s\n", i, args[i]);
#endif

	/* Fork the process */
	pid = fork();

	/* If pid < 0, error */
	if (pid < 0) {
		log_write(LOG_SYSERR,"fork");
		return -1;
	}
	/* Parent */
	else if (pid > 0) {
		dprintf(5,"pid: %d\n", pid);
		/* Close the log in the parent */
		if (log) close(logfd);
		if (wait) {
			/* Wait for it to finish */
			do {
				if (waitpid(pid,&status,0) < 0) {
					kill(pid,SIGKILL);
					return -1;
				}
				dprintf(5,"WIFEXITED: %d\n", WIFEXITED(status));
			} while(!WIFEXITED(status));

			/* Get exit status */
			status = WEXITSTATUS(status);
			dprintf(5,"status: %d\n", status);

			/* Return status */
			return status;
		}
		return pid;
	}

	/* Child */

	/* Close stdin */
	close(0);

	if (log) {
		/* Make logfd stdout */
		dup2(logfd, STDOUT_FILENO);
		close(logfd);

		/* Redirect stderr to stdout */
		dup2(1, 2);
	}

	/* Run the program */
	return execv(prog,args);
}
