#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <errno.h>
#include <poll.h>
#include <sys/stropts.h>
#include <sys/epoll.h>

static const int MAXLINE = 4096;
static const int MAXLISTEN = 1024;
static const int PORT = 45678;
static const int OPEN_MAX = 1024;

#define INFTIM -1

struct data {
    int fd;
    char buf[MAXLINE];
};

int e_socket(int domain, int type, int protocol);
int e_bind(int sockfd, sockaddr_in *myaddr, socklen_t addrlen);
int e_listen(int sockfd, int backlog);
int e_accept(int sockfd, sockaddr_in *myaddr, socklen_t *addrlen);
void init_clientfd(pollfd clients[], int socklen);
void init_sockaddr(sockaddr_in sockaddr);
int handle_client_connect(int epfd, int listenfd);
void handle_client_read(int epfd, struct epoll_event *event);
void handle_client_write(int epfd, struct epoll_event *event);

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

int handle_client_connect(int epfd, int listenfd)
{
    sockaddr_in clientaddr;
    socklen_t clientlen = sizeof(clientaddr);
    int connfd = e_accept(listenfd, &clientaddr, &clientlen);

    struct epoll_event ep_event;
    ep_event.data.fd = connfd;
    ep_event.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, connfd, &ep_event);

    return connfd;
}

void handle_client_read(int epfd, struct epoll_event *event)
{
    char buf[MAXLINE] = {0};
    int sockfd = event->data.fd;

    if (sockfd < 0)
    {
        return;
    }

    int read_len = read(sockfd, buf, MAXLINE);
    if (read_len == 0) {
        epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, event);
        close(sockfd);
    }
    else if (read_len < 0) {
        if (errno == ECONNRESET) {
            epoll_ctl(epfd, EPOLL_CTL_DEL, sockfd, event);
            close(sockfd);
        }
        else {
            exit(0);
        }
    }
    else {
        struct data send_data;
        send_data.fd = sockfd;
        strcpy(send_data.buf, buf);

        event->data.ptr = (void*)&send_data;
        event->events = EPOLLOUT;
        epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, event);
    }
}

void handle_client_write(int epfd, struct epoll_event *event)
{
    struct data *send_data = (struct data*)event->data.ptr;
    int sockfd = send_data->fd;

    if (sockfd < 0) {
        printf("write sockfd error");
        exit(-1);
    }

    char *buf = (char *)send_data->buf;
    write(sockfd, buf, strlen(buf));

    event->data.fd = sockfd;
    event->events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_MOD, sockfd, event);
}

int main(int argc, const char *argv[])
{

    int listen_sock = e_socket(AF_INET, SOCK_STREAM, 0);
    sockaddr_in servaddr;
    init_sockaddr(&servaddr);

    e_bind(listen_sock, &servaddr, sizeof(servaddr));
    e_listen(listen_sock, MAXLISTEN);

    int epfd = epoll_create(OPEN_MAX);
    struct epoll_event ep_event;
    ep_event.data.fd = listen_sock;
    ep_event.events = EPOLLIN;
    epoll_ctl(epfd, EPOLL_CTL_ADD, listen_sock, &ep_event);

    struct epoll_event clients[OPEN_MAX];
    while (true)
    {
        int readycount = epoll_wait(epfd, clients, OPEN_MAX, -1);

        for (int i = 0; i < readycount; ++i)
        {
            if (clients[i].data.fd == listen_sock && (clients[i].events & EPOLLIN))
            {
                handle_client_connect(epfd, listen_sock);
            }
            else if (clients[i].events & EPOLLIN)
            {
                handle_client_read(epfd, &clients[i]);
            }
            else if (clients[i].events & EPOLLOUT)
            {
                handle_client_write(epfd, &clients[i]);
            }
        }
    }
    close(epfd);
    return 0;
}
