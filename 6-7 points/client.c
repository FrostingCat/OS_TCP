#include <stdio.h>
#include <sys/socket.h>
#include <arpa/inet.h> 
#include <stdlib.h>
#include <string.h> 
#include <unistd.h>
#include <time.h>

void DieWithError(char *errorMessage);  /* Error handling function */

int main(int argc, char *argv[])
{
    int sock;                        /* Socket descriptor */
    struct sockaddr_in echoServAddr; /* Echo server address */
    unsigned short echoServPort;     /* Echo server port */
    char *servIP;                    /* Server IP address (dotted quad) */
    char echoString[30];                /* String to send to echo server */
    unsigned int echoStringLen;      /* Length of string to echo */
    int bytesRcvd, totalBytesRcvd;   /* Bytes read in single recv()
                                        and total bytes read */

    if (argc != 4) {
       fprintf(stderr, "Usage: %s <Server IP> <Customers number> <Echo Port>\n",
               argv[0]);
       exit(1);
    }

    servIP = argv[1];             /* First arg: server IP address (dotted quad) */
    char *p;
    long customers_kol = strtol(argv[2], &p, 10); /* Second arg: cust_num */
    echoServPort = atoi(argv[3]);

    if ((sock = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0)
        DieWithError("socket() failed");

    memset(&echoServAddr, 0, sizeof(echoServAddr));
    echoServAddr.sin_family      = AF_INET;
    echoServAddr.sin_addr.s_addr = inet_addr(servIP);
    echoServAddr.sin_port        = htons(echoServPort);

    if (connect(sock, (struct sockaddr *) &echoServAddr, sizeof(echoServAddr)) < 0)
        DieWithError("connect() failed");

    int number = 0;
    sprintf(echoString, "%d", number);
    echoStringLen = strlen(echoString);

    // отправляем сообщение серверу, которое говорит об открытии магазина
    if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
             DieWithError("send() sent a different number of bytes than expected");
    
    sleep(2);

    for (int i = 0; i < customers_kol; ++i) {
        
        sprintf(echoString, "%d", i);

        fflush(stdout);
        printf("customer %s has arrived\n", echoString);

        echoStringLen = strlen(echoString);

        if (send(sock, echoString, echoStringLen, 0) != echoStringLen)
             DieWithError("send() sent a different number of bytes than expected");

        srand(time(NULL) * (i + 2));
        sleep(rand() % 3 + 1);
    }

    close(sock);
    exit(0);
}
