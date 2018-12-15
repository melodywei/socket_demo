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

typedef void signfun(int);

int e_socket(int domain, int type, int protocol);
int e_bind(int sockfd, sockaddr_in* myaddr, socklen_t addrlen);
int e_listen(int sockfd, int backlog);
int e_accept(int sockfd, sockaddr_in* myaddr, socklen_t *addrlen);
void my_echo(int connectfd);
void sig_child(int signo);
signfun* sigaction_child(int signo, signfun *func);

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

void sig_child(int signo)
{
    int stat, pid;
    while ((pid = waitpid(-1, &stat, WNOHANG)) < 0){
    } 
    printf("%d exit", pid);
}

signfun* sigaction_child(int signo, signfun *func)
{
    struct sigaction act, oldact;
    act.sa_handler = func;
    sigemptyset(&act.sa_mask);
    act.sa_flags = 0;

    if (sigaction(signo, &act, &oldact) < 0) {
        return SIG_ERR;
    }
    return oldact.sa_handler;
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
    sigaction_child(SIGCHLD, sig_child);
    socklen_t clientlen = 0;
    int clientfd = -1;
    while (true) {
        clientlen = sizeof(clientaddr);
        clientfd = accept(listen_sock, (sockaddr*)&clientaddr, &clientlen);
        if (clientfd < 0) {
            if (errno == EINTR) {
                continue;
            }

            exit(0);
        }
    
        if (fork() == 0) {
            close(listen_sock);
            my_echo(clientfd);
            exit(0);
        }
        close(clientfd);
    }
    return 0;
}
