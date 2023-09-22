#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <unistd.h>

using namespace std;

int main(int argc, char** argv)
{
    int tcpfd, udpfd;
    sockaddr_in servaddr;
    socklen_t slen = sizeof(servaddr);

    tcpfd = socket(AF_INET, SOCK_STREAM, 0);
    udpfd = socket(AF_INET, SOCK_DGRAM, 0);

    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = inet_addr(argv[1]);
    servaddr.sin_port = htons(atoi(argv[2]));

    if(connect(tcpfd, (struct sockaddr*) &servaddr, slen) != 0)
    {
        printf("connection with the server failed...\n");
        exit(0);
    }


    char *input = (char*)malloc(60), *temp = (char*)malloc(60);
    char reply[1024];
	bzero(reply, 0);
	read(tcpfd, reply, 1024);
	printf("%s", reply);

    while(1)
    {
        int pass = 0;
        bzero(reply, 1024);
        bzero(input, 60);
        bzero(temp, 60);
        fgets(input, 60, stdin);
        strcpy(temp, input);
        char *word = strtok(temp, " \n");

        if(!strcmp(word, "register"))
        {
            sendto(udpfd, input, 60, 0, (struct sockaddr*) &servaddr, slen);
            recvfrom(udpfd, reply, 1024, 0, (struct sockaddr*) &servaddr, &slen);
        }
        else if(!strcmp(word, "game-rule"))
        {
            sendto(udpfd, input, 60, 0, (struct sockaddr*) &servaddr, slen);
            recvfrom(udpfd, reply, 1024, 0, (struct sockaddr*) &servaddr, &slen);
        }
        else if(!strcmp(word, "login"))
        {
            send(tcpfd, input, 60, 0);
            recv(tcpfd, reply, 1024, 0);
        }
        else if(!strcmp(word, "logout"))
        {
			send(tcpfd, input, 60, 0);
            recv(tcpfd, reply, 1024, 0);
        }
        else if(!strncmp(word, "start-game", 10))
        {
            send(tcpfd, input, 60, 0);
            recv(tcpfd, reply, 1024, 0);
            if(!strncmp(reply, "Please typing a 4-digit number:", 31))
            {
                printf("%s", reply);
                bzero(reply, 1024);
                char buff[80], tmp[80];
                for (;;) 
                {
                    bzero(buff, sizeof(buff));
                    fgets(buff, 80, stdin);
                    write(tcpfd, buff, sizeof(buff));
                    bzero(buff, sizeof(buff));
                    read(tcpfd, buff, sizeof(buff));
                    printf("%s", buff);
                    bzero(tmp, 80);
                    strcpy(tmp, buff);
                    char *word = strtok(tmp, "lose");
                    if(buff[1] != 'A')
                    {
                        if ((strncmp(buff, "You got the answer!", 19)) == 0)
                        {
                            pass = 1;
                            break;
                        }
                        else if(!(strncmp(buff, "Your guess", 10))){
                            continue;
                        }
                    }
                    else if(buff[6])
                    {
                        pass = 1;
                        break;
                    }
                }
            }
        }
        else if(!strcmp(word, "exit"))
        {
            send(tcpfd, input, 60, 0);
            close(tcpfd);
            close(udpfd);
            break;
        }
        if(pass == 1)
            continue;
        printf("%s\n", reply);
    }
}

