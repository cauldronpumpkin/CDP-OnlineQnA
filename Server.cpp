#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <pthread.h>
#include <fstream>
#include <cstdint>

#define MAXREQ 20
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
unordered_map<int, string> fd_user_map;
unordered_map<string, pthread_t> thread_users;
unordered_set<string> active_users;
unordered_set<string> available_users;

pthread_t reciever;

void sendMessage(int fd, string s)
{
  char* arr = new char[s.size() + 1];
  strcpy(arr, s.c_str());
  write(fd, arr, strlen(arr));
  delete[] arr;
}

void closeSocket(int &fd)
{
  string s = fd_user_map[fd];
  active_users.erase(s);

  if (available_users.find(s) != available_users.end())
    available_users.erase(s);

  close(fd);
  pthread_exit(NULL);
}

void SIGhandler(int sig)
{
  // if (pthread_self() == reciever)
  // {

  // }
  cout << "Recieved signal\n";
}

bool indivisualMode(int &fd)
{
  int n;
  char reqbuf[MAXREQ];

  string question_topics = "1 Threads\n2 Scheduling\n3 Memory Management\nn next question\nq quit\nr main menu\n";
  
  sendMessage(fd, question_topics);
  while(1)
  {
    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);
    Question *ques;
    
    string s = reqbuf;

    if (s == "CLOSE")
      closeSocket(fd);

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
      sendMessage(fd, question_topics);
      continue;
    }
    else if (s == "q\n")
      return 0;
    else if (s == "r\n")
      return 1;


    sendMessage(fd, ques->statement);

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1); // user-answer

    string user_answer = reqbuf;

    if (user_answer == "CLOSE")
      closeSocket(fd);

    string response = question_topics;
    if (user_answer == ques->answer)
      response = "\nCorrect Answer. Explantion: \n" + ques->explantion + "\n" + response; 
    else
      response = "\nWrong Answer. Explantion: \n" + ques->explantion + "\n" + response;

    sendMessage(fd, response);
  }
}

bool adminMode(int &fd)
{
  int n;
  char reqbuf[MAXREQ];

  string welcomeMsg = "Welcome to Admin Mode. Enter Question Topic.\n\n1 Threads\n2 Scheduling\n3 Memory Management\nq Quit\nr Main Menu\n";
  sendMessage(fd, welcomeMsg);

  ofstream questionFile;
  questionFile.open("question.txt", ios_base::app);

  while(1)
  {
    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);
    
    string s = reqbuf;

    if (s == "CLOSE\n")
      closeSocket(fd);

    //////////////////////////

    string topicName;
    if (s == "1\n")
      topicName = "Threads";
    else if (s == "2\n")
      topicName = "Scheduling";
    else if (s == "3\n")
      topicName = "Memory Management";
    else if (s == "q\n")
      return 0;
    else if (s == "r\n")
      return 1;
    else
      continue;

    questionFile << topicName + "\n;;\n";

    //////////////////////////

    sendMessage(fd, "\nEnter Problem Statement: \n");

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);

    string statement = reqbuf;

    if (statement == "CLOSE\n")
      closeSocket(fd);

    questionFile << statement + ";;\n";

    //////////////////////////

    sendMessage(fd, "\nEnter Answer: \n");

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);

    string answer = reqbuf;

    if (answer == "CLOSE\n")
      closeSocket(fd);

    questionFile << answer + ";;\n";

    //////////////////////////

    sendMessage(fd, "\nEnter Explantion: \n");

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);

    string explantion = reqbuf;

    if (explantion == "CLOSE\n")
      closeSocket(fd);

    questionFile << explantion + ";;\n!!\n";

    //////////////////////////

    Question *ques = new Question(statement, answer, explantion);
    question_bank[topicName].push_back(ques);

    sendMessage(fd, welcomeMsg);
  }
}

bool groupMode(int &fd)
{
  string res = "Active Users are: ";
  for (auto itr = available_users.begin(); itr != available_users.end(); itr++)
  {
    res += *itr + ",";
  }
  res.pop_back();
  res += "\nSpecify user with whom you want to collaborte.\n";

  sendMessage(fd, res);
  
  int n;
  char reqbuf[MAXREQ];

  n = read(fd, reqbuf, MAXREQ);
  string userCollab = reqbuf;

  if (userCollab == "CLOSE\n")
    closeSocket(fd);

  userCollab.pop_back();
  pthread_kill( thread_users[userCollab], SIGUSR1 );

  return 1;
}

void* server(void* fd) 
{
  int consockfd = *((int *)fd);

  int n;
  char reqbuf[MAXREQ];

  n = read(consockfd, reqbuf, MAXREQ);
  
  string userID = "";
  for (int i = 8; i < 20; i++)
  {
    if (reqbuf[i] == 'X')
      break;
    userID += reqbuf[i];
  }

  fd_user_map[consockfd] = userID;
  active_users.insert(userID);
  available_users.insert(userID);
  thread_users[userID] = pthread_self();

  string instructionMsg = "I Indivisual\nG Group\nA Admin\n";
  string welcomeMsg = "Welcome " + to_string(pthread_self()) + " to online quiz on OS\n" + instructionMsg;
  sendMessage(consockfd, welcomeMsg);

  while (1)
  {                 
    memset(reqbuf,0, MAXREQ);
    n = read(consockfd, reqbuf, MAXREQ-1); /* Recv */
    
    string s = reqbuf;

    if (s == "CLOSE\n")
      closeSocket(consockfd);

    if (s == "I\n")
    {
      available_users.erase(userID);
      bool f = indivisualMode(consockfd);
      if (f == 0)
      {
        string closingMsg = "Socket has been closed.\n";
        sendMessage(consockfd, closingMsg);
        close(consockfd);
        pthread_exit(NULL);
      }
      else
      {
        available_users.insert(userID);
        sendMessage(consockfd, instructionMsg);
      }
    }
    else if (s == "G\n")
    {
      available_users.erase(userID);
      bool f = groupMode(consockfd);
      if (f == 0)
      {
        string closingMsg = "Socket has been closed.\n";
        sendMessage(consockfd, closingMsg);
        closeSocket(consockfd);
      }
      else
      {
        available_users.insert(userID);
        sendMessage(consockfd, instructionMsg);
      }
    }
    else if (s == "A\n")
    {
      available_users.erase(userID);
      bool f = adminMode(consockfd);
      if (f == 0)
      {
        string closingMsg = "Socket has been closed.\n";
        sendMessage(consockfd, closingMsg);
        closeSocket(consockfd);
      }
      else
      {
        available_users.insert(userID);
        sendMessage(consockfd, instructionMsg);
      }
    }
    else
    {
      sendMessage(consockfd, instructionMsg);
      continue;
    }

    if (n <= 0) 
      return NULL;
  }
}

int main() 
{
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

  signal(SIGUSR1, SIGhandler);

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
