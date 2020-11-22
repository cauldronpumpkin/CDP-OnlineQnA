// Client.c
#include <stdio.h>
#include <stdlib.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <signal.h>
#include <pthread.h>

#define MAXIN 20
#define MAXOUT 1024

int sockfd;

pthread_t wId, rId;

char *getreq(char *inbuf, int len) 
{
  memset(inbuf,0,len);
  return fgets(inbuf,len,stdin);
}

char *makeHeader(char typeOfMsg[12], char lenOfPayload[4])
{
  char header[16];
  strcpy(header, typeOfMsg);
  strcat(header, lenOfPayload);

  return header;
}

void* displayMessage() 
{
  rId = pthread_self();

  int n;
  char rcvbuf[MAXOUT];
  while (1) 
  {
    memset(rcvbuf,0,MAXOUT);               /* clear */
    n = read(sockfd, rcvbuf, MAXOUT-1);  

    if (n <= 0)
      return  NULL;

    printf("%s\n", rcvbuf);    /* receive */
  }

  return NULL;
}

void* readAndSendData()
{
  wId = pthread_self();

  int n;
  char sndbuf[MAXIN];
  while (1)
  {
    getreq(sndbuf, MAXIN);
    n = write(sockfd, sndbuf, strlen(sndbuf));

    if (n <= 0)
      return NULL;
  }
}

void initialConnect(char userID[12])
{
  char sendID[] = "myId";
  int size = strlen(userID);
  char sizeID[4];
  sprintf(sizeID,"%d", size);

  strcat(sendID, sizeID);
  for (int i = strlen(sendID); i < 8; i++)
  {
    sendID[i] = 'X';
  }
  int x = 0;
  for (int i = 8; i < 20; i++)
  {
    if (x < size)
      sendID[i] = userID[x++];
    else
      sendID[i] = 'X';
  }

  int n = write(sockfd, sendID, strlen(sendID));

  pthread_t rThread, wThread;

  pthread_create(&rThread, NULL, displayMessage, NULL);
  pthread_create(&wThread, NULL, readAndSendData, NULL);

  pthread_join(rThread, NULL);
  pthread_join(wThread, NULL);
}

void closeHandler(int sig)
{
  char msg[] = "CLOSE";
  int n = write(sockfd, msg, strlen(msg));
  
  if (pthread_self() == rId)
  {
    pthread_cancel(wId);
    pthread_exit(NULL);
  }
  else
  {
    pthread_cancel(rId);
    pthread_exit(NULL);
  }

  close(sockfd);
}

// Server address
struct hostent *buildServerAddr(struct sockaddr_in *serv_addr, char *serverIP, int portno) 
{
  /* Construct an address for remote server */
  memset((char *) serv_addr, 0, sizeof(struct sockaddr_in));
  serv_addr->sin_family = AF_INET;
  inet_aton(serverIP, &(serv_addr->sin_addr));
  serv_addr->sin_port = htons(portno);
}


int main() 
{
  signal(SIGINT, closeHandler);
	//Client protocol
	char *serverIP = "35.240.152.28";
	int portno = 5033;
	struct sockaddr_in serv_addr;
	
	buildServerAddr(&serv_addr, serverIP, portno);

	/* Create a TCP socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* Connect to server on port */
	connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	// printf("Connected to %s:%d\n",serverIP, portno);

  char userID[12];
  scanf("%s", userID);

	/* Carry out Client-Server protocol */
  initialConnect(userID);
	/* Clean up on termination */
	close(sockfd);
}