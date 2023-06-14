#ifndef _UTILS_H
#define _UTILS_S

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <time.h>
#include <errno.h>

#define MAX_PATH_LEN 255
#define TRUE 1
#define FALSE 0

#define SYSCALL_EXIT(r,c,s) if((r=c)==-1){perror(s); fflush(stderr); exit(errno);}
#define CHECKNULL(r,c,s) if((r=c)==NULL){perror(s); exit(errno);}
#define SYSCALL_RETURN(r,c,s) if((r=c)==-1){perror(s); return(-1);}

#endif
