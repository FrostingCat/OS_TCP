#include <stdio.h>      /* for printf() and fprintf() */
#include <sys/socket.h> /* for recv() and send() */
#include <unistd.h>     /* for close() */
#include <stdlib.h>
#include <sys/wait.h>
#include <time.h>
#include <sys/mman.h>
#include <semaphore.h>
#include <sys/stat.h>
#include <sys/fcntl.h>
#include <string.h>

#define SHM_NAME3 "/queue3"
#define SHM_NAME4 "/queue4"
#define SHM_NAME5 "/queue5"

int* mass1;
int* mass2;
int* logs;
int shm_fd_3;
int shm_fd_4;
int shm_fd_5;

#define RCVBUFSIZE 32

void DieWithError(char *errorMessage);

void HandleClients(int clntSocket, int *queue1, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    int place1 = 0, place2 = 0;
    
    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 400);
    mass1 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);
    
    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 400);
    mass2 = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

    queue1[0] = 0;
    queue2[0] = 0;

    /* Receive message from client */
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    echoBuffer[recvMsgSize] = '\0';

    while (recvMsgSize > 0) {
        int number = logs[0] + 1;
        logs[0] += 3;
        logs[number] = 0;
        logs[number + 1] = atoi(echoBuffer);
        logs[number + 2] = 0;
        int person = atoi(echoBuffer);

        srand(time(NULL));
        int random_cashier = rand() % 2;
        if (random_cashier == 0) {
            queue1[0]++;
            mass1[place1] = person;
            place1++;
        } else {
            queue2[0]++;
            mass2[place2] = person;
            place2++;
        }

        random_cashier++;

        number = logs[0] + 1;
        logs[0] += 3;
        logs[number] = 0;
        logs[number + 1] = atoi(echoBuffer);
        logs[number + 2] = random_cashier;

        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");

        echoBuffer[recvMsgSize] = '\0';
    }

    int number = logs[0] + 1;
    logs[0] += 3;
    logs[number] = 0;
    logs[number + 1] = 0;
    logs[number + 2] = 3;
    queue1[1] = 0;
    queue2[1] = 0;

    close(clntSocket);
    munmap(mass1, 400);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);
    
    munmap(mass2, 400);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);
}

void HandleCashier1(int clntSocket, int *queue1) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char echoString[30]; 
    unsigned int echoStringLen;
    int place = 0;

    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 4);
    mass1 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);

    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

    // кассир1 готов к работе
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    echoBuffer[recvMsgSize] = '\0';

    while (echoBuffer[0] != '0') {
        
        int n = logs[0] + 1;
        logs[0] += 3;
        logs[n] = 1;
        logs[n + 1] = 0;
        logs[n + 2] = 1;
        
        while (queue1[0] == 0) {
            if (queue1[1] == 0) {
                break;
            }
            sleep(1);
        }

        if (queue1[1] == 0 && queue1[0] == 0) {
            int number = 0;
            
            n = logs[0] + 1;
            logs[0] += 3;
            logs[n] = 1;
            logs[n + 1] = 0;
            logs[n + 2] = 0;
            
            sprintf(echoString, "%d", number);
            echoStringLen = strlen(echoString);
            if (send(clntSocket, echoString, echoStringLen, 0) != echoStringLen)
                DieWithError("send() sent a different number of bytes than expected");
            break;
        }

        int number = 1;
        sprintf(echoString, "%d", number);
        echoStringLen = strlen(echoString);
        
        // отправляем сообщение о начале обслуживания
        if (send(clntSocket, echoString, echoStringLen, 0) != echoStringLen)
            DieWithError("send() sent a different number of bytes than expected");

        n = logs[0] + 1;
        logs[0] += 3;
        logs[n] = 1;
        logs[n + 1] = mass1[place];
        logs[n + 2] = 2;

        // получаем сообщение об окончании обслуживания
        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");

        queue1[0]--;
        place++;

        echoBuffer[recvMsgSize] = '\0';
    }

    close(clntSocket);
    munmap(mass1, 400);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);
}

