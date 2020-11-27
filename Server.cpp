#include <unistd.h>
#include <netdb.h>
#include <netinet/in.h>
#include <bits/stdc++.h>
#include <pthread.h>
#include <fstream>
#include <cstdint>

#define MAXREQ 36
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

unordered_map<pthread_t, bool> user_reply_map;
unordered_map<pthread_t, string> requesting_user_map;
unordered_map<pthread_t, string> common_message_map;

unordered_map<pthread_t, pthread_cond_t> cond_map; 
pthread_mutex_t mLock = PTHREAD_MUTEX_INITIALIZER; 

unordered_map<string, vector<Question*>> question_bank;
unordered_map<int, string> fd_user_map;
unordered_map<string, int> user_fd_map;
unordered_map<pthread_t, string> thread_users;
unordered_map<string, pthread_t> user_threads;
unordered_set<string> active_users;
unordered_set<string> available_users;

void sendMessage(int fd, string s)
{
  char* arr = new char[s.size() + 1];
  strcpy(arr, s.c_str());
  write(fd, arr, strlen(arr));
  delete[] arr;
}

void sendMsgToUser(string s, string t)
{
  int fd = user_fd_map[t];
  string sender = thread_users[pthread_self()];

  string res = "FROM " + sender + ": " + s + "\n";
  sendMessage(fd, res);
}

string getMsg(string s)
{
  string res = "";
  for (int i = 16; i < s.size(); i++)
  {
    if (s[i] == 'X')
      break;
    res += s[i];
  }

  return res;
}

string getType(string s)
{
  string res = "";
  for (int i = 0; i < 12; i++)
  {
    if (s[i] == 'X')
      break;
    res += s[i];
  }

  return res;
}

void closeSocket(int &fd)
{
  string user = fd_user_map[fd];
  active_users.erase(user);
  fd_user_map.erase(fd);
  user_fd_map.erase(user);
  thread_users.erase(pthread_self());
  user_threads.erase(user);
  available_users.erase(user);

  string closingMsg = "Socket has been closed.\n";
  sendMessage(fd, closingMsg);
  close(fd);
  pthread_exit(NULL);
}

void MessageHandler(int sig)
{
  string user = thread_users[pthread_self()];
  int fd = user_fd_map[user];

  sendMessage(fd, "PRESS ANT KEY");
}

