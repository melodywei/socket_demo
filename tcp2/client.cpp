#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <asm-generic/errno-base.h>

static const int MAXLINE = 4096;

int e_socket(int domain, int type, int protocol);

void my_cli(FILE* fd, int sockfd);

void my_cli(FILE* fd, int sockfd) {
    char sendline[MAXLINE] = {0};
    char recvline[MAXLINE]= {0};
    while (fgets(sendline, MAXLINE, fd) != NULL) {
        write(sockfd, sendline, strlen(sendline));
        
        read(sockfd, recvline, MAXLINE);
        fputs(recvline, stdout);
	    memset(recvline, 0, MAXLINE);
    }
}

int e_socket(int domain, int type, int protocol){
    int sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
    {
        exit(0);
    }
    return sockfd;
}

int main(int argc, const char *argv[])
{
    int sockfd = e_socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servaddr;
    servaddr.sin_family = AF_INET;
    servaddr.sin_port = htons(45678);
    inet_pton(AF_INET, "127.0.0.1", &servaddr.sin_addr);
    connect(sockfd, (sockaddr*)&servaddr, sizeof(servaddr));
    my_cli(stdin, sockfd);
    return 0;
}