void HandleCashier2(int clntSocket, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char echoString[30]; 
    unsigned int echoStringLen;
    int place = 0;

    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 4);
    mass2 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

    // кассир2 готов к работе
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    echoBuffer[recvMsgSize] = '\0';

    while (echoBuffer[0] != '0') {
        
        int n = logs[0] + 1;
        logs[0] += 3;
        logs[n] = 2;
        logs[n + 1] = 0;
        logs[n + 2] = 1;
        
        while (queue2[0] == 0) {
            if (queue2[1] == 0) {
                break;
            }
            sleep(1);
        }

        if (queue2[1] == 0 && queue2[0] == 0) {
            
            int n = logs[0] + 1;
            logs[0] += 3;
            logs[n] = 2;
            logs[n + 1] = 0;
            logs[n + 2] = 0;
            
            int number = 0;
            sprintf(echoString, "%d", number);
            echoStringLen = strlen(echoString);
            if (send(clntSocket, echoString, echoStringLen, 0) != echoStringLen)
                DieWithError("send() sent a different number of bytes than expected");
            break;
        }

        int number = 1;
        sprintf(echoString, "%d", number);
        echoStringLen = strlen(echoString);
        
        // отправляем сообщение о начале обслуживания
        if (send(clntSocket, echoString, echoStringLen, 0) != echoStringLen)
            DieWithError("send() sent a different number of bytes than expected");
        
        n = logs[0] + 1;
        logs[0] += 3;
        logs[n] = 2;
        logs[n + 1] = mass2[place];
        logs[n + 2] = 2;

        // получаем сообщение об окончании обслуживания
        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");

        queue2[0]--;
        place++;

        echoBuffer[recvMsgSize] = '\0';
    }

    close(clntSocket);
    munmap(mass2, 400);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);
}

void HandleListener(int clntSocket) {
    char echoString[50]; 
    unsigned int echoStringLen;
    
    shm_fd_5 = shm_open(SHM_NAME5, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_5, 400);
    logs = mmap(NULL, 400, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_5, 0);

    logs[0] = 0;
    int place = 0;
    
    for (;;) {
        while (logs[0] == place) {
            sleep(1);
        }
        while (place != logs[0]) {
            if (logs[place + 1] == 0) {
                if (logs[place + 2] == 0 && logs[place + 3] == 3) {
                    strcpy(echoString, "no clients anymore");
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                } else if (logs[place + 3] == 0) {
                    char cashNumber[3];
                    sprintf(cashNumber, "%d", logs[place + 2]);
                    strcpy(echoString, "customer ");
                    strcat(echoString, cashNumber);
                    strcat(echoString, " has arrived");
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                } else {
                    char cashNumber[3];
                    sprintf(cashNumber, "%d", logs[place + 2]);
                    char queueNumber[3];
                    sprintf(queueNumber, "%d", logs[place + 3]);
                    strcpy(echoString, "customer ");
                    strcat(echoString, cashNumber);
                    strcat(echoString, " goes to queue ");
                    strcat(echoString, queueNumber);
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                }  
            } else if (logs[place + 1] == 1) {
                if (logs[place + 2] == 0 && logs[place + 3] == 1) {
                    strcpy(echoString, "cashier1 is ready for a new customer");
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                } else if (logs[place + 3] == 0) {
                    strcpy(echoString, "cashier1 goes home");
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                } else {
                    char clientNumber[3];
                    sprintf(clientNumber, "%d", logs[place + 2]);
                    strcpy(echoString, "customer ");
                    strcat(echoString, clientNumber);
                    strcat(echoString, " is being served by cashier1");
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                }
            } else {
                if (logs[place + 2] == 0 && logs[place + 3] == 1) {
                    strcpy(echoString, "cashier2 is ready for a new customer");
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                } else if (logs[place + 3] == 0) {
                    strcpy(echoString, "cashier2 goes home");
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                } else {
                    char clientNumber[3];
                    sprintf(clientNumber, "%d", logs[place + 2]);
                    strcpy(echoString, "customer ");
                    strcat(echoString, clientNumber);
                    strcat(echoString, " is being served by cashier2");
                    echoStringLen = strlen(echoString);
                    send(clntSocket, echoString, echoStringLen, 0);
                }
            }
            place += 3;
            struct timespec tw = {0,300000000};
            struct timespec tr;
            nanosleep (&tw, &tr);
        }
    }

    munmap(logs, 400);
    close(shm_fd_5);
    shm_unlink(SHM_NAME5);
}

void handle(int clntSocket, int *queue1, int *queue2) {
     char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;

    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    echoBuffer[recvMsgSize] = '\0';

    if (echoBuffer[0] == '0') {
        HandleClients(clntSocket, queue1, queue2);
    } else if (echoBuffer[0] == '1') {
        HandleCashier1(clntSocket, queue1);
    } else if (echoBuffer[0] == '2') {
        HandleCashier2(clntSocket, queue2);
    } else if (echoBuffer[0] == '3') {
        HandleListener(clntSocket);
    }
}