void CollabHandler(int sig)
{
  string user = thread_users[pthread_self()];
  int fd = user_fd_map[user];

  int n;
  char reqbuf[MAXREQ];

  string req_usr;
  while (1)
  {
    req_usr = requesting_user_map[pthread_self()];
    sendMessage(fd, "Wanna Collaborate with " + req_usr +" [y/n]?");
  
    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);

    string msg = reqbuf;

    string s = getMsg(msg);
    string t = getType(msg);

    if (s == "n\n")
    {
      user_reply_map[pthread_self()] = 0;
      pthread_cond_signal(&cond_map[pthread_self()]); 
      return;
    }
    else if (s == "y\n")
    {
      user_reply_map[pthread_self()] = 1;
      pthread_cond_signal(&cond_map[pthread_self()]); 
      break;
    }
    else
      continue;
  }

  string temp = "Collaboration Established. Please Wait. To send Message, @user:<msg>\n You cannot give answer directly. Only discuss with " + req_usr;
  sendMessage(fd, temp);

  
  while (1)
  {
    while (common_message_map[pthread_self()] == "")
    {
      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1);

      string msg = reqbuf;

      string s = getMsg(msg);
      string t = getType(msg);

      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      if (t == "response" && s == "q")
        return;

      if (s == "CLOSE")
        closeSocket(fd);
    }

    sendMessage(fd, common_message_map[pthread_self()]);
    common_message_map[pthread_self()] = "";
    // pthread_cond_signal(&cond_map[pthread_self()]);

    while (common_message_map[pthread_self()] == "")
    {
      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1);
      
      string msg = reqbuf;

      string s = getMsg(msg);
      string t = getType(msg);

      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      if (t == "response" && s == "q")
        return;

      if (s == "CLOSE")
        closeSocket(fd);
    }

    sendMessage(fd, common_message_map[pthread_self()]);
    common_message_map[pthread_self()] = "";
  }
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
    
    string msg = reqbuf;

    string s = getMsg(msg);
    string t = getType(msg);

    while (t != "response")
    {
      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1); // user-answer

      msg = reqbuf;

      s = getMsg(msg);
      t = getType(msg);
    }

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
    else
    {
      sendMessage(fd, question_topics);
      continue;
    }


    sendMessage(fd, ques->statement);

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1); // user-answer

    msg = reqbuf;

    s = getMsg(msg);
    t = getType(msg);

    while (t != "response")
    {
      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1); // user-answer

      msg = reqbuf;

      s = getMsg(msg);
      t = getType(msg);
    }

    if (s == "CLOSE")
      closeSocket(fd);

    string response = question_topics;
    if (s == ques->answer)
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
    
    string msg = reqbuf;

    string s = getMsg(msg);
    string t = getType(msg);

    while (t != "response")
    {
      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1); // user-answer

      msg = reqbuf;

      s = getMsg(msg);
      t = getType(msg);
    }

    if (s == "CLOSE")
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
    {
      sendMessage(fd, welcomeMsg);
      continue;
    }

    questionFile << topicName + "\n;;\n";

    //////////////////////////

    sendMessage(fd, "\nEnter Problem Statement: \n");

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);

    msg = reqbuf;

    s = getMsg(msg);
    t = getType(msg);

    while (t != "response")
    {
      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1); // user-answer

      msg = reqbuf;

      s = getMsg(msg);
      t = getType(msg);
    }

    if (s == "CLOSE")
      closeSocket(fd);

    string statement = s;

    questionFile << s + ";;\n";

    //////////////////////////

    sendMessage(fd, "\nEnter Answer: \n");

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);

    msg = reqbuf;

    s = getMsg(msg);
    t = getType(msg);

    while (t != "response")
    {
      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1); // user-answer

      msg = reqbuf;

      s = getMsg(msg);
      t = getType(msg);
    }

    if (s == "CLOSE")
      closeSocket(fd);

    string answer = s;

    questionFile << s + ";;\n";

    //////////////////////////

    sendMessage(fd, "\nEnter Explantion: \n");

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);

    msg = reqbuf;

    s = getMsg(msg);
    t = getType(msg);


    while (t != "response")
    {
      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1); // user-answer

      msg = reqbuf;

      s = getMsg(msg);
      t = getType(msg);
    }

    if (s == "CLOSE")
      closeSocket(fd);

    string explantion = s;

    questionFile << s + ";;\n!!\n";

    //////////////////////////

    Question *ques = new Question(statement, answer, explantion);
    question_bank[topicName].push_back(ques);

    sendMessage(fd, welcomeMsg);
  }
}

bool groupMode(int &fd)
{
  if (active_users.size() == 0)
  {
    sendMessage(fd, "No active Users.\n");
    return 1;
  }

  string res = "Active Users are: ";
  for (auto itr = active_users.begin(); itr != active_users.end(); itr++)
  {
    res += *itr + ",";
  }
  res.pop_back();
  res += "\nSpecify user with whom you want to collaborate.\n";

  sendMessage(fd, res);
  
  int n;
  char reqbuf[MAXREQ];

  n = read(fd, reqbuf, MAXREQ);
  
  string msg = reqbuf;

  string s = getMsg(msg);
  string t = getType(msg);

  while (t != "response")
  {
    if (active_users.find(t) != active_users.end())
    {
      sendMsgToUser(s, t);
    }

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1); // user-answer

    msg = reqbuf;

    s = getMsg(msg);
    t = getType(msg);
  }

  string usr = s.substr(0, s.size() - 1);

  pthread_t other_usr_thread_id = user_threads[usr];

  if (active_users.find(usr) != active_users.end())
  {
    // pthread_mutex_mLock(&mLock);
    requesting_user_map[other_usr_thread_id] = thread_users[pthread_self()];
    cond_map[other_usr_thread_id] = PTHREAD_COND_INITIALIZER;
    pthread_kill(user_threads[usr], SIGUSR1);
    pthread_cond_wait(&cond_map[other_usr_thread_id], &mLock);

    if (!user_reply_map[other_usr_thread_id])
    {
      sendMessage(fd, "They Refused to Collaborate.\n");
      return 1;
    }
    else
    {
      sendMessage(fd, "Collaboration Established. To send Message, @user:<msg>\n");
      // pthread_cond_wait(&cond_map[other_usr_thread_id], &mLock);
    }
  }

  string question_topics = "1 Threads\n2 Scheduling\n3 Memory Management\nn next question\nq quit\nr main menu\n";
  
  sendMessage(fd, question_topics);
  while(1)
  {
    // pthread_cond_signal(&cond_map[other_usr_thread_id]);

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1);
    Question *ques;
    
    string msg = reqbuf;

    string s = getMsg(msg);
    string t = getType(msg);

    string other_user;

    while (t != "response")
    {
      if (active_users.find(t) != active_users.end())
      {
        other_user = t;
        sendMsgToUser(s, t);
      }

      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1); // user-answer

      msg = reqbuf;

      s = getMsg(msg);
      t = getType(msg);
    }

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
    else
    {
      sendMessage(fd, question_topics);
      continue;
    }

    common_message_map[other_usr_thread_id] = ques->statement;
    sendMessage(fd, ques->statement);
    pthread_kill(user_threads[usr], SIGUSR2);

    memset(reqbuf,0, MAXREQ);
    n = read(fd, reqbuf, MAXREQ - 1); // user-answer

    msg = reqbuf;

    s = getMsg(msg);
    t = getType(msg);

    while (t != "response")
    {
      if (active_users.find(t) != active_users.end())
      {
        sendMsgToUser(s, t);
      }

      memset(reqbuf,0, MAXREQ);
      n = read(fd, reqbuf, MAXREQ - 1); // user-answer

      msg = reqbuf;

      s = getMsg(msg);
      t = getType(msg);
    }

    if (s == "CLOSE")
        closeSocket(fd);

    string response;
    if (s == ques->answer)
      response = "\nCorrect Answer. Explantion: \n" + ques->explantion + "\n";
    else
      response = "\nWrong Answer. Explantion: \n" + ques->explantion + "\n";

    common_message_map[other_usr_thread_id] = response;
    sendMessage(fd, response + question_topics);
    pthread_kill(user_threads[usr], SIGUSR2);
  }

  return 1;
}

