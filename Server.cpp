// #include <stdio.h>
// #include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <pthread.h>
#include <fstream>
#include <cstdint>
#define MAXREQ 1024
#define MAXQUEUE 5

using namespace std;

class Question 
{
public:
  string statement;
  string answer;
  string explantion;

  Question(string a, string b, string c)
  {
    statement = a;
    answer = b;
    explantion = c;
  }
};

unordered_map<string, vector<Question*>> question_bank;

string question_topics = "1 Threads\n2 Scheduling\n3 Memory Management\nn next question\nq quit\nr main menu\n";
char* question_topics_arr;

bool indivisualMode(int &fd)
{
  int n;
  char reqbuf[MAXREQ];
  
  write(fd, question_topics_arr, strlen(question_topics_arr));
  while(1)
  {
    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);
    Question *ques;
    
    string s = reqbuf;
    int rn;
    if (s == "1\n")
    {
      if (question_bank["Threads"].size() == 0)
        continue;
      rn = rand() % question_bank["Threads"].size();
      ques = question_bank["Threads"][rn];
    }
    else if (s == "2\n")
    { 
      if (question_bank["Scheduling"].size() == 0)
        continue;
      rn = rand() % question_bank["Scheduling"].size();
      ques = question_bank["Scheduling"][rn];
    }
    else if (s == "3\n")
    {
      if (question_bank["Memory Management"].size() == 0)
        continue;
      rn = rand() % question_bank["Memory Management"].size();
      ques = question_bank["Memory Management"][rn];
    }
    else if (s == "n\n")
    {
      write(fd, question_topics_arr, strlen(question_topics_arr));
      continue;
    }
    else if (s == "q\n")
      return 0;
    else if (s == "r\n")
      return 1;

    char statArr[ques->statement.size() + 1];
    strcpy(statArr, ques->statement.c_str());
    n = write(fd, statArr, strlen(statArr));

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1); // user-answer

    string user_answer = reqbuf;

    string response = question_topics;
    if (user_answer == ques->answer)
      response = "\nCorrect Answer. Explantion: \n" + ques->explantion + "\n" + response; 
    else
      response = "\nWrong Answer. Explantion: \n" + ques->explantion + "\n" + response;

    char resArr[response.size() + 1];
    strcpy(resArr, response.c_str());
    n = write(fd, resArr, strlen(resArr));
  }
}

void* server(void* fd) 
{
  int n;
  char reqbuf[MAXREQ];
  int consockfd = *((int *)fd);

  string instructionMsg = "I Indivisual\nG Group\nA Admin\n";
  char instructionArr[instructionMsg.size() + 1];
  strcpy(instructionArr, instructionMsg.c_str());

  string welcomeMsg = "Welcome " + to_string(pthread_self()) + " to online quiz on OS\n" + instructionMsg;
  char welcomeArr[welcomeMsg.size() + 1];
  strcpy(welcomeArr, welcomeMsg.c_str());
  n = write(consockfd, welcomeArr, strlen(welcomeArr));

  while (1) 
  {                   
    memset(reqbuf,0, MAXREQ);
    n = read(consockfd, reqbuf, MAXREQ-1); /* Recv */
    
    string s = reqbuf;
    if (s == "I\n")
    {
      bool f = indivisualMode(consockfd);
      if (f == 0)
      {
        string closingMsg = "Socket has been closed.\n";
        char closingArr[closingMsg.size() + 1];
        strcpy(closingArr, closingMsg.c_str());
        write(consockfd, closingArr, strlen(closingArr));
        close(consockfd);
        pthread_exit(NULL);
      }
      else
      {
        write(consockfd, instructionArr, strlen(instructionArr));
      }
    }
    else if (s == "G\n")
    {
      //
    }
    else if (s == "A\n")
    {
      //
    }

    if (n <= 0) 
      return NULL;
  }
}

int main() 
{
  question_topics_arr = new char[question_topics.size() + 1];
  strcpy(question_topics_arr, question_topics.c_str());

  string line;
  ifstream myfile ("question.txt");
  if (myfile.is_open())
  {
    while (getline (myfile,line))
    {
      int i = 0;
      string arr[] = {"", "", "", ""};
      while (i < 4 && line != "!!")
      {
        while (line != ";;")
        {
          arr[i] += line + "\n";
          if (!getline(myfile, line))
            break;
        }
        i++;
        getline(myfile, line);
      }

      arr[0].pop_back();

      Question *ques = new Question(arr[1], arr[2], arr[3]);
      question_bank[arr[0]].push_back(ques);
    }
    myfile.close();
  }

  int lstnsockfd, consockfd, portno = 5033;
  unsigned int clilen;
  struct sockaddr_in serv_addr, cli_addr;

  memset((char *) &serv_addr,0, sizeof(serv_addr));
  serv_addr.sin_family      = AF_INET;
  serv_addr.sin_addr.s_addr = INADDR_ANY;
  serv_addr.sin_port        = htons(portno);

  // Server protocol
  /* Create Socket to receive requests*/
  lstnsockfd = socket(AF_INET, SOCK_STREAM, 0);

  /* Bind socket to port */
  bind(lstnsockfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr));
  printf("Bounded to port\n");

  pthread_t tid[5];
  int tcounter = 0;
  printf("Listening for incoming connections\n");
  while (1) {

    /* Listen for incoming connections */
    listen(lstnsockfd, MAXQUEUE); 

    //clilen = sizeof(cl_addr);

    /* Accept incoming connection, obtaining a new socket for it */
    int newsocketfd = accept(lstnsockfd, (struct sockaddr *) &cli_addr, &clilen);

    pthread_create(&tid[tcounter], NULL, server, &newsocketfd);
      // cout << "Failed to Create a Thread\n";

    tcounter++;

    if (tcounter >= 5)
    {
      tcounter = 0;
      while (tcounter < 5)
      {
        pthread_join(tid[tcounter++], NULL);
      }
      tcounter = 0;
    }
    printf("Accepted connection\n");


    // close(consockfd);/
  }
  close(lstnsockfd);
}
