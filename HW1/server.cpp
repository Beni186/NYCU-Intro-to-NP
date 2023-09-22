#include <iostream>
#include <sys/types.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <cstring>
#include <string.h>
#include <strings.h>
#include <pthread.h>
#include <map>
#include <algorithm>
#include <vector>
#include <cstdlib> //rand()
#include <ctime>

using namespace std;

struct User{
    char *username;
    char *email;
    char *password;
    User() {}
    User(char *username, char *email, char *password) {
        this->username = new char[strlen(username) + 1];
        strcpy(this->username, username);
        this->email = new char[strlen(email) + 1];
        strcpy(this->email, email);
        this->password = new char[strlen(password) + 1];
        strcpy(this->password, password);
    }
};

vector<User> user_list;
map <char*, bool> logged;
map <int, char*> fd_user;

void split(vector<char*> &v, char *buf)
{
    char *word, *save_p;
    int cnt = 0;
    word = strtok_r(buf, " \n", &save_p);

    while(word!=NULL)
    {
        word = strtok_r(NULL, " \n", &save_p);
        v.push_back(word);
    }
    //return cnt;
}

void gamerule(int udpfd, struct sockaddr* cliaddr, socklen_t len )
{
    const char *m = "1. Each question is a 4-digit secret number.\n\
2. After each guess, you will get a hint with the following information:\n\
2.1 The number of \"A\", which are digits in the guess that are in the correct position.\n\
2.2 The number of \"B\", which are digits in the guess that are in the answer but are in the wrong position.\n\
The hint will be formatted as \"xAyB\".\n\
3. 5 chances for each question.";
    sendto(udpfd, m, 397, 0, cliaddr, len);
}

void regis(int udpfd, char *buf, struct sockaddr* cliaddr, socklen_t len)
{
    vector<char*> v;
    char *word, *save_p;
    word = strtok_r(buf, " \n", &save_p);
    while(word!=NULL)
    {
        word = strtok_r(NULL, " \n", &save_p);
        v.push_back(word);
    }

    if(v.size() != 4)
    {
        const char *m = "Usage: register <username> <email> <password>";
        sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
        return;
    }

    for(auto u : user_list)
    {
        if(!strcmp(v[0], u.username))
        {
            const char *m = "Username is already used.";
            sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
            return;
        }
        else if(!strcmp(v[1], u.email))
        {
            const char *m = "Email is already used.";
            sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
            return;
        }
    }

    
    User user(v[0], v[1], v[2]);
    user_list.push_back(user);

    const char *m = "Register successfully.";
    sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
    return;
}

