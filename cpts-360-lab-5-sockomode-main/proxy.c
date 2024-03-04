#include <stdio.h>
#include <string.h>
#include "csapp.h"

/* Recommended max cache and object sizes */
#define MAX_CACHE_SIZE 1049000
#define MAX_OBJECT_SIZE 102400

// Default headers
static const char *defaultUserAgent = "User-Agent: Mozilla/5.0 (X11; Linux x86_64; rv:10.0.3) Gecko/20120305 Firefox/10.0.3\r\n";
static const char *connectionHeader = "Connection: close\r\n";
static const char *proxyConnectionHeader = "Proxy-Connection: close\r\n";

// Header keys
static const char *userAgentKey = "User-Agent: ";
static const char *connectionKey = "Connection: ";
static const char *proxyConnectionKey = "Proxy-Connection: ";
static const char *hostKey = "Host: ";

// Function to check if a header contains a specific key
int hasKey(char *header, const char *key) 
{
    int len = strlen(key);
    return strncmp(header, key, len) == 0;
}

// Function to parse URI into hostname, port, and query
void parseUri(char *uri, char *hostname, unsigned int *port, char *query) 
{
    char *ptr;
    if (hasKey(uri, "http://")) 
    {
        ptr = uri + 7; 
    } else {
        ptr = uri;
    }

    // Iterate until port or query section
    while (*ptr != ':' && *ptr != '/') {
        *hostname = *ptr;
        ptr++;
        hostname++;
    }
    *hostname = '\0';  

    // Get port and query
    if (*ptr == ':') 
    {
        sscanf(ptr + 1, "%d%s", port, query);
    } else {
        *port = 80;  
        strcpy(query, ptr);
    }
}

// Function to check if a header is not a standard header
int isOtherHeader(char *header) 
{
    return !(hasKey(header, userAgentKey) || hasKey(header, connectionKey) ||
             hasKey(header, proxyConnectionKey));
}

// Function to create the header string for the server request
void createHeaderString(rio_t rio, char *headers, char *hostname, char *query) 
{
    char buf[MAXLINE], hostHeader[MAXLINE] = "";
    int hostFlag = 1, userAgentFlag = 1;

    // HTTP request header
    sprintf(headers, "GET %s HTTP/1.0\r\n", query);

    // Default value headers
    strcat(headers, connectionHeader);
    strcat(headers, proxyConnectionHeader);

    // Check if the client sent any headers
    if (strcmp(rio.rio_bufptr, "") != 0) {
        // Iterate over existing headers of request to the proxy
        while (Rio_readlineb(&rio, buf, MAXLINE) != 0) 
        {
            if (strcmp(buf, "\r\n") == 0) 
            {
                // Reached end of all headers
                break;
            } else if (hasKey(buf, hostKey)) 
            {
                // Browser overrides Host header
                strcat(headers, buf);
                hostFlag = 0;
            } else if (hasKey(buf, userAgentKey)) 
            {
                // Browser overrides User Agent header
                strcat(headers, buf);
                userAgentFlag = 0;
            } else if (isOtherHeader(buf)) {
                // Any other headers that a browser might include, like Cookies
                strcat(headers, buf);
            }
        }
    }

    // If the Host or User Agent were not set by existing headers
    if (hostFlag) 
    {
        sprintf(hostHeader, "Host: %s\r\n", hostname);
        strcat(headers, hostHeader);
    }
    if (userAgentFlag) 
    {
        strcat(headers, defaultUserAgent);
    }
    strcat(headers, "\r\n");
}

// Function to forward the client's request to the destination server
void forwardRequest(int clientfd, rio_t rio, char *uri) 
{
    int serverfd;
    char hostname[MAXLINE], portStr[8], query[MAXLINE];
    char headers[MAXLINE] = "", response[MAXLINE] = "";
    unsigned int port;
    size_t len;

    parseUri(uri, hostname, &port, query);
    sprintf(portStr, "%d", port);

    if ((serverfd = open_clientfd(hostname, portStr)) < 0) 
    {
        return;
    }

    createHeaderString(rio, headers, hostname, query);

    Rio_readinitb(&rio, serverfd);
    Rio_writen(serverfd, headers, strlen(headers));

    while ((len = Rio_readlineb(&rio, response, MAXLINE)) != 0) {
        Rio_writen(clientfd, response, len);  
    }

    Close(serverfd);
}

// Function to handle the client's proxy request
void handleProxyRequest(int clientfd) 
{
    char buf[MAXLINE], method[8], uri[MAXLINE], version[8];
    rio_t rio;

    Rio_readinitb(&rio, clientfd);
    memset(rio.rio_buf, 0, 8192);  

    // Receive HTTP request
    if (!Rio_readlineb(&rio, buf, MAXLINE)) 
    {
        return;
    }
    printf("%s", buf);
    sscanf(buf, "%s %s %s", method, uri, version);

    if (strcasecmp(method, "GET") != 0) 
    {
        // Discard requests other than HTTP GET
        printf("Proxy can only handle HTTP GET requests\n");
        return;
    }

    forwardRequest(clientfd, rio, uri);
}

int main(int argc, char **argv) 
{
    int listenfd, connfd;
    char clientHostname[MAXLINE], clientPort[MAXLINE];
    socklen_t clientlen;
    struct sockaddr_storage clientaddr;

    if (argc != 2) 
    {
        fprintf(stderr, "usage: %s <port>\n", argv[0]);
        return 1;
    }

    if ((listenfd = open_listenfd(argv[1])) < 0) {
        printf("Unable to open port %s\n", argv[1]);
        return 1;
    }

    while (1) 
    {
        clientlen = sizeof(clientaddr);
        connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
        Getnameinfo((SA *)&clientaddr, clientlen, clientHostname, MAXLINE, clientPort, MAXLINE, 0);
        printf("Success: Accepted connection from (%s, %s)\n", clientHostname, clientPort);
        handleProxyRequest(connfd);
        Close(connfd);
    }

    return 0;
}