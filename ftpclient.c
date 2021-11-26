/* client.c - code for example client program that uses TCP */
#ifndef unix
#define WIN32
#include <windows.h>
#include <winsock.h>
#else
#define closesocket close
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>
#endif
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#define PROTOPORT 8888 /* default protocol port number */
extern int errno;
char localhost[] = "localhost"; /* default host name */
/*------------------------------------------------------------------------
 * * Program: client
 * *
 * * Purpose: allocate a socket, connect to a server, and print all output
 * * Syntax: client [ host [port] ]
 * *
 * * host - name of a computer on which server is executing
 * * port - protocol port number server is using
 * *
 * * Note: Both arguments are optional. If no host name is specified,
 * * the client uses "localhost"; if no protocol port is
 * * specified, the client uses the default given by PROTOPORT.
 * *
 * *------------------------------------------------------------------------
 * */
const int BUFFERSIZE = 1024;

void uploadFile(unsigned int, char*);
void deleteFile(unsigned int, char*);
void renameFile(unsigned int, char*);
void downloadFile(unsigned int, char*);
void menu(unsigned int);

int main(int argc, char *argv[]) {
    struct hostent *ptrh; /* pointer to a host table entry */
    struct protoent *ptrp; /* pointer to a protocol table entry */
    struct sockaddr_in sad; /* structure to hold an IP address */
    int sd; /* socket descriptor */
    int port; /* protocol port number */
    char *host; /* pointer to host name */
    int n; /* number of characters read */
    char buf[1000]; /* buffer for data from the server */
    char *task; //DW
    #ifdef WIN32
        WSADATA wsaData;
        WSAStartup(0x0101, &wsaData);
    #endif
    memset((char *)&sad,0,sizeof(sad)); /* clear sockaddr structure */
    sad.sin_family = AF_INET; /* set family to Internet */
    /* Check command-line argument for protocol port and extract */
    /* port number if one is specified. Otherwise, use the default */
    /* port value given by constant PROTOPORT */
    if (argc > 2) { /* if protocol port specified */
        port = atoi(argv[2]); /* convert to binary */
    } else {
        port = PROTOPORT; /* use default port number */
    }
    if (port > 0) /* test for legal value */
        sad.sin_port = htons((u_short)port);
    else { /* print error message and exit */
        fprintf(stderr,"bad port number %s\n",argv[2]);
        exit(1);
    }
    /* Check host argument and assign host name. */
    if (argc > 1) {
        host = argv[1]; /* if host argument specified */
    } else {
        host = localhost;
    }
    /* Convert host name to equivalent IP address and copy to sad. */
    ptrh = gethostbyname(host);
    if ( ((char *)ptrh) == NULL ) {
        fprintf(stderr,"invalid host: %s\n", host);
        exit(1);
    }
    memcpy(&sad.sin_addr, ptrh->h_addr, ptrh->h_length);
    /* Map TCP transport protocol name to protocol number. */
    if ( ((int)(ptrp = getprotobyname("tcp"))) == 0) {
        fprintf(stderr, "cannot map \"tcp\" to protocol number");
        exit(1);
    }
    /* Create a socket. */
    sd = socket(PF_INET, SOCK_STREAM, ptrp->p_proto);
    if (sd < 0) {
        fprintf(stderr, "socket creation failed\n");
        exit(1);
    }
    /* Connect the socket to the specified server. */
    if (connect(sd, (struct sockaddr *)&sad, sizeof(sad)) < 0) {
        fprintf(stderr,"connect failed\n");
        exit(1);
    }
    // DW
    menu(sd);
//    /* Repeatedly read data from socket and write to user.s screen. */
//    n = recv(sd, buf, sizeof(buf), 0);
//    while (n > 0) {
//        write(1,buf,n);
//        printf("n: %d\n", n);
//        n = recv(sd, buf, sizeof(buf), 0);
//    }
//    printf("n: %d\n", n);
    /* Close the socket. */
    closesocket(sd);
    /* Terminate the client program gracefully. */
    exit(0);
}

