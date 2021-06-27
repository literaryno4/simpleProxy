#include <stdio.h>
#include <signal.h>
#include "../include/read_line.h"
#include "../include/inet_sockets.h"
#include "../include/tlpi_hdr.h"
#include "../include/cache.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400
#define BUF_SIZE 2048
#define MAX_CONN_NUM 10

struct cache caches[MAX_CONN_NUM];

int readnum[MAX_CONN_NUM];

/* You won't lose style points for including this long line in your code */
static const char *user_agent_hdr = "User-Agent: Mozilla/5.0 (X11; Ubuntu; Linux x86_64; rv:79.0) Gecko/20100101 Firefox/79.0\r\n";

void readothhrd(int cfd);

int parseRequest(char *request, char *filename, char *host, char *port);

int proxy(void *pfd);

int main(int argc, const char *argv[])
{
    int lfd, cfd, s;
    int *pfd;
    pthread_t t1;
    struct sockaddr_storage claddr;
    socklen_t addrlen;
    struct sigaction sa;

    sigemptyset(&sa.sa_flags);
    sa.sa_handler = SIG_IGN;
    if (sigaction(SIGPIPE, &sa, NULL) == -1) {
        errExit("sigaction");
    }

    sem_init(&mutex, 0, 1);

    sem_init(&m, 0, 1);

    lfd = inetListen(argv[1], 50, &addrlen);

    for (;;) {
        pfd = malloc(sizeof(int));
        *pfd = accept(lfd, (struct sockaddr *) &claddr, &addrlen);
        if (*pfd == -1) {
            free(pfd);
            continue;
        }

        printf("connected\n");

        s = pthread_create(&t1, NULL, proxy, (void *)pfd);
        if (s != 0) {
            free(pfd);
            continue;
        }
    }

    return 0;
}

int
proxy(void *pfd)
{
    int numRead, cfd, sfd, ind, total;
    char request[BUF_SIZE], filename[BUF_SIZE], host[BUF_SIZE],
             port[BUF_SIZE], srequest[BUF_SIZE + 2048], buf[BUF_SIZE];
    char *object;
    char objectBuf[MAX_OBJECT_SIZE];
    
    cfd = * (int *)pfd;
    free(pfd);

    if ((numRead = readLine(cfd, request, BUF_SIZE)) <= 0) {
        close(cfd);
        return -1;
    }

    printf("%s\n", request);
    readothhrd(cfd);
    
    ind = inCache(request, caches);
    if (ind != -1) {
        if ((object = readCache(caches, ind, readnum)) != NULL) {
            while(write(cfd, object, BUF_SIZE) == BUF_SIZE) {
                object += BUF_SIZE;
            }
            close(cfd);
            return 0;
        }
    }

    parseRequest(request, filename, host, port);

    // parseRequest("GET http://www.cmu.edu/hub/index.html HTTP/1.1", filename, host, port);

    printf("%s %s %s\n", filename, host, port);

    sfd = inetConnect(host, port, SOCK_STREAM);
    if (sfd == -1) {
        close(cfd);
        return -1;
    }

    printf("connected server\n");

    sprintf(srequest, "GET %s HTTP/1.0\r\n", filename);
    write(sfd, srequest, strlen(srequest));
    sprintf(srequest, "Host: %s\r\n", host);
    write(sfd, srequest, strlen(srequest));
    sprintf(srequest, "%s", user_agent_hdr);
    write(sfd, srequest, strlen(srequest));
    sprintf(srequest, "Connection: close\r\n");
    write(sfd, srequest, strlen(srequest));
    sprintf(srequest, "Proxy-Connection: close\r\n\r\n");
    write(sfd, srequest, strlen(srequest));

    object = objectBuf;
    total = 0;
    while ((numRead = readLine(sfd, buf, BUF_SIZE)) > 0) {
        if (write(cfd, buf, numRead) != numRead) {
            close(cfd);
            close(sfd);
            return -1;
        }
        total += numRead;
        printf("total: %d\n", total);
        if (total < MAX_OBJECT_SIZE) {
            strncpy(object, buf, numRead);
            object += numRead;
        }
    }

    if (total < MAX_OBJECT_SIZE) {
        objectBuf[total] = '\0';
        writeCache(caches, request, objectBuf, readnum);
    }
    
    close(cfd);
    close(sfd);

    return 0;
}

int 
parseRequest(char *request, char *filename, char *host, char *port)
{
    char *ptr;
    char method[BUF_SIZE], url[BUF_SIZE], version[BUF_SIZE];

    sscanf(request, "%s %s %s", method, url, version);

    ptr = strstr(url, "http://");
    if (!ptr) {
        if (strstr(url, "https://")){
            sscanf(url, "https://%s", host);
        } else {
            sscanf(url, "%s", host);
        }
    } else {
        sscanf(url, "http://%s", host);
    }

    ptr = index(host, '/');
    if (!ptr) {
        strcpy(filename, "/");
    } else {
        strcpy(filename, ptr);
        *ptr = '\0';
    }

    ptr = index(host, ':');
    if (!ptr) {
        strcpy(port, "80");
    } else {
        strcpy(port, ptr + 1);
        *ptr = '\0';
    }

    return 0;

}

void
readothhrd(int cfd)
{
    char buf[BUF_SIZE];
    readLine(cfd, buf, BUF_SIZE);
    while(strcmp(buf, "\r\n")) {
        readLine(cfd, buf, BUF_SIZE);
        printf("%s", buf);
    }
    return;
}