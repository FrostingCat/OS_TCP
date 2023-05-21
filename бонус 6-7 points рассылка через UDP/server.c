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

int* queue1;
int* queue2;
int shm_fd_1;
int shm_fd_2;
int sendSock;

void my_handler(int nsig) {
    close(sendSock);

    munmap(queue1, 4);
    close(shm_fd_1);
    shm_unlink(SHM_NAME1);

    munmap(queue2, 4);
    close(shm_fd_2);
    shm_unlink(SHM_NAME2);

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
    struct sockaddr_in broadcastAddr;
    int broadcastPermission;
    unsigned short broadcastPort;

    if (argc != 4)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port> <Broadcast IP Address> <Broadcast Port>\n", argv[0]);
        exit(1);
    }

    shm_fd_1 = shm_open(SHM_NAME1, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_1, 4);
    queue1 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_1, 0);

    shm_fd_2 = shm_open(SHM_NAME2, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_2, 4);
    queue2 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_2, 0);

    queue1[1] = 1;
    queue2[1] = 1;
    queue1[0] = 0;
    queue2[0] = 0;

    echoServPort = atoi(argv[1]);
    broadcastIP = argv[2];
    broadcastPort = atoi(argv[3]);

    if ((sendSock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");
    
    broadcastPermission = 1;
    if (setsockopt(sendSock, SOL_SOCKET, SO_BROADCAST, (void *) &broadcastPermission, 
          sizeof(broadcastPermission)) < 0)
        DieWithError("setsockopt() failed");
    
    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = inet_addr(broadcastIP);/* Broadcast IP address */
    broadcastAddr.sin_port = htons(broadcastPort);

    servSock = CreateTCPServerSocket(echoServPort);  // тут мы создаем сервер, который слушает поступающие соединения

    for (;;) {
        clntSock = AcceptTCPConnection(servSock);
 
        if ((processID = fork()) < 0)
            DieWithError("fork() failed");
        else if (processID == 0) {
            close(servSock);   /* Child closes parent socket */
            handle(clntSock, sendSock, broadcastAddr, queue1, queue2);

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