void login(int fd, char *buf)
{
    vector<char*> v;
    split(v, buf);

    if(v.size() != 3)
    {
        const char *m = "Usage: login <username> <password>";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }

    map <int, char*>::iterator it;
    it = fd_user.find(fd);
    if(it != fd_user.end())
    {
        if(logged[fd_user[fd]])
        {
            const char *m = "Please logout first.";
            send(fd, m, sizeof(char)*strlen(m), 0);
            return;
        }
    }

    for(auto u : user_list)
    {
        if(!strcmp(v[0], u.username))
        {
            if(logged[u.username])
            {
                const char *m = "Please logout first.";
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
            if(strcmp(v[1], u.password) != 0)
            {
                const char *m = "Password not correct.";
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
            logged[u.username] = true;
            char m[22];
            snprintf(m, 22, "Welcome, %s.", u.username);
            fd_user[fd] = u.username;
            send(fd, m, sizeof(char)*strlen(m), 0);
            return;
        }
    }

    const char *m = "Username not found.";
    send(fd, m, sizeof(char)*strlen(m), 0);
    return;
}

void logout(int fd)
{
    map <int, char*>::iterator it;
    it = fd_user.find(fd);
    if(it != fd_user.end())
    {
        char *uname = fd_user[fd];
        if(!logged[uname])
        {
            const char *m = "Please login first.";
            send(fd, m, sizeof(char)*strlen(m), 0);
            return;
        }
        else
        {
            logged[uname] = false;
            fd_user.erase(fd);
            char *m = (char*)malloc(30);
            memset(m, 0, 30);
            snprintf(m, 30, "Bye, %s", uname);
            send(fd, m, sizeof(char)*strlen(m), 0);
            return;
        }
    }
    else
    {
        const char *m = "Please login first.";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
}

void startgame(int fd, char *ans)
{
    const char *m = "Please typing a 4-digit number:\n";
    send(fd, m, sizeof(char)*strlen(m), 0);

    if(!strncmp(ans, "no", 2))
    {
        bzero(ans, 5);
        srand(time(NULL));
        int question = rand()%(9999 - 1000 + 1) + 1000;
        snprintf(ans, 5, "%d", question);
    }

    char guess[80];
    //bzero(ans, 5);
    // snprintf(ans, 5, "%d", question);
    int cnt = 0;
    int n;
    // infinite loop for chat
    for (;;) {
        int a = 0, b = 0;
        bzero(guess, 80);
        read(fd, guess, sizeof(guess));

        if(guess[5])
        {
            const char *m = "Your guess should be a 4-digit number.\n";
            write(fd, m, 40);
            continue;
        }

        if(!strncmp(ans, guess, 4))
        {
            const char *m = "You got the answer!\n";
            write(fd, m, 21);
            return;
        }

        // int val = atoi(guess);
        // if(val == 0 || val < 1000 || val > 9999)
        // {
        //     const char *m = "Your guess should be a 4-digit number!\n";
        //     write(fd, m, 41);
        //     continue;
        // }
        int flag = 0;
        for(int i=0; i<4; i++)
        {
            if(guess[i] < 48 || guess[i] > 57)
            {
                const char *m = "Your guess should be a 4-digit number.\n";
                send(fd, m, sizeof(char)*strlen(m), 0);
                flag = 1;
                break;
            }
        }

        if(flag)
            continue;
        
        map <int, int> q, g;
        for(int i=0; i<4; i++)
        {
            if(guess[i] == ans[i])
            {
                a++;
                q[i] = 1;
                g[i] = 1;
            }  
        }
            
        for(int i=0; i < 4; i++)
        {
            if(q[i])
                continue;
            for(int j = 0; j < 4 ; j++)
            {
                if(g[j])
                    continue;
                if(guess[j] == ans[i] && i != j)
                {
                    b++;
                    q[i] = 1;
                    g[j] = 1;
                    break;
                }
            }
        }
        q.clear();
        g.clear();

        bzero(guess, 80);
        cnt++;
        if (cnt == 5) {
            snprintf(guess, 80, "%dA%dB\nYou lose the game!\n", a, b);
            write(fd, guess, sizeof(guess));
            break;
        }
        snprintf(guess, 20, "%dA%dB\n", a, b);
        write(fd, guess, sizeof(guess));
    }
    return;
}

void game(int fd, char *buf)
{
    map <int, char*>::iterator it;
    it = fd_user.find(fd);
    if(it != fd_user.end())
    {
        char *uname = fd_user[fd];
        if(!logged[uname])
        {
            const char *m = "Please login first.";
            send(fd, m, sizeof(char)*strlen(m), 0);
            return ;
        }
        else
        {
            vector <char*> v;
            split(v, buf);

            if(v.size() == 1) 
            {
                char ans[5];
                bzero(ans, 5);
                strcpy(ans, "no");
                startgame(fd, ans);
                return ;
            }

            if(v.size() != 2)
            {
                const char *m = "Usage: start-game <4-digit number>";
                send(fd, m, sizeof(char)*strlen(m), 0);
                return ;
            }

            for(int i=0; i<4; i++)
            {
                if(v[0][i] < 48 || v[0][i] > 57)
                {
                    const char *m = "Usage: start-game <4-digit number>";
                    send(fd, m, sizeof(char)*strlen(m), 0);
                    return ;
                }
            }

            if(v[0][4])
            {
                const char *m = "Usage: start-game <4-digit number>";
                send(fd, m, sizeof(char)*strlen(m), 0);
                return ;
            }
            
            // if(val == 0 || val < 1000 || val > 9999)
            // {
            //     const char *m = "Usage: start-game <4-digit number>";
            //     send(fd, m, sizeof(char)*strlen(m), 0);
            //     return ;
            // }
            startgame(fd, v[0]);
            return;
        }
    }
    else
    {
        const char *m = "Please login first.";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
}

void *client(void *arg)
{
    int sockfd = *((int *) arg);

    while(1)
    {
        char buf[100];
        memset(buf, 0, 100);

        char *word, *save_p, *temp = (char*)malloc(30);
        memset(temp, 0, 30);

        recv(sockfd, buf, 100, 0);

        strcpy(temp, buf);
        word = strtok_r(temp, " \n", &save_p);

        if(!strcmp(word, "login"))
            login(sockfd, buf);
        else if(!strncmp(word, "start-game", 10))
            game(sockfd, buf);
        else if(!strcmp(word, "logout"))
            logout(sockfd);
        else if(!strcmp(word, "exit"))
        {
            if(logged[fd_user[sockfd]])
                logged[fd_user[sockfd]] = false;
            cout<<"tcp get msg: exit\n";
            close(sockfd);
            return NULL;
        }
    }
}

int main(int argc, char** argv)
{
    int listenfd, connfd, udpfd, clientfd, client_socket[20], sd;
    int max_sd;
    fd_set set;
    sockaddr_in servaddr, cliaddr;
    socklen_t clilen = sizeof(cliaddr);

    for (int i = 0; i < 20; i++)  
        client_socket[i] = 0;  

    listenfd = socket(AF_INET, SOCK_STREAM, 0);
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);

    int one = 1;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    setsockopt(udpfd, SOL_SOCKET, SO_REUSEADDR, &one, sizeof(one));
    
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
    servaddr.sin_port = htons(atoi(argv[1]));
    
    if ((bind(listenfd, (sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }

    if ((bind(udpfd, (sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }

    cout<<"UDP server is running\n";
    cout<<"TCP server is running\n";
    
    if (listen(listenfd, 10)!=0)
    {
        printf("Listen failed...\n");
        exit(0);
    }

    while(1)  
    {   
        FD_ZERO(&set);
        FD_SET(listenfd, &set);
        FD_SET(udpfd, &set);
        max_sd = std::max(listenfd, udpfd);

        if(select(max_sd+1, &set, NULL, NULL, NULL) < 0)
                    continue;

        if(FD_ISSET(listenfd, &set))
        {
            clientfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
            if(clientfd)
                cout<<"New connection\n";
            const char *m = "*****Welcome to Game 1A2B*****\n";
            write(clientfd, m, 32);
            pthread_t thread;
            pthread_create(&thread, NULL, client, &clientfd);
            pthread_detach(thread);
        }

        if(FD_ISSET(udpfd, &set))
        {
            char buf[100];
            memset(buf, 0, sizeof(buf));
            recvfrom(udpfd, buf, 100, 0, (struct sockaddr*) &cliaddr, &clilen);
            //char *command = strtok(buf, " \n");
            if(strncmp(buf, "game-rule", 9) == 0){
                gamerule(udpfd, (struct sockaddr*) &cliaddr, clilen);
            }
            if(strncmp(buf, "register", 8) == 0)
                regis(udpfd, buf, (struct sockaddr*) &cliaddr, clilen);
        }
    }
}