void* server(void* fd) 
{
  int consockfd = *((int *)fd);

  int n;
  char reqbuf[MAXREQ];

  n = read(consockfd, reqbuf, MAXREQ-1);
  
  string userID = "";
  for (int i = 8; i < 20; i++)
  {
    if (reqbuf[i] == 'X')
      break;
    userID += reqbuf[i];
  }

  fd_user_map[consockfd] = userID;
  user_fd_map[userID] = consockfd;
  active_users.insert(userID);
  available_users.insert(userID);
  thread_users[pthread_self()] = userID;
  user_threads[userID] = pthread_self();


  string instructionMsg = "I Indivisual\nG Group\nA Admin\nq Quit\n";
  string welcomeMsg = "Welcome " + userID + " to online quiz on OS\n" + instructionMsg;
  sendMessage(consockfd, welcomeMsg);

  while (1)
  {                 
    memset(reqbuf,0, MAXREQ);
    n = read(consockfd, reqbuf, MAXREQ-1);

    string msg = reqbuf;

    string s = getMsg(msg);
    string t = getType(msg);

    if (active_users.find(t) != active_users.end())
    {
      sendMsgToUser(s, t);
    }

    if (s == "CLOSE\n")
      closeSocket(consockfd);

    else if (s == "I\n")
    {
      active_users.erase(userID);
      bool f = indivisualMode(consockfd);
      active_users.insert(userID);
      if (f == 0)
      {
        closeSocket(consockfd);
      }
      else
      {
        sendMessage(consockfd, instructionMsg);
      }
    }
    else if (s == "G\n")
    {
      active_users.erase(userID);
      groupMode(consockfd);
      active_users.insert(userID);
      sendMessage(consockfd, instructionMsg);
    }
    else if (s == "A\n")
    {
      active_users.erase(userID);
      bool f = adminMode(consockfd);
      active_users.insert(userID);
      if (f == 0)
      {
        closeSocket(consockfd);
      }
      else
      {
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

  lstnsockfd = socket(AF_INET, SOCK_STREAM, 0);

  bind(lstnsockfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr));
  printf("Bounded to port\n");

  pthread_t tid[5];
  int tcounter = 0;
  printf("Listening for incoming connections\n");

  signal(SIGUSR1, CollabHandler);
  signal(SIGUSR2, MessageHandler);

  while (1) 
  {
    listen(lstnsockfd, MAXQUEUE); 

    int newsocketfd = accept(lstnsockfd, (struct sockaddr *) &cli_addr, &clilen);

    pthread_create(&tid[tcounter], NULL, server, &newsocketfd);

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
  }
  close(lstnsockfd);
}
