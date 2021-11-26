/* server.c - code for example server program that uses TCP */
#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#else
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stddef.h>
#include <sys/stat.h>
#define PROTOPORT 8888 /* default protocol port number */
#define QLEN 6 /* size of request queue */
int visits = 0; /* counts client connections */
/*------------------------------------------------------------------------
 * * Program: server
 * *
 * * Purpose: allocate a socket and then repeatedly execute the following:
 * * (1) wait for the next connection from a client
 * * (2) send a short message to the client
 * * (3) close the connection
 * * (4) go back to step (1)
 * * Syntax: server [ port ]
 * *
 * * port - protocol port number to use
 * *
 * * Note: The port argument is optional. If no port is specified,
 * * the server uses the default given by PROTOPORT.
 * *
 * *------------------------------------------------------------------------
 * */
const int BUFFERSIZE = 1024;

void *worker(void*);
void process(unsigned int);
void uploadFile(unsigned int, char*);
void deleteFile(unsigned int, char*);
void renameFile(unsigned int, char*, char*);
void downloadFile(unsigned int, char*);


int main(int argc, char *argv[]) {
    struct hostent *ptrh; /* pointer to a host table entry */
    struct protoent *ptrp; /* pointer to a protocol table entry */
    struct sockaddr_in sad; /* structure to hold server.s address */
    struct sockaddr_in cad; /* structure to hold client.s address */
    //DW
    unsigned int sd, sd2; /* socket descriptors */
    int port; /* protocol port number */
    int alen; /* length of address */
    char buf[1000]; /* buffer for string the server sends */
    #ifdef WIN32
        WSADATA wsaData;
        WSAStartup(0x0101, &wsaData);
    #endif
    memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
    sad.sin_family = AF_INET; /* set family to Internet */
    sad.sin_addr.s_addr = INADDR_ANY; /* set the local IP address */
    /* Check command-line argument for protocol port and extract */
    /* port number if one is specified. Otherwise, use the default */
   /* port value given by constant PROTOPORT */
    if (argc > 1) { /* if argument specified */
        port = atoi(argv[1]); /* convert argument to binary */
    } else {
        port = PROTOPORT; /* use default port number */
    }
    if (port > 0) /* test for illegal value */
        sad.sin_port = htons((u_short)port);
    else { /* print error message and exit */
        fprintf(stderr,"bad port number %s\n",argv[1]);
        exit(1);
    }
    /* Map TCP transport protocol name to protocol number */
    if ( ((int)(ptrp = getprotobyname("tcp"))) == 0) {
        fprintf(stderr, "cannot map \"tcp\" to protocol number");
        exit(1);
    }
    /* Create a socket */
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd < 0) {
        fprintf(stderr, "socket creation failed\n");
        exit(1);
    }
    /* Bind a local address to the socket */
    if (bind(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        fprintf(stderr,"bind failed\n");
        exit(1);
    }
    /* Specify size of request queue */
    if (listen(sd, QLEN) < 0) {
        fprintf(stderr,"listen failed\n");
        exit(1);
    }
    /* Main server loop - accept and handle requests */
    // DW
    printf("FTP server is on\n");
    pthread_t threads[200];
    unsigned int i = 0;
    int *socket;
    while (1) {
        alen = sizeof(cad);
        if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
            fprintf(stderr, "accept failed\n");
            exit(1);
        }
        // DW
        socket = malloc(1);
        *socket = sd2;
//        pthread_create(&threads[i++], NULL, worker, (void*)socket);
//        pthread_join(threads[i-1], NULL);
        worker((void*)socket);
        //visits++;
        //sprintf(buf,"This server has been contacted %d time%s\n",
        //		visits,visits==1?".":"s.");
        //send(sd2,buf,strlen(buf),0);
        //closesocket(sd2);
    }
}

void *worker(void *arg) // worker thread
{
    unsigned int socket;
    socket = *(unsigned int*)arg;
    process(socket);
    pthread_exit(0);
}

void process(unsigned int socket) {
    char *buffer;
    buffer = (char*)malloc(BUFFERSIZE);
    while (read(socket, buffer, BUFFERSIZE) > 0) {
        printf("Waiting for command\n");
        char* args = strtok(buffer, " ");
        printf("Command received: %s\n", buffer);
        if (!strncmp(args, "UPLOAD", 6)) {
            args = strtok(NULL, " ");
            uploadFile(socket, args);
        } else if (!strncmp(args, "DELETE", 6)) {
            args = strtok(NULL, " ");
            deleteFile(socket, args);
        } else if (!strncmp(args, "RENAME", 6)) {
            char* filename;
            filename = (char*)malloc(256);
            args = strtok(NULL, " ");
            strcpy(filename, args);
            args = strtok(NULL, " ");
            renameFile(socket, filename, args);
            free(filename);
        } else if (!strncmp(args, "DOWNLOAD", 8)) {
            args = strtok(NULL, " ");
            downloadFile(socket, args);
        }
    }
    free(buffer);
    close(socket);
}

void uploadFile(unsigned int socket, char* filename) {
    char *buffer;
    int size;
    buffer = (char*)malloc(BUFFERSIZE);
    read(socket, buffer, sizeof(int));
    size = (int)buffer[0];
    strcpy(buffer, "ACK");
    write(socket, buffer, strlen(buffer)+1);

    FILE *file = fopen(filename, "w");
    if (file == NULL) {
        strcpy(buffer, "FAILURE");
    } else {
        buffer = (char*)malloc(size);
        read(socket, buffer, size);
        fwrite(buffer, size, 1, file);
        free(buffer);
        buffer = (char*)malloc(BUFFERSIZE);
        strcpy(buffer, "SUCCESS");
        fclose(file);
    }
    write(socket, buffer, strlen(buffer)+1);
    free(buffer);
}

void deleteFile(unsigned int socket, char* filename) {
    printf("Deleting %s\n", filename);
    char *buffer;
    buffer = (char*)malloc(BUFFERSIZE);
    if (remove(filename) == 0) {
        strcpy(buffer, "SUCCESS");
    } else {
        strcpy(buffer, "FAILURE");
    }
    write(socket, buffer, strlen(buffer)+1);
    free(buffer);
}

void renameFile(unsigned int socket, char* origFilename, char* newFilename) {
    char *buffer;
    buffer = (char*)malloc(BUFFERSIZE);
    FILE *origFile = fopen(origFilename, "r");
    if (origFile == NULL) {
        strcpy(buffer, "FAILURE");
    } else {
        fclose(origFile);
        if (rename(origFilename, newFilename) == 0) {
            strcpy(buffer, "SUCCESS");
        } else {
            strcpy(buffer, "FAILURE");
        }
    }
    write(socket, buffer, strlen(buffer)+1);
    free(buffer);
}

void downloadFile(unsigned int socket, char* filename) {
    char *buffer;
    FILE *file = fopen(filename, "r");
    if (file == NULL) {
        buffer = (char*)malloc(BUFFERSIZE);
        strcpy(buffer, "FAILURE");
    } else {
        struct stat fileStats;
        stat(filename, &fileStats);
        int size = fileStats.st_size;
        write(socket, &size, sizeof(int));
        read(socket, NULL, BUFFERSIZE);
        buffer = (char*)malloc(size);
        fread(buffer, size, 1, file);
        write(socket, buffer, size);
        free(buffer);
        read(socket, NULL, BUFFERSIZE);
        buffer = (char*)malloc(BUFFERSIZE);
        strcpy(buffer, "SUCCESS");
        fclose(file);
    }
    write(socket, buffer, strlen(buffer)+1);
    free(buffer);
}
