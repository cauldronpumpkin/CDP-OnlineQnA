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
#include <iostream>

#define MAXIN 20
#define MAXSE 36
#define MAXOUT 1024

using namespace std;

int sockfd;

pthread_t wId, rId;

void getreq(char *inbuf, int len) 
{
  // printf("%s: ", "REQ");
  memset(inbuf, 0 ,len);
  fgets(inbuf,len,stdin);
}

string makeHeader(char lenOfPayload[4], char payload[MAXIN])
{
  if (payload[0] == '\n')
    return "";

  string sndbuf = "";
  if (payload[0] == '@')
  {
    int x = 1;
    for (int i = 0; i < 12; i++)
    {
      if (payload[x] != ':')
      {
        sndbuf += payload[i + 1];
        x++;
      }
      else
        sndbuf += 'X';
    }

    string temp = lenOfPayload;
    sndbuf += temp;

    for (int i = strlen(lenOfPayload); i < 4; i++)
    {
      sndbuf += 'X';
    }

    x++;
    for (int i = 16; i < MAXSE; i++)
    {
      if (x < strlen(payload))
        sndbuf += payload[x++];
      else
        sndbuf += 'X';
    }

    return sndbuf;
  }
  else
  {
    sndbuf = "responseXXXX";
    string temp = lenOfPayload;
    sndbuf += temp;

    for (int i = strlen(lenOfPayload); i < 4; i++)
    {
      sndbuf += 'X';
    }

    sndbuf += payload;

    if (snd.back() == '\n')
    {
      sndbuf.pop_back();
    }
    sndbuf += 'X';
    
    return sndbuf;
  }
}

void* displayMessage(void*) 
{
  rId = pthread_self();

  int n;
  char rcvbuf[MAXOUT];
  while (1) 
  {
    memset(rcvbuf,0,MAXOUT);               /* clear */
    n = read(sockfd, rcvbuf, MAXOUT - 1);  

    if (n <= 0)
      return  NULL;

    printf("%s\n", rcvbuf);    /* receive */
  }

  return NULL;
}

void sendMessage(int fd, string s)
{
  if (s == "")
    return;

  char* arr = new char[s.size() + 1];
  strcpy(arr, s.c_str());
  write(fd, arr, strlen(arr));
  delete[] arr;
}


void* readAndSendData()
{
  wId = pthread_self();

  int n;
  char payload[MAXIN];
  char tempbuf[MAXIN];
  while (1)
  {
    memset(payload, 0, MAXIN);
    getreq(payload, MAXIN);

    char sizeP[4];
    sprintf(sizeP, "%d", (int)strlen(payload));
    string sndbuf = makeHeader(sizeP, payload);

    sendMessage(sockfd, sndbuf);

    if (n <= 0)
      return NULL;
  }
}

void initialConnect(char userID[12])
{
  char sendID[MAXIN] = "myId";
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
  // printf("%s\n", sendID);

  pthread_t rThread, wThread;

  pthread_create(&rThread, NULL, displayMessage, NULL);
  readAndSendData();
  // pthread_create(&wThread, NULL, readAndSendData, NULL);

  pthread_join(rThread, NULL);
  // pthread_join(wThread, NULL);
}

void closeHandler(int sig)
{
  string s = "responseXXXX5XXXCLOSINGX";

  sendMessage(sockfd, s);
  
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
	char serverIP[] = "35.240.152.28";
	int portno = 5033;
	struct sockaddr_in serv_addr;
	
	buildServerAddr(&serv_addr, serverIP, portno);

	/* Create a TCP socket */
	sockfd = socket(AF_INET, SOCK_STREAM, 0);

	/* Connect to server on port */
	connect(sockfd, (struct sockaddr *) &serv_addr, sizeof(serv_addr));
	// printf("Connected to %s:%d\n",serverIP, portno);

  char userID[12];
  printf("%s: ", "Enter Name");
  scanf("%s", userID);

	/* Carry out Client-Server protocol */
  initialConnect(userID);
	/* Clean up on termination */
	// close(sockfd);
}