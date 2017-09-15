#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>

void error(const char *msg)
{
    perror(msg);
    exit(0);
}

int main(int argc, char *argv[])
{
    int sockfd, portno, n;
    struct sockaddr_in serv_addr;
    struct hostent *server;
    char state = 0x20;
    if (argc < 5) {
      fprintf(stdout,"usage %s ip password relay time(in ms) [state]\n", argv[0]);
      fprintf(stdout,"\tstate can be 1 (default) for or 0 for off\n");

       exit(0);
    }

    if (argc > 5) {
      if (atoi(argv[5])){
        state = 0x21;
      }
    }
    //portno = atoi(argv[2]);
    portno = 17494;


    /*
    0x20 - turn the relay on command
    0x03 - relay 3
    0x32 (50) - 5 seconds (50 * 100ms)
     */
    char responsebuffer[255];

    int i;
    char logoutbuffer[1];
    logoutbuffer[0]=0x7b;

    char relaybuffer[3];
    relaybuffer[0]=state;
    relaybuffer[1]=(char)(atoi(argv[3]));
    relaybuffer[2]=(char)(atoi(argv[4])/100);

    char pwbuffer[strlen(argv[2])+1];
    bzero(pwbuffer,strlen(argv[2])+1);
    pwbuffer[0]=0x79;//121;
    for(i=0;i<strlen(argv[2]);i++){
      pwbuffer[i+1]=argv[2][i];
    }


    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0)
        error("ERROR opening socket");
    server = gethostbyname(argv[1]);
    if (server == NULL) {
        fprintf(stderr,"ERROR, no such host\n");
        exit(0);
    }

    bzero((char *) &serv_addr, sizeof(serv_addr));
    serv_addr.sin_family = AF_INET;
    bcopy((char *)server->h_addr, (char *)&serv_addr.sin_addr.s_addr, server->h_length);
    serv_addr.sin_port = htons(portno);
    if (connect(sockfd,(struct sockaddr *) &serv_addr,sizeof(serv_addr)) < 0) error("ERROR connecting");

    // send the password message
    n = write(sockfd,pwbuffer,strlen(pwbuffer));
    if (n < 0) error("ERROR writing to socket");


    bzero(responsebuffer,255);
    n = read(sockfd,responsebuffer,255);
    if (n < 0)  error("ERROR reading from socket");
    printf("one means pw ok %s\n",responsebuffer);

    // send the relay message
    n = write(sockfd,relaybuffer,strlen(relaybuffer));
    if (n < 0) error("ERROR writing to socket");

    bzero(responsebuffer,255);
    n = read(sockfd,responsebuffer,255);
    if (n < 0)  error("ERROR reading from socket");
    printf("zero means ok %s\n",responsebuffer);


    // send the logout message
    n = write(sockfd,logoutbuffer,strlen(logoutbuffer));
    if (n < 0) error("ERROR writing to socket");

    n = read(sockfd,responsebuffer,255);
    if (n < 0)  error("ERROR reading from socket");
    printf("logout %s\n",responsebuffer);

    close(sockfd);
    return 0;
}