void menu(unsigned int socket) {
    int choice = 0;
    char *allParams;
    char *input;
    while (choice != 5) {
        allParams = (char*)malloc(BUFFERSIZE);
        input = (char*)malloc(256);
        printf("Enter a choice:\n1 - UPLOAD\n2 - DOWNLOAD\n3 - DELETE\n4 - RENAME\n5 - EXIT\n");
        fgets(input, 3, stdin);
        choice = atoi(input);
        switch(choice) {
            case 1:
                strcpy(allParams, "UPLOAD ");
                printf("Enter filename to upload: ");
                fgets(input, 256, stdin);
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';
                strcat(allParams, input);
                uploadFile(socket, allParams);
                break;
            case 2:
                strcpy(allParams, "DOWNLOAD ");
                printf("Enter filename to download: ");
                fgets(input, 256, stdin);
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';
                strcat(allParams, input);
                downloadFile(socket, allParams);
                break;
            case 3:
                strcpy(allParams, "DELETE ");
                printf("Enter filename to delete: ");
                fgets(input, 256, stdin);
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';
                strcat(allParams, input);
                deleteFile(socket, allParams);
                break;
            case 4:
                strcpy(allParams, "RENAME ");
                printf("Enter filename to rename: ");
                fgets(input, 256, stdin);
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';
                strcat(allParams, input);
                strcat(allParams, " ");
                printf("Enter new filename: ");
                fgets(input, 256, stdin);
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';
                strcat(allParams, input);
                renameFile(socket, allParams);
                break;
        }
        free(allParams);
        free(input);
    }
}

void uploadFile(unsigned int socket, char* params) {
    char *buffer;
    char *filename;
    write(socket, params, strlen(params)+1);
    filename = strtok(params, " ");
    filename = strtok(NULL, " ");
    FILE *file = fopen(filename, "r");
    if (file != NULL) {
        struct stat fileStats;
        stat(filename, &fileStats);
        int size = fileStats.st_size;
        write(socket, &size, sizeof(int));
        buffer = (char*)malloc(BUFFERSIZE);
        read(socket, buffer, BUFFERSIZE);
        buffer = (char*)malloc(size);
        fread(buffer, size, 1, file);
        write(socket, buffer, size);
        free(buffer);
        buffer = (char*)malloc(BUFFERSIZE);
        read(socket, buffer, BUFFERSIZE);
        printf("%s\n", buffer);
        free(buffer);
        fclose(file);
    }
}

void deleteFile(unsigned int socket, char* params) {
    char *buffer;
    buffer = (char*)malloc(BUFFERSIZE);
    write(socket, params, strlen(params)+1);
    read(socket, buffer, BUFFERSIZE);
    printf("%s\n", buffer);
    free(buffer);
}

void renameFile(unsigned int socket, char* params){
    char *buffer;
    buffer = (char*)malloc(BUFFERSIZE);
    write(socket, params, strlen(params)+1);
    read(socket, buffer, BUFFERSIZE);
    printf("%s\n", buffer);
    free(buffer);
}

void downloadFile(unsigned int socket, char* params) {
    char *buffer;
    char *filename;
    int size;
    buffer = (char*)malloc(BUFFERSIZE);
    write(socket, params, strlen(params)+1);
    read(socket, buffer, sizeof(int));
    size = (int)buffer[0];
    strcpy(buffer,"ACK");
    write(socket, buffer, strlen(buffer)+1);
    filename = strtok(params, " ");
    filename = strtok(NULL, " ");
    FILE *file = fopen(filename, "w");
    free(buffer);
    buffer = (char*)malloc(size);
    read(socket, buffer, size);
    fwrite(buffer, size, 1, file);
    fclose(file);
    free(buffer);
    buffer = (char*)malloc(BUFFERSIZE);
    strcpy(buffer, "ACK");
    write(socket, buffer, strlen(buffer)+1);
    read(socket, buffer, BUFFERSIZE);
    printf("%s\n", buffer);
    free(buffer);
}

