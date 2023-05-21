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

#define SHM_NAME1 "/queue1"
#define SHM_NAME2 "/queue2"

int* queue1;
int* queue2;
int shm_fd_1;
int shm_fd_2;

void my_handler(int nsig) {
    printf("\nthe shop is closing\n");

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
    pid_t processID;                 /* Process ID from fork() */
    unsigned int childProcCount = 0; /* Number of child processes */ 

    if (argc != 2)     /* Test for correct number of arguments */
    {
        fprintf(stderr, "Usage:  %s <Server Port>\n", argv[0]);
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

    servSock = CreateTCPServerSocket(echoServPort);  // тут мы создаем сервер, который слушает поступающие соединения

    for (;;) {
        clntSock = AcceptTCPConnection(servSock);   // соединяем какого-то клиента, тут можно запустить сначала кассиров
 
        if ((processID = fork()) < 0)
            DieWithError("fork() failed");
        else if (processID == 0) {
            close(servSock);   /* Child closes parent socket */
            handle(clntSock, queue1, queue2); // получаем сообщение от клиента и отправляем обратно в вечном цикле (пока сообщение не 0)

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