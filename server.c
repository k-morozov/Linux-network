#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netdb.h>
#include <signal.h>
#include <string.h>
#include <sys/wait.h>
#include <arpa/inet.h>
#include <stdlib.h>
#include <unistd.h>

#define PORT ("3491")
#define BACKLOG (10)
#define MAXDATASIZE 256
#define UNUSADE(x)  ((x)==(x))

void sigchld_handler(int s) {
    UNUSADE(s);
    while(waitpid(-1, NULL, WNOHANG)>0);
}
void *get_in_addr(struct sockaddr* sa) {
    if (sa->sa_family == AF_INET) {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    return &(((struct sockaddr_in6*)sa)->sin6_addr);
}
void print_hostname(void) {
    char host_name[MAXDATASIZE];
    int er;
    if ((er=gethostname(host_name, MAXDATASIZE))==-1) {
        perror("error name");
        exit(1);
    }
    printf("Host-name: %s\n", host_name);
}

struct addrinfo* init_server(void) {
    struct addrinfo hints, *server_info;

    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;            // my IP

    int rv;
    if ((rv=getaddrinfo(NULL, PORT, &hints, &server_info))==-1) {
        fprintf(stderr, "bad getaddrinfo: %s\n\n", gai_strerror(rv));
        exit(1);
    }

    return server_info;
}

int init_socket(struct addrinfo *server_info) {
    int socketfd;
    int yes=1;
    struct addrinfo *p;

    for(p = server_info; p!=NULL; p=p->ai_next) {
        if((socketfd=socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror(("socket::server"));
            return -1;
        }
        // read
        if (setsockopt(socketfd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == 1) {
            perror("setsockopt");
            exit(1);
        }
        if (bind(socketfd, p->ai_addr, p->ai_addrlen) == 1) {
            close(socketfd);
            perror("bind");
            return -1;
        }
        break;
    }

    if (p==NULL) {
        fprintf(stderr, "server failed to bind\n");
        return 3;
    }
    freeaddrinfo(server_info);

    return socketfd;
}

//*******************************************************************************************

int main(int argc, char *argv[]) {
    UNUSADE(argc);
    UNUSADE(argv);

    int newfd;
    struct sockaddr_storage their_addr;

    socklen_t sin_size;
    struct sigaction sa;
    char s[INET6_ADDRSTRLEN];

    print_hostname();
    struct addrinfo *server_info = init_server();
    int socketfd = init_socket(server_info);

    // ====================================================================================
    if (listen(socketfd, BACKLOG)==-1) {
        perror("listen");
        return 4;
    }
    sa.sa_handler = sigchld_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = SA_RESTART;
    if(sigaction(SIGCHLD, &sa, NULL)==-1) {
        perror("sigaction");
        return 5;
    }
    // ************************************************************************************

    printf("Server wait to connections...\n");

    while (1) {
        sin_size = sizeof(their_addr);
        newfd = accept(socketfd, (struct sockaddr*)&their_addr, &sin_size);
        if (newfd==-1){
            perror("accept");
            continue;
        }
        inet_ntop(their_addr.ss_family, get_in_addr((struct sockaddr*)&their_addr), s, sizeof(s));
        printf("Server got a connection from %s\n", s);

        if (!fork()) {
            printf("work fork\n");
            close(socketfd);
            if (send(newfd, "my first socket-server", 22, 0) == -1) {
                perror("send");
            }
            close(newfd);
            exit(0);
        }

        /*
        if (send(newfd, "my first socket-server", 22, 0) == -1) {
            perror("send");
        }
        */
        close(newfd);
    }
    return 0;
}
