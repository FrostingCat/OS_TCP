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
#include <arpa/inet.h>

#define RCVBUFSIZE 50

void DieWithError(char *errorMessage);  /* Error handling function */

int main(int argc, char *argv[]) {
    int sock;                         /* Socket */
    struct sockaddr_in broadcastAddr; /* Broadcast Address */
    unsigned short broadcastPort;     /* Port */
    char recvString[RCVBUFSIZE + 1]; /* Buffer for received string */
    int recvStringLen;                /* Length of received string */

    if (argc != 2) {
        fprintf(stderr,"Usage: %s <Broadcast Port>\n", argv[0]);
        exit(1);
    }

    broadcastPort = atoi(argv[1]);   /* First arg: broadcast port */

    if ((sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0)
        DieWithError("socket() failed");

    memset(&broadcastAddr, 0, sizeof(broadcastAddr));   /* Zero out structure */
    broadcastAddr.sin_family = AF_INET;                 /* Internet address family */
    broadcastAddr.sin_addr.s_addr = htonl(INADDR_ANY);  /* Any incoming interface */
    broadcastAddr.sin_port = htons(broadcastPort);      /* Broadcast port */

    if (bind(sock, (struct sockaddr *) &broadcastAddr, sizeof(broadcastAddr)) < 0)
        DieWithError("bind() failed");

    for (;;) {
        if ((recvStringLen = recvfrom(sock, recvString, RCVBUFSIZE, 0, NULL, 0)) < 0)
        DieWithError("recvfrom() failed");

        recvString[recvStringLen] = '\0';
        printf("%s\n", recvString);
    }

    close(sock);
    exit(0);
}