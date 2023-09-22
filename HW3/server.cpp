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
#include <fstream>

using namespace std;

struct mail
{
    char *inviter;
    char *invitermail;
    unsigned int rid;
    unsigned int rcode;
    mail() {}
    mail(char *inviter, char *invitermail, unsigned int rid, unsigned int rcode) {
        this->inviter = new char[strlen(inviter) + 1];
        strcpy(this->inviter, inviter);
        this->invitermail = new char[strlen(invitermail) + 1];
        strcpy(this->invitermail, invitermail);
        this->rid = rid;
        this->rcode = rcode;
    }
};

struct User{
    char *username;
    char *email;
    char *password;
    vector <mail> mailbox;
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

class room
{
    public:
        char *ans;
        int round;
        int cur_round;
        int cur_player_index;
        char *manager;
        bool ispublic = true;
        bool isplaying = false;
        unsigned int incode;
        unsigned int roomid;
        vector <int> playersfd;
        room() {
            this->ans = new char[5];
            bzero(ans, 5);
            this->manager = new char[128]; 
            bzero(manager, 128);
        }
        room(unsigned int i) { 
            this->roomid = i; 
            this->ans = new char[5];
            bzero(ans, 5);
            this->manager = new char[128]; 
            bzero(manager, 128);
        }
        void broadcast(int fd, char *reply);
};

vector <room*> room_list;
vector <User*> user_list;
map <char*, bool> inroom;
map <char*, unsigned int> user_room;
map <char*, bool> logged;
map <int, char*> fd_user;
map <char*, int> usertofd;
int loginuser = 0;
ofstream out;

void room::broadcast(int fd, char *reply)
{
    for(int i=0; i < this->playersfd.size(); i++)
        if(this->playersfd[i] != fd){
            send(playersfd[i], reply, sizeof(char)*strlen(reply), 0);
        }
        
    return;
}

void split(vector<char*> &v, char *buf);
void gamerule(int udpfd, struct sockaddr* cliaddr, socklen_t len);
void list(int udpfd, char *buf, struct sockaddr* cliaddr, socklen_t len);
void regis(int udpfd, char *buf, struct sockaddr* cliaddr, socklen_t len);
void login(int fd, char *buf);
void logout(int fd);
void createroom(int fd, char *buf);
void *client(void *arg);
void join(int fd, char *buf);
void invite(int fd, char *buf);
void invitation(int fd, char *buf);
void accept(int fd, char *buf);
void leave(int fd, bool not_ex);
void start(int fd, char *buf);
void guess(int fd, char *buf);
void Exit(int fd);
void status(int fd);

int main(int argc, char** argv)
{
    out.open("/home/ubuntu/efs/a.txt");
    out << "0";
    out.close();
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
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(8888);
    
    if ((bind(listenfd, (sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }

    if ((bind(udpfd, (sockaddr *) &servaddr, sizeof(servaddr))) != 0) {
        printf("socket bind failed...\n");
        exit(0);
    }

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
        for(int i=0; i<20; i++)
        {
            sd = client_socket[i];  
            if(sd > 0)  
                FD_SET(sd , &set);  

            if(sd > max_sd)  
                max_sd = sd;  
        }

        if(select(max_sd+1, &set, NULL, NULL, NULL) < 0)
                    continue;

        if(FD_ISSET(listenfd, &set))
        {
            clientfd = accept(listenfd, (struct sockaddr*) &cliaddr, &clilen);
            // if(clientfd)
            //     cout<<clientfd<<" New connection\n";
            for(int i = 0; i<20; i++)
            {
                if(client_socket[i] == 0)
                {
                    client_socket[i] = clientfd;
                    break;
                }     
            }
        }

        if(FD_ISSET(udpfd, &set))
        {
            char buf[100];
            memset(buf, 0, sizeof(buf));
            recvfrom(udpfd, buf, 100, 0, (struct sockaddr*) &cliaddr, &clilen);
            cout<<"udp receive\n";
            if(strncmp(buf, "register", 8) == 0)
                regis(udpfd, buf, (struct sockaddr*) &cliaddr, clilen);
            else if(!strncmp(buf, "list", 4))
                list(udpfd, buf, (struct sockaddr*) &cliaddr, clilen);
        }
        for(int i=0; i<20; i++)
        {
            int sockfd;
            char buf[100];
            memset(buf, 0, 100);
            if((sockfd = client_socket[i]) == 0)
                continue;
            
            if(FD_ISSET(sockfd, &set))
            {
                char buf[100];
                memset(buf, 0, 100);

                char *word, *save_p, *temp = (char*)malloc(30);
                memset(temp, 0, 30);

                int n = recv(sockfd, buf, 100, 0);
                if(n<=0)
                {
                    Exit(sockfd);
                    client_socket[i] = 0;
                    close(sockfd);
                    continue;
                }

                strcpy(temp, buf);
                word = strtok_r(temp, " \n", &save_p);

                if(!strcmp(word, "login"))
                    login(sockfd, buf);
                else if(!strcmp(word, "logout"))
                    logout(sockfd);
                else if(!strcmp(word, "exit"))
                {
                    Exit(sockfd);
                    client_socket[i] = 0;
                    close(sockfd);
                }
                else if(!strncmp(word, "create", 6))
                    createroom(sockfd, buf);
                else if(!strncmp(word, "join", 4))
                    join(sockfd, buf);
                else if(!strncmp(word, "invite", 6))
                    invite(sockfd, buf);
                else if(!strncmp(word, "list", 4))
                    invitation(sockfd, buf);
                else if(!strncmp(word, "accept", 6))
                    accept(sockfd, buf);
                else if(!strncmp(word, "leave", 5))
                    leave(sockfd, true);
                else if(!strncmp(word, "start", 5))
                    start(sockfd, buf);
                else if(!strncmp(word, "guess", 5))
                    guess(sockfd, buf);
                else if(!strncmp(word, "status", 6))
                    status(sockfd);  
            }
        }
    }
}

void status(int fd)
{
    ifstream in;
    printf("Server1: %d\n", loginuser);
    //server2
    in.open("/home/ubuntu/efs/b.txt", ios::in);
    if(!in)
        cout<<"Can't open file!\n";
    int b = 0;
    in>>b;
    printf("Server2: %d\n", b);
    in.close();
    //server3
    in.open("/home/ubuntu/efs/c.txt");
    int c = 0;
    in>>c;
    printf("Server3: %d\n", c);
    in.close();
    char m[96];
    snprintf(m, 96, "Server1: %d\nServer2: %d\nServer3: %d\n", loginuser, b, c);
    send(fd, m, sizeof(char)*strlen(m), 0);
    cout<<"status end!!!\n";
    return;
}

void Exit(int fd)
{
    if(inroom[fd_user[fd]])
        leave(fd, false);
    if(!fd_user.count(fd))
        return;
    if(logged[fd_user[fd]])
    {
        logged[fd_user[fd]] = false;
        loginuser--;
        out.open("/home/ubuntu/efs/a.txt");
        out << loginuser;
        out.close();
    }
    fd_user.erase(fd);
    usertofd.erase(fd_user[fd]);
    return;
}

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
}

bool cmp1(User *user1, User *user2)
{
    return user1->username < user2->username;
}

bool cmp2(room *room1, room *room2)
{
    return room1->roomid < room2->roomid;
}

void list(int udpfd, char *buf, struct sockaddr* cliaddr, socklen_t len)
{
    vector<char*> v;
    split(v, buf);
    if(!strncmp(v[0], "users", 5))
    {
        if(user_list.empty())
        {
            const char* m = "List Users\nNo Users\n";
            sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
            return;
        }
        sort(user_list.begin(), user_list.end(), cmp1);
        string str = "List Users\n";
        int i = 1;
        for(auto u : user_list)
        {
            str.append(to_string(i)).append(". ");
            str.append(u->username).append("<").append(u->email).append("> ");
            if(logged[u->username])
                str.append("Online\n");
            else
                str.append("Offline\n");
            i++;
        }
        const char *m = str.c_str();
        sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
        return;
    }
    else if(!strncmp(v[0], "rooms", 5))
    {
        if(room_list.empty())
        {
            const char* m = "List Game Rooms\nNo Rooms\n";
            sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
            return;
        }
        sort(room_list.begin(), room_list.end(), cmp2);
        string str = "List Game Rooms\n";
        int i = 1;
        for(auto r : room_list)
        {
            str.append(to_string(i)).append(". ");
            if(r->ispublic)
                str.append("(").append("Public) Game Room ").append(to_string(r->roomid));
            else
                str.append("(").append("Private) Game Room ").append(to_string(r->roomid));
            if(r->isplaying)
                str.append(" has started playing\n");
            else
                str.append(" is open for players\n");
            i++;
        }
        const char *m = str.c_str();
        sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
        return;
    }
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
        const char *m = "Usage: register <username> <email> <password>\n";
        sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
        return;
    }

    for(auto u : user_list)
    {
        if(!strcmp(v[0], u->username))
        {
            const char *m = "Username or Email is already used\n";
            sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
            return;
        }
        else if(!strcmp(v[1], u->email))
        {
            const char *m = "Username or Email is already used\n";
            sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
            return;
        }
    }

    
    User user(v[0], v[1], v[2]);
    user_list.push_back(new User(user));

    const char *m = "Register Successfully\n";
    sendto(udpfd, m, sizeof(char)*strlen(m), 0, cliaddr, len);
    return;
}

void login(int fd, char *buf)
{
    vector<char*> v;
    split(v, buf);

    if(v.size() != 3)
    {
        const char *m = "Usage: login <username> <password>\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }

    int flag = 0;
    for(auto u : user_list)
    {
        if(!strcmp(v[0], u->username))
            flag = 1;
    }
    if(!flag)
    {
        const char *m = "Username does not exist\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }

    map <int, char*>::iterator it;
    it = fd_user.find(fd);
    if(it != fd_user.end())
    {
        if(logged[fd_user[fd]])
        {
            string str = "You already logged in as ";
            str.append(fd_user[fd]).append("\n");
            const char *m = str.c_str();
            send(fd, m, sizeof(char)*strlen(m), 0);
            return;
        }
    }

    for(auto u : user_list)
    {
        if(!strcmp(v[0], u->username))
        {
            if(logged[u->username])
            {
                string str = "Someone already logged in as ";
                str.append(u->username).append("\n");
                const char *m = str.c_str();
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
            if(strcmp(v[1], u->password) != 0)
            {
                const char *m = "Wrong password\n";
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
            logged[u->username] = true;
            inroom[u->username] = false;
            fd_user[fd] = u->username;
            usertofd[u->username] = fd;
            char m[96];
            snprintf(m, 96, "Welcome, %s\n", u->username);
            send(fd, m, sizeof(char)*strlen(m), 0);
            loginuser++;
            out.open("/home/ubuntu/efs/a.txt");
            out << loginuser;
            out.close();
            return;
        }
    }
}

void logout(int fd)
{
    if(!fd_user.count(fd))
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!logged[fd_user[fd]])
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(inroom[fd_user[fd]])
    {
        char m[128];
        bzero(m, 128);
        snprintf(m, 128, "You are already in game room %u, please leave game room\n", user_room[fd_user[fd]]);
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    char *m = (char*)malloc(30);
    memset(m, 0, 30);
    snprintf(m, 30, "Goodbye, %s\n", fd_user[fd]);
    send(fd, m, sizeof(char)*strlen(m), 0);
    logged[fd_user[fd]] = false;
    fd_user.erase(fd);
    usertofd.erase(fd_user[fd]);
    loginuser--;
    out.open("/home/ubuntu/efs/a.txt");
    out << loginuser;
    out.close();
    return;
}

void createroom(int fd, char *buf)
{
    map <int, char*>::iterator it;
    it = fd_user.find(fd);
    if(it != fd_user.end());
    else
    {
        string str = "You are not logged in\n";
        const char *m = str.c_str();
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }

    if(!logged[fd_user[fd]])
    {
        string str = "You are not logged in\n";
        const char *m = str.c_str();
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    
    if(inroom[fd_user[fd]])
    {
        char m[128];
        bzero(m, 128);
        snprintf(m, 128, "You are already in game room %u, please leave game room\n", user_room[fd_user[fd]]);
        send(fd, m, sizeof(char)*strlen(m), 0);
            return;
    }
    vector<char*> v;
    split(v, buf);
    if(!strncmp(v[0], "public", 6))
    {
        if(!room_list.empty())
        {
            for(auto u : room_list)
            {
                if(u->roomid == atoi(v[2]))
                {
                    const char *m = "Game room ID is used, choose another one\n";
                    send(fd, m, sizeof(char)*strlen(m), 0);
                    return;
                }
            }
        }
        room R(atoi(v[2]));
        R.playersfd.push_back(fd);
        strcpy(R.manager, fd_user[fd]);
        room_list.push_back(new room(R));
        inroom[fd_user[fd]] = true;
        user_room[fd_user[fd]] = atoi(v[2]);
        char m[128];
        bzero(m, 128);
        snprintf(m, 128, "You create public game room %s\n", v[2]);
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    else if(!strncmp(v[0], "private", 7))
    {
        if(!room_list.empty())
        {
            for(auto u : room_list)
            {
                if(u->roomid == atoi(v[2]))
                {
                    const char *m = "Game room ID is used, choose another one\n";
                    send(fd, m, sizeof(char)*strlen(m), 0);
                    return;
                }
            }
        }
        room R(atoi(v[2]));
        R.playersfd.push_back(fd);
        strcpy(R.manager, fd_user[fd]);
        R.ispublic = false;
        R.incode = atoi(v[3]);
        room_list.push_back(new room(R));
        inroom[fd_user[fd]] = true;
        user_room[fd_user[fd]] = atoi(v[2]);
        char m[128];
        bzero(m, 128);
        snprintf(m, 128, "You create private game room %s\n", v[2]);
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
}

void join(int fd, char *buf)
{
    map <int, char*>::iterator it;
    it = fd_user.find(fd);
    if(it != fd_user.end());
    else
    {
        string str = "You are not logged in\n";
        const char *m = str.c_str();
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }

    if(!logged[fd_user[fd]])
    {
        string str = "You are not logged in\n";
        const char *m = str.c_str();
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }

    if(inroom[fd_user[fd]])
    {
        char m[128];
        bzero(m, 128);
        snprintf(m, 128, "You are already in game room %u, please leave game room\n", user_room[fd_user[fd]]);
        send(fd, m, sizeof(char)*strlen(m), 0);
            return;
    }
    vector<char*> v;
    split(v, buf);
    if(room_list.empty())
    {
        char m[128];
        bzero(m, 128);
        snprintf(m, 128, "Game room %s is not exist\n", v[1]);
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    else
    {
        for(auto r : room_list)
        {
            if(r->roomid == atoi(v[1]))
            {
                if(!r->ispublic)
                {
                    const char *m = "Game room is private, please join game by invitation code\n";
                    send(fd, m, sizeof(char)*strlen(m), 0);
                    return;
                }
                if(r->isplaying)
                {
                    const char *m = "Game has started, you can't join now\n";
                    send(fd, m, sizeof(char)*strlen(m), 0);
                    return;
                }
                inroom[fd_user[fd]] = true;
                user_room[fd_user[fd]] = r->roomid;
                r->playersfd.push_back(fd);
                char reply[128];
                bzero(reply, 128);
                snprintf(reply, 128, "Welcome, %s to game!\n", fd_user[fd]);
                r->broadcast(fd, reply);
                char m[128];
                bzero(m, 128);
                snprintf(m, 128, "You join game room %u\n", r->roomid);
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
        }
        char m[128];
        bzero(m, 128);
        snprintf(m, 128, "Game room %s is not exist\n", v[1]);
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
}

void invite(int fd, char*buf)
{
    map <int, char*>::iterator it;
    it = fd_user.find(fd);
    if(it != fd_user.end());
    else
    {
        string str = "You are not logged in\n";
        const char *m = str.c_str();
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }

    if(!logged[fd_user[fd]])
    {
        string str = "You are not logged in\n";
        const char *m = str.c_str();
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }

    if(!inroom[fd_user[fd]])
    {
        string str = "You did not join any game room\n";
        const char *m = str.c_str();
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    for(auto r : room_list)
    {
        if(!strcmp(fd_user[fd], r->manager))
        {
            if(r->ispublic)
            {
                const char *m = "You are not private game room manager\n";
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
            else
            {
                vector<char*> v;
                split(v, buf);
                char mmail[64];
                bzero(mmail, 64);
                for(auto it : user_list)
                    if(!strcmp(it->username, fd_user[fd]))
                        strcpy(mmail, it->email);

                for(auto u : user_list)
                {
                    if(!strcmp(v[0], u->email))
                    {
                        if(!logged[u->username])
                        {
                            const char *m = "Invitee not logged in\n";
                            send(fd, m, sizeof(char)*strlen(m), 0);
                            return;
                        }
                        else
                        {
                            char c[1024];
                            bzero(c, 1024);
                            snprintf(c, 1024, "%s<%s> invite you to join game room %u,  invitation code is %u\n", fd_user[fd], mmail, r->roomid, r->incode);
                            mail mm(fd_user[fd], mmail, r->roomid, r->incode);
                            u->mailbox.push_back(mm);
                            char m[1024];
                            bzero(m, 1024);
                            snprintf(m, 1024, "You receive invitation from %s<%s>\n", fd_user[fd], mmail);
                            send(usertofd[u->username], m, sizeof(char)*strlen(m), 0);
                            bzero(m, 1024);
                            snprintf(m, 1024, "You send invitation to %s<%s>\n", u->username, u->email);
                            send(fd, m, sizeof(char)*strlen(m), 0);
                            return;
                        }
                    }
                }
            }
        }
    }
    const char *m = "You are not private game room manager\n";
    send(fd, m, sizeof(char)*strlen(m), 0);
    return;
}

bool cmp3(mail &m1, mail &m2)
{
    return m1.rid < m2.rid;
}

void invitation(int fd, char*buf)
{
    if(!fd_user.count(fd))
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!logged[fd_user[fd]])
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    for(auto u : user_list)
    {
        if(!strcmp(u->username, fd_user[fd]))
        {
            if(u->mailbox.empty())
            {
                const char *m = "List invitations\nNo Invitations\n";
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
            else
            {
                string str = "List invitations\n";
                int i=1;
                sort(u->mailbox.begin(), u->mailbox.end(), cmp3);
                map <unsigned int, bool> mp;
                for(auto mm : u->mailbox)
                {
                    if(mp.count(mm.rid))
                        continue;
                    str.append(to_string(i)).append(". ").append(mm.inviter).append("<").append(mm.invitermail).append("> invite you to join game room ");
                    str.append(to_string(mm.rid)).append(", invitation code is ").append(to_string(mm.rcode)).append("\n");
                    i++;
                    mp[mm.rid] = true;
                }
                const char *m = str.c_str();
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
            
        }
    }
}

void accept(int fd, char*buf)
{
    if(!fd_user.count(fd))
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!logged[fd_user[fd]])
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(inroom[fd_user[fd]])
    {
        char m[128];
        bzero(m, 128);
        snprintf(m, 128, "You are already in game room %u, please leave game room\n", user_room[fd_user[fd]]);
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    vector<char*> v;
    split(v, buf);
    for(auto u : user_list)
    {
        if(!strcmp(u->username, fd_user[fd]))
        {
            for(auto mm : u->mailbox)
            {
                if(!strcmp(mm.invitermail, v[0]))
                {
                    if(mm.rcode == atoi(v[1]))
                    {
                        for(auto r : room_list)
                        {
                            if(r->incode == atoi(v[1]))
                            {
                                if(r->isplaying)
                                {
                                    const char *m = "Game has started, you can't join now\n";
                                    send(fd, m, sizeof(char)*strlen(m), 0);
                                    return;
                                }
                                //success 
                                inroom[fd_user[fd]] = true;
                                user_room[fd_user[fd]] = r->roomid;
                                r->playersfd.push_back(fd);
                                char reply[128];
                                bzero(reply, 128);
                                snprintf(reply, 128, "Welcome, %s to game!\n", fd_user[fd]);
                                r->broadcast(fd, reply);
                                char m[128];
                                bzero(m, 128);
                                snprintf(m, 128, "You join game room %u\n", r->roomid);
                                send(fd, m, sizeof(char)*strlen(m), 0);
                                return;
                            }
                        }
                        const char *m = "Invitation not exist\n";
                        send(fd, m, sizeof(char)*strlen(m), 0);
                        return;
                    }
                    else
                    {
                        const char *m = "Your invitation code is incorrect\n";
                        send(fd, m, sizeof(char)*strlen(m), 0);
                        return;
                    }
                }
            }
            const char *m = "Invitation not exist\n";
            send(fd, m, sizeof(char)*strlen(m), 0);
            return;
        }
    }
}

void deletemail(unsigned int roomid)
{
    for(auto u : user_list)
    {
        for(vector<mail>::iterator it = u->mailbox.begin(); it!=u->mailbox.end();)
        {
            if(roomid == it->rid)
                u->mailbox.erase(it);
            else
                it++;
        }
    }
}

void leave(int fd, bool not_ex)
{
    if(!fd_user.count(fd))
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!logged[fd_user[fd]])
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!inroom[fd_user[fd]])
    {
        const char *m = "You did not join any game room\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    vector <room*>::iterator r, it;
    for(r = room_list.begin(); r!=room_list.end(); r++)
    {
        if(user_room[fd_user[fd]] == (*r)->roomid)
        {
            if(!strcmp((*r)->manager, fd_user[fd]))
            {
                if(not_ex)
                {
                    char reply[128];
                    bzero(reply, 128);
                    snprintf(reply, 128, "Game room manager leave game room %u, you are forced to leave too\n", (*r)->roomid);
                    (*r)->broadcast(fd, reply);
                    char m[128];
                    bzero(m, 128);
                    snprintf(m, 128, "You leave game room %u\n", (*r)->roomid);
                    send(fd, m, sizeof(char)*strlen(m), 0);
                }
                for(int i=0; i<(*r)->playersfd.size(); i++)
                {
                    inroom[fd_user[(*r)->playersfd[i]]] = false;
                    user_room.erase(fd_user[(*r)->playersfd[i]]);
                }
                deletemail((*r)->roomid);
                it = r;
                break;
            }
            else
            {
                if((*r)->isplaying)
                {
                    (*r)->isplaying = false;
                    if(not_ex)
                    {
                        char reply[128];
                        bzero(reply, 128);
                        snprintf(reply, 128, "%s leave game room %u, game ends\n", fd_user[fd], (*r)->roomid);
                        (*r)->broadcast(fd, reply);
                    }
                    vector <int>::iterator it;
                    it = find((*r)->playersfd.begin(), (*r)->playersfd.end(), fd);
                    if(it != (*r)->playersfd.end())
                        (*r)->playersfd.erase(it);
                    if(not_ex)
                    {
                        char m[128];
                        bzero(m, 128);
                        snprintf(m, 128, "You leave game room %u, game ends\n", (*r)->roomid);
                        send(fd, m, sizeof(char)*strlen(m), 0);
                    }
                    inroom[fd_user[fd]] = false;
                    user_room.erase(fd_user[fd]);
                    return;
                }
                else
                {
                    if(not_ex)
                    {
                        char reply[128];
                        bzero(reply, 128);
                        snprintf(reply, 128, "%s leave game room %u\n", fd_user[fd], (*r)->roomid);
                        (*r)->broadcast(fd, reply);
                    }
                    vector <int>::iterator it;
                    it = find((*r)->playersfd.begin(), (*r)->playersfd.end(), fd);
                    if(it != (*r)->playersfd.end())
                        (*r)->playersfd.erase(it);
                    if(not_ex)
                    {
                        char m[128];
                        bzero(m, 128);
                        snprintf(m, 128, "You leave game room %u\n", (*r)->roomid);
                        send(fd, m, sizeof(char)*strlen(m), 0);
                    }
                    inroom[fd_user[fd]] = false;
                    user_room.erase(fd_user[fd]);
                    return;
                }
            }
        }
    }
    room_list.erase(it);
    return;
}

void start(int fd, char *buf)
{
    if(!fd_user.count(fd))
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!logged[fd_user[fd]])
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!inroom[fd_user[fd]])
    {
        const char *m = "You did not join any game room\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    vector<char*> v;
    split(v, buf);
    for(auto r : room_list)
    {
        if(!strcmp(r->manager, fd_user[fd]))
        {
            if(r->isplaying)
            {
                const char *m = "Game has started, you can't start again\n";
                send(fd, m, sizeof(char)*strlen(m), 0);
                return;
            }
            else
            {
                if(v.size() == 4)
                {
                    for(int i=0; i<4; i++)
                    {
                        if(v[2][i] < 48 || v[2][i] > 57)
                        {
                            const char *m = "Please enter 4 digit number with leading zero\n";
                            send(fd, m, sizeof(char)*strlen(m), 0);
                            return ;
                        }
                    }
                    if(v[2][4])
                    {
                        const char *m = "Please enter 4 digit number with leading zero\n";
                        send(fd, m, sizeof(char)*strlen(m), 0);
                        return ;
                    }
                    strcpy(r->ans, v[2]);
                }
                else
                {
                    char ans[5];
                    bzero(ans, 5);
                    srand(time(NULL));
                    int question = rand()%(9999 - 1000 + 1) + 1000;
                    snprintf(ans, 5, "%d", question);
                    strcpy(r->ans, ans);
                }
                r->isplaying = true;
                r->round = atoi(v[1]);
                r->cur_round = 0;
                r->cur_player_index = 0;
                char reply[128];
                bzero(reply, 128);
                snprintf(reply, 128, "Game start! Current player is %s\n", fd_user[r->playersfd[0]]);
                r->broadcast(fd, reply);
                send(fd, reply, sizeof(char)*strlen(reply), 0);
                return;
            }
        }
    }
    const char *m = "You are not game room manager, you can't start game\n";
    send(fd, m, sizeof(char)*strlen(m), 0);
    return;
}

void guess(int fd, char *buf)
{
    if(!fd_user.count(fd))
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!logged[fd_user[fd]])
    {
        const char *m = "You are not logged in\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    if(!inroom[fd_user[fd]])
    {
        const char *m = "You did not join any game room\n";
        send(fd, m, sizeof(char)*strlen(m), 0);
        return;
    }
    for(auto r : room_list)
    {
        if(r->roomid == user_room[fd_user[fd]])
        {
            if(r->isplaying)
            {
                if(r->playersfd[r->cur_player_index] != fd)
                {
                    char m[128];
                    bzero(m, 128);
                    snprintf(m, 128, "Please wait..., current player is %s\n", fd_user[r->playersfd[r->cur_player_index]]);
                    send(fd, m, sizeof(char)*strlen(m), 0);
                    return;
                }
                else
                {
                    vector<char*> v;
                    split(v, buf);
                    for(int i=0; i<4; i++)
                    {
                        if(v[0][i] < 48 || v[0][i] > 57)
                        {
                            const char *m = "Please enter 4 digit number with leading zero\n";
                            send(fd, m, sizeof(char)*strlen(m), 0);
                            return ;
                        }
                    }
                    if(v[0][4])
                    {
                        const char *m = "Please enter 4 digit number with leading zero\n";
                        send(fd, m, sizeof(char)*strlen(m), 0);
                        return ;
                    }
                    //success
                    if(!strncmp(r->ans, v[0], 4))
                    {
                        char m[128];
                        bzero(m, 128);
                        snprintf(m, 128, "%s guess '%s' and got Bingo!!! %s wins the game, game ends\n", fd_user[fd], v[0], fd_user[fd]);
                        r->broadcast(fd, m);
                        send(fd, m, sizeof(char)*strlen(m), 0);
                        r->isplaying = false;
                        return;
                    }
                    else
                    {
                        int a = 0, b = 0;
                        map <int, int> q, g;
                        for(int i=0; i<4; i++)
                        {
                            if(v[0][i] == r->ans[i])
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
                                if(v[0][j] == r->ans[i] && i != j)
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

                        r->cur_player_index++;
                        if(r->cur_player_index == r->playersfd.size())
                        {
                            r->cur_round++;
                            r->cur_player_index = 0;
                        }
                        char m[256];
                        bzero(m, 256);
                        snprintf(m, 256, "%s guess '%s' and got '%dA%dB'\n", fd_user[fd], v[0], a, b);
                        if(r->round == r->cur_round)
                        {
                            const char *cat = "Game ends, no one wins\n";
                            strcat(m, cat);
                            r->isplaying = false;
                        }
                        r->broadcast(fd, m);
                        send(fd, m, sizeof(char)*strlen(m), 0);
                        return;
                    }
                }
            }
            else
            {
                if(!strcmp(r->manager, fd_user[fd]))
                {
                    const char *m = "You are game room manager, please start game first\n";
                    send(fd, m, sizeof(char)*strlen(m), 0);
                    return;
                }
                else
                {
                    const char *m = "Game has not started yet\n";
                    send(fd, m, sizeof(char)*strlen(m), 0);
                    return;
                }
            }
        }
    }
}