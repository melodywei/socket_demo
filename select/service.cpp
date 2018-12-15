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
int e_bind(int sockfd, sockaddr_in *myaddr, socklen_t addrlen);
int e_listen(int sockfd, int backlog);
int e_accept(int sockfd, sockaddr_in *myaddr, socklen_t *addrlen);
void init_clientfd(int sockfd[], int socklen);
void init_sockaddr(sockaddr_in sockaddr);
int append_clientfd(int sockfd[], int socklen, int clientfd);
int handle_client_connect(int listensock, int sockfd[], int socklen, int *max_use_index);
void handle_client_data(fd_set *read_set, fd_set *all_set, int sockfds[], int use_count, int ready_count);

int e_socket(int domain, int type, int protocol)
{
    int sockfd = socket(domain, type, protocol);
    if (sockfd < 0)
    {
        exit(0);
    }
    return sockfd;
}

int e_bind(int sockfd, sockaddr_in *myaddr, socklen_t addrlen)
{
    int ret = bind(sockfd, (sockaddr *)myaddr, addrlen);
    if (ret != 0)
    {
        close(sockfd);
        exit(0);
    }
    return ret;
}

int e_listen(int sockfd, int backlog)
{
    int ret = listen(sockfd, backlog);
    if (ret != 0)
    {
        close(sockfd);
        exit(0);
    }
    return ret;
}

int e_accept(int sockfd, sockaddr_in *myaddr, socklen_t *addrlen)
{
    int ret = accept(sockfd, (sockaddr *)myaddr, addrlen);
    if (ret == -1)
    {
        printf("%d", errno);
        exit(0);
    }
    return ret;
}

void init_clientfd(int sockfd[], int socklen)
{
    for (int i = 0; i < socklen; ++i)
    {
        sockfd[i] = -1;
    }
}

void init_sockaddr(sockaddr_in *sockaddr)
{
    if (!sockaddr)
    {
        return;
    }

    bzero(sockaddr, sizeof(sockaddr_in));
    sockaddr->sin_family = AF_INET;
    sockaddr->sin_addr.s_addr = htonl(INADDR_ANY);
    sockaddr->sin_port = htons(PORT);
}

int append_clientfd(int sockfd[], int socklen, int clientfd)
{
    int i = 0;
    for (i = 0; i < socklen; ++i)
    {
        if (sockfd[i] == -1)
        {
            sockfd[i] = clientfd;
            break;
        }
    }
    if (i == socklen)
    {
        return -1;
    }

    return i;
}

int handle_client_connect(int listensock, int sockfd[], int socklen, int *max_use_index)
{
    sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int connfd = e_accept(listensock, &clientaddr, &clientlen);

    int cur_use = 0;
    if ((cur_use = append_clientfd(sockfd, socklen, connfd)) == -1)
    {
        return -1;
    }

    if (*max_use_index < cur_use)
    {
        *max_use_index = cur_use;
    }

    return connfd;
}

void handle_client_data(fd_set *read_set, fd_set *all_set, int sockfds[], int use_count, int ready_count)
{
    char buf[MAXLINE] = {0};
    for (int i = 0; i <= use_count; ++i) {
        int sockfd = sockfds[i];
        if (sockfd < 0) {
            continue;
        }

        if (FD_ISSET(sockfd, read_set)) {
            int read_len = read(sockfd, buf, MAXLISTEN);
            if (read_len == 0) {
                close(sockfd);
                FD_CLR(sockfd, all_set);
                sockfds[i] = -1;
            } else {
                write(sockfd, buf, read_len);
            }

            if (--ready_count <= 0) {
                break;
            }
        }
    }
}

int main(int argc, const char *argv[])
{

    int listen_sock = e_socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in servaddr;
    init_sockaddr(&servaddr);

    e_bind(listen_sock, &servaddr, sizeof(servaddr));
    e_listen(listen_sock, MAXLISTEN);

    fd_set all_set, read_set;
    int clientfds[FD_SETSIZE];

    init_clientfd(clientfds, FD_SETSIZE);

    FD_ZERO(&all_set);
    FD_ZERO(&read_set);
    FD_SET(listen_sock, &all_set);

    int maxfd = listen_sock + 1;
    int readyfdcount = 0;
    int client_use_count = 0;
    while (true)
    {
        read_set = all_set;
        readyfdcount = select(maxfd, &read_set, NULL, NULL, NULL);

        if (FD_ISSET(listen_sock, &read_set))
        {
            int connfd = handle_client_connect(listen_sock, clientfds, FD_SETSIZE, &client_use_count);

            if (connfd == -1)
            {
                connect;
            }
            FD_SET(connfd, &all_set);

            if (maxfd <= connfd)
            {
                maxfd = connfd + 1;
            }

            if (--readyfdcount <= 0)
            {
                continue;
            }
        }

        handle_client_data(&read_set, &all_set, clientfds, client_use_count, readyfdcount);
    }
    return 0;
}
