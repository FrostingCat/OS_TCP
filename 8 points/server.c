#include "server.h"  /* TCP echo server includes */
#include <sys/wait.h>       /* for waitpid() */
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>
#include <arpa/inet.h>

#define SHM_NAME1 "/queue1"
#define SHM_NAME2 "/queue2"
#define SHM_NAME5 "/queue5"

int* queue1;
int* queue2;
int* logs;
int shm_fd_1;
int shm_fd_2;
int shm_fd_5;

void my_handler(int nsig) {
    
    munmap(queue1, 400);
    close(shm_fd_1);
    shm_unlink(SHM_NAME1);

    munmap(queue2, 400);
    close(shm_fd_2);
    shm_unlink(SHM_NAME2);

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);

    exit(0);
}

int main(int argc, char *argv[]) {

    signal(SIGINT, my_handler);
    
    int servSock;                    /* Socket descriptor for server */
    int clntSock;                    /* Socket descriptor for client */
    unsigned short echoServPort;     /* Server port */
    char *broadcastIP;
    pid_t processID;                 /* Process ID from fork() */
    unsigned int childProcCount = 0; /* Number of child processes */

    if (argc != 2) {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
        exit(1);
    }

    shm_fd_1 = shm_open(SHM_NAME1, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_1, 400);
    queue1 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_1, 0);

    shm_fd_2 = shm_open(SHM_NAME2, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_2, 400);
    queue2 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_2, 0);

    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

    queue1[1] = 1;
    queue2[1] = 1;
    queue1[0] = 0;
    queue2[0] = 0;
    logs[0] = 0;

    echoServPort = atoi(argv[1]);

    servSock = CreateTCPServerSocket(echoServPort);

    for (;;) {
        clntSock = AcceptTCPConnection(servSock);
 
        if ((processID = fork()) < 0)
            DieWithError("fork() failed");
        else if (processID == 0) {
            close(servSock);
            handle(clntSock, logs, queue1, queue2);

            exit(0);
        }

        close(clntSock);
        childProcCount++;

        while (childProcCount) {
            processID = waitpid((pid_t) -1, NULL, WNOHANG);
            if (processID < 0)
                DieWithError("waitpid() failed");
            else if (processID == 0)
                break;
            else
                childProcCount--;
        }

    }
}