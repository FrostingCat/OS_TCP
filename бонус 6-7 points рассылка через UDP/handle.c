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
#include <arpa/inet.h>

#define SHM_NAME3 "/queue3"
#define SHM_NAME4 "/queue4"

int* mass1;
int* mass2;
int shm_fd_3;
int shm_fd_4;

#define RCVBUFSIZE 32

void DieWithError(char *errorMessage);

void HandleClients(int clntSocket, int sendSock, struct sockaddr_in broadcastAddr, int *queue1, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    int place1 = 0, place2 = 0;
    char echoString[50]; 
    unsigned int echoStringLen;
    
    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 4);
    mass1 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);
    
    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 4);
    mass2 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    queue1[0] = 0;
    queue2[0] = 0;

    /* Receive message from client */
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    echoBuffer[recvMsgSize] = '\0';

    while (recvMsgSize > 0) {
        strcpy(echoString, "customer ");
        strcat(echoString, echoBuffer);
        strcat(echoString, " has arrived");
        echoStringLen = strlen(echoString);
        if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
               &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
            DieWithError("sendto() sent a different number of bytes than expected");
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

        char cashNumber[2];
        sprintf(cashNumber, "%d", random_cashier);

        strcpy(echoString, "customer ");
        strcat(echoString, echoBuffer);
        strcat(echoString, " goes to queue ");
        strcat(echoString, cashNumber);
        echoStringLen = strlen(echoString);
        if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
               &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
            DieWithError("sendto() sent a different number of bytes than expected");

        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");

        echoBuffer[recvMsgSize] = '\0';
    }

    strcpy(echoString, "no clients anymore");
    echoStringLen = strlen(echoString);
    if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
           &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
        DieWithError("sendto() sent a different number of bytes than expected");
    
    queue1[1] = 0;
    queue2[1] = 0;

    close(clntSocket);
    munmap(mass1, 4);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);
    munmap(mass2, 4);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);
}

void HandleCashier1(int clntSocket, int sendSock, struct sockaddr_in broadcastAddr, int *queue1) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char echoString[50]; 
    unsigned int echoStringLen;
    int place = 0;

    shm_fd_3 = shm_open(SHM_NAME3, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_3, 4);
    mass1 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_3, 0);

    // кассир1 готов к работе
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    echoBuffer[recvMsgSize] = '\0';

    while (echoBuffer[0] != '0') {
        strcpy(echoString, "cashier1 is ready for a new customer");
        echoStringLen = strlen(echoString);
        if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
               &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
             DieWithError("sendto() sent a different number of bytes than expected");
        while (queue1[0] == 0) {
            if (queue1[1] == 0) {
                break;
            }
            sleep(1);
        }

        if (queue1[1] == 0 && queue1[0] == 0) {
            int number = 0;
            strcpy(echoString, "cashier1 goes home");
            echoStringLen = strlen(echoString);
            if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
                   &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
                 DieWithError("sendto() sent a different number of bytes than expected");
            
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

        char clientNumber[3];
        sprintf(clientNumber, "%d", mass1[place]);
        
        strcpy(echoString, "customer ");
        strcat(echoString, clientNumber);
        strcat(echoString, " is being served by cashier1");
        echoStringLen = strlen(echoString);
        if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
               &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
             DieWithError("sendto() sent a different number of bytes than expected");

        // получаем сообщение об окончании обслуживания
        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");

        queue1[0]--;
        place++;

        echoBuffer[recvMsgSize] = '\0';
    }

    close(clntSocket);
    munmap(mass1, 4);
    close(shm_fd_3);
    shm_unlink(SHM_NAME3);
}

void HandleCashier2(int clntSocket, int sendSock, struct sockaddr_in broadcastAddr, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;
    char echoString[50]; 
    unsigned int echoStringLen;
    int place = 0;

    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 4);
    mass2 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    // кассир2 готов к работе
    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");
    echoBuffer[recvMsgSize] = '\0';

    while (echoBuffer[0] != '0') {
        strcpy(echoString, "cashier2 is ready for a new customer");
        echoStringLen = strlen(echoString);
        if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
               &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
             DieWithError("sendto() sent a different number of bytes than expected");
        
        while (queue2[0] == 0) {
            if (queue2[1] == 0) {
                break;
            }
            sleep(1);
        }

        if (queue2[1] == 0 && queue2[0] == 0) {
            strcpy(echoString, "cashier2 goes home");
            echoStringLen = strlen(echoString);
            if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
                   &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
                 DieWithError("sendto() sent a different number of bytes than expected");
            
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

        char clientNumber[3];
        sprintf(clientNumber, "%d", mass2[place]);
        
        strcpy(echoString, "customer ");
        strcat(echoString, clientNumber);
        strcat(echoString, " is being served by cashier2");
        echoStringLen = strlen(echoString);
        if (sendto(sendSock, echoString, echoStringLen, 0, (struct sockaddr *) 
               &broadcastAddr, sizeof(broadcastAddr)) != echoStringLen)
             DieWithError("sendto() sent a different number of bytes than expected");

        // получаем сообщение об окончании обслуживания
        if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
            DieWithError("recv() failed");

        queue2[0]--;
        place++;

        echoBuffer[recvMsgSize] = '\0';
    }

    close(clntSocket);
    munmap(mass2, 4);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);
}

void handle(int clntSocket, int sendSock, struct sockaddr_in broadcastAddr, int *queue1, int *queue2) {
    char echoBuffer[RCVBUFSIZE];
    int recvMsgSize;

    if ((recvMsgSize = recv(clntSocket, echoBuffer, RCVBUFSIZE, 0)) < 0)
        DieWithError("recv() failed");

    echoBuffer[recvMsgSize] = '\0';

    if (echoBuffer[0] == '0') {
        HandleClients(clntSocket, sendSock, broadcastAddr, queue1, queue2);
    } else if (echoBuffer[0] == '1') {
        HandleCashier1(clntSocket, sendSock, broadcastAddr, queue1);
    } else if (echoBuffer[0] == '2') {
        HandleCashier2(clntSocket, sendSock, broadcastAddr, queue2);
    }
}