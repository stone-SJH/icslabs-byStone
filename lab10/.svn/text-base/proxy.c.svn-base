/*
 * proxy.c - CS:APP Web proxy
 *
 * TEAM MEMBERS:
 *     Andrew Carnegie, ac00@cs.cmu.edu 
 *     Harry Q. Bovik, bovik@cs.cmu.edu
 * 
 * IMPORTANT: Give a high level description of your code here. You
 * must also provide a header comment at the beginning of each
 * function that describes what that function does.
 */ 
/* proxy.c
 * by  5130379072 shi jiahao
 */
#include "csapp.h"
#include <strings.h>
/*
 * Function prototypes
 */
int parse_uri(char *uri, char *target_addr, char *path, int  *port);
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, char *uri, int size);
ssize_t Rio_readn_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t n);
ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n);
int open_clientfd_ts(char *hostname, int port);
void clienterror(int fd, char *errnum, char *shortmsg, char *longmsg);
void doit(int fd, struct sockaddr_in *sockaddr);
void *thread(void *vargp); 
//Global variables
sem_t s_log;//used in doit, help to lock the write to log file thread
sem_t s_dns;//used in open_clientfd_ts, help to lock the thread
FILE *log_file;//which is open "proxy.log",cuz it is used in doit function, and every thread needs it, so it must be open in main function. but the main function is connected to doit function through thread function, so i set it as a globalvariable in case the thread-safe.
struct client{
    int fd;
    struct sockaddr_in sockaddr;
};
/* 
 * main - Main routine for the proxy program 
 */
int main(int argc, char **argv)
{
    int listenfd, connfd, port, clientlen;
    struct sockaddr_in clientaddr;
    signal(SIGPIPE, SIG_IGN);
    sem_init(&s_log, 0, 1);
    sem_init(&s_dns, 0, 1);
    log_file = fopen("proxy.log","w");
    pthread_t tid;//a unique thread id
    /* Check arguments */
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port number>\n", argv[0]);
        exit(0);
    }
    else port = atoi(argv[1]);
    listenfd = Open_listenfd(port);
    while (1){
	clientlen = sizeof(clientaddr);
	connfd = Accept(listenfd, (SA *)&clientaddr, &clientlen);
	struct client *arg = (struct client *)malloc(sizeof(struct client));
	arg->fd = connfd;
 	arg->sockaddr = clientaddr;
	Pthread_create(&tid, NULL, thread, (void *)arg);
    }
    exit(0);
}
ssize_t Rio_readnb_w(rio_t *rp, void *usrbuf, size_t n){
    ssize_t rc;
    if ((rc = rio_readnb(rp, usrbuf, n)) < 0){
	fprintf(stderr, "error occured in Rio_readnb() function: %s\n", strerror(errno));
	return 0;
    }
    return rc;
}
ssize_t Rio_readlineb_w(rio_t *rp, void *usrbuf, size_t maxlen){
    ssize_t rc;
    if ((rc = rio_readlineb(rp, usrbuf, maxlen)) < 0){
	fprintf(stderr, "error occured in Rio_readlineb() function: %s\n", strerror(errno));
	return 0;
    }
    return rc;
}
ssize_t Rio_writen_w(int fd, void *usrbuf, size_t n){
    ssize_t rc;
    if ((rc = rio_writen(fd, usrbuf, n)) != n){
	fprintf(stderr, "error occured in Rio_writen() function: %s\n", strerror(errno));
	return 0;
    }
    return rc;
}  
void clienterror(int fd, char *errnum, char *shortmsg, char *longmsg){
    char buf[MAXLINE];
    sprintf(buf, "HTTP/1.0 %s %s\r\n", errnum, shortmsg);
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-type: text/html\r\n");
    Rio_writen_w(fd, buf, strlen(buf));
    sprintf(buf, "Content-length: %d\r\n\r\n", strlen(longmsg));
    Rio_writen_w(fd, buf, strlen(buf));
}
int open_clientfd_ts(char *hostname, int port){
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;
    if ((clientfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) return -1;
    P(&s_dns);//lock before build ad server socket cuz gethostbyname() function isnot thread-safe
    if ((hp = gethostbyname(hostname)) == NULL){
	V(&s_dns);
	return -2;
    }
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr_list[0],(char *) &serveraddr.sin_addr.s_addr, hp->h_length);
    V(&s_dns);
    serveraddr.sin_port = htons(port);
    if (connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr)) <0) return -1;
    return clientfd;
}
void *thread(void *vargp){
    struct client *arg = (struct client *) vargp;
    int fd = arg->fd;
    struct sockaddr_in sockaddr = arg->sockaddr;
    free(vargp);//get the information and then free the main-process
    Pthread_detach(Pthread_self());//detach the thread
    doit(fd, &sockaddr);
    Close(fd);
    return 0;
}
void doit(int fd, struct sockaddr_in *sockaddr){
    rio_t rio;
    int port;
    char buf[MAXLINE], method[MAXLINE], uri[MAXLINE], version[MAXLINE];
    char host[MAXLINE], path[MAXLINE];
    Rio_readinitb(&rio, fd); 
    Rio_readlineb_w(&rio, buf, MAXLINE);
    sscanf(buf, "%s %s %s", method, uri, version);
    if (strcasecmp(method, "GET")){
	clienterror(fd, "501", "Not Implemented", "Tiny does not implement this method");
	return;
    }
    if (parse_uri(uri, host, path, &port) != 0){
	clienterror(fd, "400", "Bad Request", "Tiny can not get this request");
        return;
    }
    char request[MAXLINE];
    strcpy(request, method);
    strcat(request, " ");
    strcat(request, path);
    strcat(request, " ");
    strcat(request,version);
    strcat(request,"\r\n");//build a HTTP request as <method> <uri> <version>
    //send request:
    int client_fd = open_clientfd_ts(host, port);
    rio_t server_rio;
    Rio_readinitb(&server_rio, client_fd);
    Rio_writen_w(client_fd, request, strlen(request));
    while(Rio_readlineb_w(&rio, buf, MAXLINE) > 2)//2 means "\r\n"
	Rio_writen_w(client_fd, buf, strlen(buf));
    Rio_writen_w(client_fd, "\r\n", 2);
    //get response and send back:
    int size = 0;
    int temp = 0;
    while ((temp = Rio_readnb_w(&server_rio, buf, MAXLINE)) != 0){
	size += temp;
	Rio_writen_w(fd, buf, temp);
    }
    //write the log file:
    format_log_entry(buf, sockaddr, uri, size);
    P(&s_log);
    fprintf(log_file, "%s", buf);
    fflush(log_file);
    V(&s_log);
    Close(client_fd);//here close the client fd and the server fd will be closed when the thread is quit
}
/*
 * parse_uri - URI parser
 * 
 * Given a URI from an HTTP proxy GET request (i.e., a URL), extract
 * the host name, path name, and port.  The memory for hostname and
 * pathname must already be allocated and should be at least MAXLINE
 * bytes. Return -1 if there are any problems.
 */
