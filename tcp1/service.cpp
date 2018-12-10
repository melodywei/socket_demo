#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <asm-generic/errno-base.h>
#include <signal.h>
#include <wait.h>
#include <signal.h>

static const int MAXLINE = 4096;
static const int MAXLISTEN = 1024;
static const int PORT = 45678;

int e_socket(int domain, int type, int protocol);
int e_bind(int sockfd, sockaddr_in* myaddr, socklen_t addrlen);
int e_listen(int sockfd, int backlog);
int e_accept(int sockfd, sockaddr_in* myaddr, socklen_t *addrlen);
void my_echo(int connectfd);

int e_socket(int domain, int type, int protocol){
    int sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
    {
        exit(0);
    }
    return sockfd;
}

int e_bind(int sockfd, sockaddr_in* myaddr, socklen_t addrlen){
    int ret = bind(sockfd, (sockaddr*)myaddr, addrlen);
    if (ret != 0){
        close(sockfd);
        exit(0);
    }
    return ret;
}

int e_listen(int sockfd, int backlog){
    int ret = listen(sockfd, backlog);
    if (ret != 0) {
        close(sockfd);
        exit(0);
    }
    return ret;
}

int e_accept(int sockfd, sockaddr_in* myaddr, socklen_t *addrlen){
    int ret = accept(sockfd, (sockaddr*)myaddr, addrlen);
    if (ret == -1)
    {
        printf("%d", errno);
    }
    return ret;
}

void my_echo(int connectfd){
    ssize_t n;
    char buf[MAXLINE] = {0};
    while ((n = read(connectfd, buf, MAXLINE)) > 0) {
        write(connectfd, buf, n);

	    memset(buf, 0, MAXLINE);
        if (n < 0 && errno == EINTR)
            continue;
    }
}

int main(int argc, const char * argv[]) {
    
    int listen_sock = e_socket(AF_INET, SOCK_STREAM, 0);
    
    sockaddr_in servaddr, clientaddr;
    bzero(&servaddr, sizeof(servaddr));
    servaddr.sin_family = AF_INET;
    servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    servaddr.sin_port = htons(PORT);
    
    e_bind(listen_sock, &servaddr, sizeof(servaddr));
    e_listen(listen_sock, MAXLISTEN);
    socklen_t clientlen = 0;
    int clientfd = -1;
    while (true) {
        clientlen = sizeof(clientaddr);
        clientfd = e_accept(listen_sock, &clientaddr, &clientlen);
    
        if (fork() == 0) {
            close(listen_sock);
            my_echo(clientfd);
            exit(0);
        }
        close(clientfd);
    }
    return 0;
}
