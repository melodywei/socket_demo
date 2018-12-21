#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <errno.h>
#include <sys/select.h>

static const int MAXLINE = 4096;

int e_socket(int domain, int type, int protocol);

void my_cli(FILE* fd, int sockfd);

void my_cli(FILE* fd, int sockfd) {
    char sendline[MAXLINE] = {0};
    char recvline[MAXLINE]= {0};

    fd_set read_set;
    int maxfd = 0;
    FD_ZERO(&read_set);

    bool shutwd = false;
    while (true) {
        FD_SET(sockfd, &read_set);

        if (!shutwd) {
            FD_SET(fileno(fd), &read_set);
        }
        maxfd = (sockfd > fileno(fd)) ? (sockfd + 1) : (fileno(fd) + 1);

        if (select(maxfd, &read_set, NULL, NULL, NULL) == -1) {
            return;
        }

        if (FD_ISSET(sockfd, &read_set))
        {
            if (read(sockfd, recvline, MAXLINE) == 0) {
                if (shutwd) {
                    return; 
                }
                printf("service has exit");              
                return;
            }
            fputs(recvline, stdout);
	        memset(recvline, 0, MAXLINE);
            
        }

        if (FD_ISSET(fileno(fd), &read_set)) {
            if (fgets(sendline, MAXLINE, fd) == NULL) {
                shutwd = true;
                shutdown(sockfd, SHUT_WR);
                FD_CLR(fileno(fd), &read_set);
                continue;
            }
            write(sockfd, sendline, strlen(sendline));
        }
          
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
    close(sockfd);
    return 0;
}