int parse_uri(char *uri, char *hostname, char *pathname, int *port)
{
    char *hostbegin = uri + 7;
    char *pathbegin = strchr(hostbegin, '/');
    int len = strlen(uri);

    if (strncasecmp(uri, "http://", 7) != 0) {
        hostname[0] = '\0';
        return -1;
    }
    if (pathbegin == 0){
	strcpy(pathname, "/");
	pathbegin = uri + len;
    }
    else{
	int path_len = len - (pathbegin - uri);
	strncpy(pathname, pathbegin, path_len);
	pathname[path_len] = 0;
    }
    char *portbegin = strchr(hostbegin, ':');
    if (portbegin != 0) *port = atoi(portbegin + 1);
    else{
	*port = 80;
	portbegin = pathbegin;
    }
    int host_len = pathbegin - hostbegin;
    strncpy(hostname, hostbegin, host_len);
    hostname[host_len] = 0;
   return 0;
}
 
    /* Extract the host name */
   // hostbegin = uri + 7;
   // hostend = strpbrk(hostbegin, " :/\r\n\0");
   // len = hostend - hostbegin;
   // strncpy(hostname, hostbegin, len);
   // hostname[len] = '\0';

    /* Extract the port number */
    //*port = 80; /* default */
    //if (*hostend == ':')   
    //    *port = atoi(hostend + 1);

    /* Extract the path */
    //pathbegin = strchr(hostbegin, '/');
   // if (pathbegin == NULL) {
   //     pathname[0] = '\0';
   // }
   // else {
    //    pathbegin++;	
      //  strcpy(pathname, pathbegin);
    //}

    //return 0;
//}

/*
 * format_log_entry - Create a formatted log entry in logstring. 
 * 
 * The inputs are the socket address of the requesting client
 * (sockaddr), the URI from the request (uri), and the size in bytes
 * of the response from the server (size).
 */
void format_log_entry(char *logstring, struct sockaddr_in *sockaddr, 
        char *uri, int size)
{
    time_t now;
    char time_str[MAXLINE];
    unsigned long host;
    unsigned char a, b, c, d;

    /* Get a formatted time string */
    now = time(NULL);
    strftime(time_str, MAXLINE, "%a %d %b %Y %H:%M:%S %Z", localtime(&now));

    /* 
     * Convert the IP address in network byte order to dotted decimal
     * form. Note that we could have used inet_ntoa, but chose not to
     * because inet_ntoa is a Class 3 thread unsafe function that
     * returns a pointer to a static variable (Ch 13, CS:APP).
     */
    host = ntohl(sockaddr->sin_addr.s_addr);
    a = host >> 24;
    b = (host >> 16) & 0xff;
    c = (host >> 8) & 0xff;
    d = host & 0xff;


    /* Return the formatted log entry string */
    sprintf(logstring, "%s: %d.%d.%d.%d %s %d\n", time_str, a, b, c, d, uri, size);
}


