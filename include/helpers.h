#ifndef HELPERS_H
#define HELPERS_H


#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <ctype.h>
#include <setjmp.h>
#include <signal.h>
#include <sys/time.h>
#include <sys/types.h>
 #include <sys/wait.h>
 #include <sys/stat.h>
 #include <fcntl.h>
 #include <sys/mman.h>
 #include <errno.h>
 #include <math.h>
 #include <pthread.h>
 #include <semaphore.h>
 #include <sys/socket.h>
 #include <netdb.h>
 #include <netinet/in.h>
 #include <arpa/inet.h>


#define BUFFER_SIZE 1024
#define SA struct sockaddr


/*************************
 * Error-handling functions
 **************************/
 void unix_error(char *msg) /* unix-style error */
 {
    fprintf(stderr, "%s: %s\n", msg, strerror(errno));
    exit(0);
 }

 void posix_error(int code, char *msg) /* posix-style error */
 {
    fprintf(stderr, "%s: %s\n", msg, strerror(code));
    exit(0);
 }

 void dns_error(char *msg) /* dns-style error */
 {
    fprintf(stderr, "%s: %s\n", msg, hstrerror(h_errno));
    exit(0);
 }


 /*********************************************
 * Wrappers for Unix process control functions
 ********************************************/


 

 pid_t Wait(int *status)
 {
    pid_t pid;
    if ((pid = wait(status)) < 0)
        unix_error("Wait error");
    return pid;
 }

 pid_t Waitpid(pid_t pid, int *iptr, int options)
 {
    pid_t retpid;

    if ((retpid = waitpid(pid, iptr, options)) < 0)
        unix_error("Waitpid error");
    return(retpid);
 }


 /********************************
 * Wrappers for Unix I/O routines
 ********************************/

 int Open(const char *pathname, int flags, mode_t mode)
 {
    int rc;

    if ((rc = open(pathname, flags, mode)) < 0)
        unix_error("Open error");
    return rc;
 }

 ssize_t Read(int fd, void *buf, size_t count)
 {
    ssize_t rc;

    if ((rc = read(fd, buf, count)) < 0)
        unix_error("Read error");
    return rc;
 }

 ssize_t Write(int fd, const void *buf, size_t count)
 {
    ssize_t rc;

    if ((rc = write(fd, buf, count)) < 0)
        unix_error("Write error");
    return rc;
 }

 off_t Lseek(int fildes, off_t offset, int whence)
 {
    off_t rc;

    if ((rc = lseek(fildes, offset, whence)) < 0)
        unix_error("Lseek error");
    return rc;
 }
void Close(int fd)
 {
    int rc;

    if ((rc = close(fd)) < 0)
        unix_error("Close error");
 }


/************************************************
 * Wrappers for Pthreads thread control functions
 ************************************************/

 void Pthread_create(pthread_t *tidp, pthread_attr_t *attrp, void * (*routine)(void *), void *argp)
 {
    int rc;

    if ((rc = pthread_create(tidp, attrp, routine, argp)) != 0)
        posix_error(rc, "Pthread_create error");
 }

 void Pthread_cancel(pthread_t tid) {
    int rc;

    if ((rc = pthread_cancel(tid)) != 0)
        posix_error(rc, "Pthread_cancel error");
 }

 void Pthread_join(pthread_t tid, void **thread_return) {
    int rc;

    if ((rc = pthread_join(tid, thread_return)) != 0)
        posix_error(rc, "Pthread_join error");
 }

 void Pthread_detach(pthread_t tid) {
    int rc;

    if ((rc = pthread_detach(tid)) != 0)
        posix_error(rc, "Pthread_detach error");
 }

 void Pthread_exit(void *retval) {
    pthread_exit(retval);
 }


 /*************************************************************
 * Wrappers for Pthreads mutex and condition variable functions
 ************************************************************/

 void Pthread_mutex_init(pthread_mutex_t *mutex, pthread_mutexattr_t *attr)
 {
    int rc;

    if ((rc = pthread_mutex_init(mutex, attr)) != 0)
        posix_error(rc, "Pthread_mutex_init error");
 }

 void Pthread_mutex_lock(pthread_mutex_t *mutex)
 {
    int rc;

    if ((rc = pthread_mutex_lock(mutex)) != 0)
        posix_error(rc, "Pthread_mutex_lock error");
 }

 void Pthread_mutex_unlock(pthread_mutex_t *mutex)
 {
    int rc;

    if ((rc = pthread_mutex_unlock(mutex)) != 0)
        posix_error(rc, "Pthread_mutex_unlock error");
 }
