#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <sys/stropts.h>

static const int MAXLINE = 4096;
static const int MAXLISTEN = 1024;
static const int PORT = 45678;
static const int OPEN_MAX = 1024;

#define INFTIM -1

int e_socket(int domain, int type, int protocol);
int e_bind(int sockfd, sockaddr_in *myaddr, socklen_t addrlen);
int e_listen(int sockfd, int backlog);
int e_accept(int sockfd, sockaddr_in *myaddr, socklen_t *addrlen);
void init_clientfd(pollfd clients[], int socklen);
void init_sockaddr(sockaddr_in sockaddr);
int append_clientfd(pollfd clients[], int socklen, int clientfd);
int handle_client_connect(pollfd clients[], int socklen, int *max_use_index);
void handle_client_data(pollfd clients[], int use_count, int ready_count);

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

void init_clientfd(pollfd clients[], int socklen)
{
    for (int i = 1; i < socklen; ++i)
    {
        clients[i].fd = -1;
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

int append_clientfd(pollfd clients[], int socklen, int clientfd)
{
    int i = 0;
    for (i = 1; i < socklen; ++i)
    {
        if (clients[i].fd == -1)
        {
            clients[i].fd = clientfd;
            clients[i].events = POLLRDNORM;
            break;
        }
    }
    if (i == socklen)
    {
        return -1;
    }

    return i;
}

int handle_client_connect(pollfd clients[], int socklen, int *max_use_index)
{
    sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int connfd = e_accept(clients[0].fd, &clientaddr, &clientlen);

    int cur_use = 0;
    if ((cur_use = append_clientfd(clients, socklen, connfd)) == -1)
    {
        return -1;
    }

    if (*max_use_index < cur_use)
    {
        *max_use_index = cur_use;
    }

    return connfd;
}

void handle_client_data(pollfd clients[], int use_count, int ready_count)
{
    char buf[MAXLINE] = {0};
    for (int i = 1; i <= use_count; ++i) {
        int sockfd = clients[i].fd;
        if (sockfd < 0) {
            continue;
        }

        if (clients[i].revents & (POLLRDNORM | POLLERR)) {
            int read_len = read(sockfd, buf, MAXLINE);
            if (read_len == 0) {
                close(sockfd);
                clients[i].fd = -1;
            } 
            else if (read_len < 0) {
                if (errno == ECONNRESET) {
                    close(sockfd);
                    clients[i].fd = -1;
                }
                else {
                    exit(0);
                }
            }
            else {
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

    struct pollfd clients[OPEN_MAX];

    clients[0].fd = listen_sock;
    clients[0].events = POLLRDNORM;
    init_clientfd(clients, OPEN_MAX);

    int readycount = 0;
    int max_use = 0;
    while (true)
    {
        readycount = poll(clients, OPEN_MAX, INFTIM);

        if (clients[0].revents & POLLRDNORM)
        {
            handle_client_connect(clients, OPEN_MAX, &max_use);
            if (--readycount <= 0) {
                continue;
            }
        }

        handle_client_data(clients, max_use, readycount);
    }
    return 0;
}
