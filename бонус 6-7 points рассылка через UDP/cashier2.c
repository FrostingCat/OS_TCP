#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <semaphore.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <signal.h>
#include <time.h>
#include <sys/fcntl.h>

#define SHM_NAME4 "/queue4"

int* mass2;
int shm_fd_4;

#define RCVBUFSIZE 32   /* Size of receive buffer */

void DieWithError(char *errorMessage);  /* Error handling function */

int main(int argc, char *argv[]) {
    
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char echoString[30];              /* String to send to echo server */
    char echoBuffer[RCVBUFSIZE];     /* Buffer for echo string */
    unsigned int echoStringLen;      /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv()
                                        and total bytes read */
    int place = 0;
    
    shm_fd_4 = shm_open(SHM_NAME4, O_CREAT | O_RDWR, 0644);
    ftruncate(shm_fd_4, 4);
    mass2 = mmap(NULL, 4, PROT_READ | PROT_WRITE, MAP_SHARED, shm_fd_4, 0);

    if (argc != 3) {
       fprintf(stderr, "Usage: %s <Server IP> <Echo Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];
    echoServPort = atoi(argv[2]);

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family      = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);
    echoServAddr.sin_port        = htons(echoServPort);

    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

    int number = 2;
    sprintf(echoString, "%d", number);
    echoStringLen = strlen(echoString);

    // отправляем сообщение, чтобы сервер понимал, что кассир1 стучится на сервер
    if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
        DieWithError("send() sent a different number of bytes than expected");

    sleep(1);

    // отправляем сообщение, чтобы сервер понимал, что кассир1 готов к работе
    if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
        DieWithError("send() sent a different number of bytes than expected");
    printf("i am cashier2 and i am ready\n");

    // получаем сообщение, что подошел клиент
    if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
        DieWithError("recv() failed or connection closed prematurely");
    echoBuffer[bytesRcvd] = '\0';

    while (echoBuffer[0] != '0') { // если клиенты еще есть
        printf("cashier2 is serving a client number %d\n", mass2[place]);
        place++;
        
        srand(time(NULL));
        sleep(rand() % 3 + 2);

        // сообщаем о готовности принять следующего клиента
        if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
                DieWithError("send() sent a different number of bytes than expected");
        printf("cashier2 is ready for a new client\n");

        // получаем информацию о следующем клиенте
        if ((bytesRcvd = recv(sock, echoBuffer, RCVBUFSIZE - 1, 0)) <= 0)
            DieWithError("recv() failed or connection closed prematurely");
        echoBuffer[bytesRcvd] = '\0';
    }

    printf("cashier2 is leaving\n");

    close(sock);
    munmap(mass2, 4);
    close(shm_fd_4);
    shm_unlink(SHM_NAME4);
    exit(0);
}