/*****************************
 * Wrappers for Posix semaphores
 *******************************/

 void Sem_init(sem_t *sem, int pshared, unsigned int value)
 {
    if (sem_init(sem, pshared, value) < 0)
        unix_error("Sem_init error");
 }

 void P(sem_t *sem)
 {
    if (sem_wait(sem) < 0)
        unix_error("P error");
 }

 void V(sem_t *sem)
 {
    if (sem_post(sem) < 0)
        unix_error("V error");
 }

 /****************************
 * Sockets interface wrappers
 ****************************/

 int Socket(int domain, int type, int protocol)
 {
    int rc;

    if ((rc = socket(domain, type, protocol)) < 0)
    {
        unix_error("Socket error");
        exit(1);
    }
    return rc;
 }

 void Setsockopt(int s, int level, int optname, const void *optval, int optlen)
 {
    int rc;

    if ((rc = setsockopt(s, level, optname, optval, optlen)) < 0)
    {
        unix_error("Setsockopt error");
        exit(1);
    }
 }

 void Bind(int sockfd, struct sockaddr *my_addr, int addrlen)
 {
    int rc;

    if ((rc = bind(sockfd, my_addr, addrlen)) < 0)
    {
        unix_error("Bind error");
        exit(1);
    }
 }

 void Listen(int s, int backlog)
 {
    int rc;

    if ((rc = listen(s, backlog)) < 0)
    {
        unix_error("Listen error");
        exit(1);
    }

 }

 int Accept(int s, struct sockaddr *addr, socklen_t addrlen)
 {
    int rc;

    if ((rc = accept(s, addr, &addrlen)) < 0)
    {
        unix_error("Accept error");
        exit(1);
    }
    return rc;
 }

 void Connect(int sockfd, struct sockaddr *serv_addr, int addrlen)
 {
    int rc;

    if ((rc = connect(sockfd, serv_addr, addrlen)) < 0)
    {
        unix_error("Connect error");
        exit(1);
    }
 }

 /************************
 * DNS interface wrappers
 ***********************/

 struct hostent *Gethostbyname(const char *name)
 {
    struct hostent *p;

    if ((p = gethostbyname(name)) == NULL)
        dns_error("Gethostbyname error");
    return p;
 }

 struct hostent *Gethostbyaddr(const char *addr, int len, int type)
 {
    struct hostent *p;

    if ((p = gethostbyaddr(addr, len, type)) == NULL)
        dns_error("Gethostbyaddr error");
    return p;
 }


 /********************************
 * Client/server helper functions
 ********************************/
 /*
 * open_clientfd - open connection to server at <hostname, port>
 * and return a socket descriptor ready for reading and writing.
 */
 int open_clientfd(char *hostname, int port)
 {
    int clientfd;
    struct hostent *hp;
    struct sockaddr_in serveraddr;

    clientfd = Socket(AF_INET, SOCK_STREAM, 0);

    /* fill in the server’s IP address and port */
    hp = Gethostbyname(hostname);
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    bcopy((char *)hp->h_addr,
    (char *)&serveraddr.sin_addr.s_addr, hp->h_length);
    serveraddr.sin_port = htons(port);

    /* establish a connection with the server */
    Connect(clientfd, (SA *) &serveraddr, sizeof(serveraddr));

    return clientfd;
 }

 /*
 * open_listenfd - open and return a listening socket on port
 */
 int open_listenfd(int port)
 {
    int listenfd;
    int optval;
    struct sockaddr_in serveraddr;

    /* create a socket descriptor */
    listenfd = Socket(AF_INET, SOCK_STREAM, 0);

    /* eliminates "Address already in use" error from bind. */
    optval = 1;
    Setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR,
    (const void *)&optval , sizeof(int));

    /* listenfd will be an endpoint for all requests to port
    on any IP address for this host */
    bzero((char *) &serveraddr, sizeof(serveraddr));
    serveraddr.sin_family = AF_INET;
    serveraddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serveraddr.sin_port = htons((unsigned short)port);
    Bind(listenfd, (SA *)&serveraddr, sizeof(serveraddr));

    /* make it a listening socket ready to accept connection requests */
    Listen(listenfd, 1);

    return listenfd;
 }









#endif /*  HELPERS_H */
