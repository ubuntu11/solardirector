
#include "common.h"
#include "debug.h"

#if 0
#include <stdarg.h>
#include <sys/types.h>
#include <sys/signal.h>
#include <sys/wait.h>


int os_wait(long pid) {
        long rpid;
        int status, reason;

        DLOG(LOG_DEBUG,"pid: %ld", pid);

        /* Get the exit reason */
        rpid = wait4(pid, &reason, WNOHANG, 0);
        if (rpid < 0) {
                DLOG(LOG_DEBUG2|LOG_SYSERR,"waitpid");
                return 1;
        } else if (rpid == 0) return 0;

        DLOG(LOG_DEBUG2,"pid: %ld, reason: %x", rpid, reason);

        status = 0;
        if (WIFEXITED(reason)) {
                DLOG(LOG_DEBUG2,"exited.");
                /* Kill the child process */
                kill(rpid,SIGTERM);
                status = WEXITSTATUS(reason);
        } else if (WIFSIGNALED(reason)) {
                DLOG(LOG_DEBUG2,"signaled.");
                status = WTERMSIG(reason);
        } else if (WIFSTOPPED(reason)) {
                DLOG(LOG_DEBUG2,"stopped.");
                status = WSTOPSIG(reason);
        }

        DLOG(LOG_DEBUG,"returning: %d", status);
        return status;
}

int os_spawn(int wait, char *prog,...) {
        va_list ap;
        long pid;
        char *args[16];
        register int x;

        /* Get the arguments into an array */
        DLOG(LOG_DEBUG,"spawn: getting args...");
        args[0] = prog;
        DLOG(LOG_DEBUG,"spawn: arg[0]: %s",args[0]);
        va_start(ap,prog);
        for(x=1; x < 16; x++) {
                args[x] = va_arg(ap,char *);
                if (!args[x]) break;
                DLOG(LOG_DEBUG,"spawn: arg[%d]: %s",x,args[x]);
        }
        va_end(ap);

        /* Fork a subprocess and execute the prog */
        DLOG(LOG_DEBUG,"spawn: forking...");
        pid = fork();
        if (pid < 0)
                return 1;
        else if (pid == 0) {
                execvp(prog,args);
                /*
                log_write(LOG_SYSERR,"spawn: %s",prog);
                */
                _exit(1);
        }

        if (!wait) return 0;

        /* Return waitpid's answer */
        return os_wait(pid);
}
#endif

int main(void) {
	char *args[32];
	int i;
	char *targs[] = { "t","-d","8" };

	solard_common_init(3,targs,0,0xFFFFF);

	os_setenv("MQTT_C_CLIENT_TRACE","ON",1);
	os_setenv("MQTT_C_CLIENT_TRACE_LEVEL","PROTOCOL",1);
//	os_setenv("MQTT_C_CLIENT_TRACE_LEVEL","MINIMUM",1);
#if 0
	os_spawn(1,"../agents/jbd/jbd","-d","6","-c","jbdtest1.conf","-n","pack_01","-I");
#else
	args[i++] = "../agents/jbd/jbd";
	args[i++] = "-d";
	args[i++] = "6";
	args[i++] = "-c";
	args[i++] = "jbdtest1.conf";
	args[i++] = "-n";
	args[i++] = "pack_01";
	args[i++] = "-I";
	args[i++] = 0;
	solard_exec("../agents/jbd/jbd",args,0,1);
#endif
	return 0;
}
