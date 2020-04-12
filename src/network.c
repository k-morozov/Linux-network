#include "network.h"


void check_ip(const char *site)
{
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;

    err_status = getaddrinfo(site, NULL, &hints, &servinfo);
    if (err_status!=0)
    {
        printf("errro getaddrinfo: %s\n", gai_strerror(err_status));
        return ;
    }

    char data[INET6_ADDRSTRLEN];
    for(p=servinfo; p!=NULL; p=p->ai_next)
    {
        void *address;
        if (p->ai_family == AF_INET)
        {
            struct sockaddr_in *ipv4 = (struct sockaddr_in*)p->ai_addr;
            address = &ipv4->sin_addr;
        } else 
        {
            struct sockaddr_in6 *ipv6 = (struct sockaddr_in6*)p->ai_addr;
            address = &ipv6->sin6_addr;
        }
        inet_ntop(p->ai_family, address, data, INET6_ADDRSTRLEN);
        printf("%s\n", data);
    }
}

int init_general(const char*node, const char*service, ptr_init_f func)
{
    struct addrinfo hints, *servinfo, *p;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_flags = AI_PASSIVE;

    err_status = getaddrinfo(node, service, &hints, &servinfo);
    if (err_status!=0)
    {
        printf("getaddr failed: %s\n", gai_strerror(err_status));
        return -1;
    }
    int sockfd;
    for(p=servinfo; p!=NULL; p=p->ai_next)
    {
        sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if (sockfd==-1)
        {
            perror("socket failed");
            continue;
        }
        err_status = func(sockfd, p->ai_addr, p->ai_addrlen);
        
        if (err_status==-1)
        {
            perror("ptr function failed");
            continue;
        }
        break;
    }
    if (p==NULL)
    {
        perror("addrinfo failed");
        sockfd = -1;
    }
    freeaddrinfo(servinfo);

    return sockfd;
}

int init_server()
{
    int listener = init_general(NULL, PORT, bind);
    if (listener==-1)
    {
        perror("init server failed");
        exit(1);
    }

    err_status = listen(listener, BACKLOG);
    if (err_status==-1)
    {
        perror("listen failed");
        exit(1);
    }
    printf("server wait connections...\n");

    return listener;
}

int init_client()
{
    int sock_fd = init_general("zverek", PORT, connect);
    if (sock_fd==-1)
    {
        perror("init general failed");
        exit(1);
    }
    char message[MAX_LEN];

    //while (1)
    {
       // int nbytes = recv(sock_fd, message, MAX_LEN, 0);
        int nbytes = read_message(sock_fd, message, MAX_LEN, 7);
        if (nbytes==0)
        {
            printf("nothing recv\n");
        }
        if (nbytes==-1)
        {
            perror("recv failed");
            //break;
        }
        if (nbytes>0)
        {
            message[nbytes] = '\0';
            printf("message: %s\n", message);
        }
    }
    
    close(sock_fd);
    printf("close client\n");
}

void run_server()
{
    int listener = init_server();
    struct sockaddr_storage address;
    socklen_t len_addr = sizeof(address);
    while (1)
    {
        int new_fd = accept(listener, (struct sockaddr *)&address, &len_addr);
        if (new_fd==-1)
        {
            perror("accept failed");
            continue;
        }
        char buf[INET6_ADDRSTRLEN];
        inet_ntop(address.ss_family, get_in_addr((struct sockaddr *)&address), buf, sizeof(buf));
        printf("new connection from %s\n", buf);

        char message[MAX_LEN];
        write_message(message, MAX_LEN, 5);

        int nbytes = send(new_fd, message, strlen(message), 0);
        
        if (nbytes<0)
        {
            perror("send failed");
        }
        if (nbytes==0)
        {
            perror("connetion client close");
        }
        if (nbytes>0)
        {
            printf("success send %d bytes\n", nbytes);
        }
        close(new_fd);
    }
    
}

void* get_in_addr(struct sockaddr * sa)
{
    if (sa->sa_family == AF_INET)
    {
        return &(((struct sockaddr_in*)sa)->sin_addr);
    }
    else 
    {
        return &(((struct sockaddr_in6*)sa)->sin6_addr);
    }
}

void write_message(char *str, int nbytes, int sec)
{
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(STDIN, &readfds);
    int maxfds = STDIN;
    select(maxfds+1, &readfds, NULL, NULL, &tv);

    if (FD_ISSET(STDIN, &readfds))
    {
        int n = read(STDIN, str, nbytes);
        str[n-1] = '\0';
    }
    else 
    {
        strcpy(str, "default message");
    }
}

int read_message(int fd, char *str, int nbytes, int sec)
{
    struct timeval tv;
    tv.tv_sec = sec;
    tv.tv_usec = 0;

    fd_set readfds;
    FD_ZERO(&readfds);
    FD_SET(fd, &readfds);
    int maxfds = fd;
    select(maxfds+1, &readfds, NULL, NULL, &tv);

    if (FD_ISSET(fd, &readfds))
    {
        int nbytes = recv(fd, str, nbytes, 0);
    }
    else
    {
        strcpy(str, "no answer");
    }
    

    return nbytes;
}