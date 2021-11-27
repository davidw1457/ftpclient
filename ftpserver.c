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

// Size of default recieve/transmit buffer
const int BUFFERSIZE = 1024;

// function prototypes
void *worker(void*);
void process(unsigned int);
void uploadFile(unsigned int, char*);
void deleteFile(unsigned int, char*);
void renameFile(unsigned int, char*, char*);
void downloadFile(unsigned int, char*);


int main(int argc, char *argv[]) {
    // BEGIN CODE FROM server.c //
    struct protoent *ptrp; /* pointer to a protocol table entry */
    struct sockaddr_in sad; /* structure to hold server.s address */
    struct sockaddr_in cad; /* structure to hold client.s address */
    unsigned int sd, sd2; /* socket descriptors */
    int port; /* protocol port number */
    int alen; /* length of address */
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

    // Server has started
    printf("FTP server is on\n");
    pthread_t threads[200]; // pointers to threads
    unsigned int i = 0;     // number of threads opened
    int *socket;            // pointer to socket
    while (1) {
        // attempt to accept a connection
        alen = sizeof(cad);
        if ( (sd2=accept(sd, (struct sockaddr *)&cad, &alen)) < 0) {
            fprintf(stderr, "accept failed\n");
            exit(1);
        }

        // copy opened socket to a new pointer
        socket = malloc(1);
        *socket = sd2;

        // create thread
        pthread_create(&threads[i++], NULL, worker, (void*)socket);

        // close thread
        pthread_join(threads[i-1], NULL);

//        worker((void*)socket); // for testing without threading
    }
}

/*******************************************************************************
 * worker
 * Spin off thread to handle FTP client
 * @param arg open socket to FTP client
 * @return nothing
 ******************************************************************************/
void *worker(void *arg) // worker thread
{
    unsigned int socket;    // pointer to open socket to FTP client

    // copy socket
    socket = *(unsigned int*)arg;

    // process input from socket
    process(socket);

    // end thread
    pthread_exit(0);
}

/*******************************************************************************
 * process
 * Handle input from FTP client
 * @param socket open socket to FTP client
 ******************************************************************************/
void process(unsigned int socket) {
    char *buffer;   // buffer for transferring or receiving data over socket
    buffer = (char*)malloc(BUFFERSIZE);

    // Loop as long as the socket is open
    while (read(socket, buffer, BUFFERSIZE) > 0) {
        printf("Waiting for command\n");

        // parse command from received data
        char* args = strtok(buffer, " ");
        printf("Command received: %s\n", buffer);

        if (!strncmp(args, "UPLOAD", 6)) {
            // parse filename from received data
            args = strtok(NULL, " ");

            // upload file
            uploadFile(socket, args);
        } else if (!strncmp(args, "DELETE", 6)) {
            // parse filename from received date
            args = strtok(NULL, " ");

            // delete file
            deleteFile(socket, args);
        } else if (!strncmp(args, "RENAME", 6)) {
            char* filename;  // original filename
            filename = (char*)malloc(256);

            // parse original filename from received data
            args = strtok(NULL, " ");
            strcpy(filename, args);

            // parse new filename from received data
            args = strtok(NULL, " ");

            // rename file
            renameFile(socket, filename, args);
            free(filename);
        } else if (!strncmp(args, "DOWNLOAD", 8)) {
            // parse filename from received data
            args = strtok(NULL, " ");

            // download file
            downloadFile(socket, args);
        }
    }
    // free memory and close socket
    printf("\n");
    free(buffer);
    close(socket);
}

/*******************************************************************************
 * uploadFile
 * Receive uploaded file from FTP client
 * @param socket open socket to FTP client
 * @param filename filename to receive
 ******************************************************************************/
void uploadFile(unsigned int socket, char* filename) {
    printf("Receiving upload of %s\n", filename);
    char *buffer;   // Buffer for transferring or receiving data over socket
    int size;       // File size

    // read file size from FTP client
    buffer = (char*)malloc(BUFFERSIZE);
    read(socket, buffer, sizeof(int));
    size = (int)buffer[0];

    if (size == -1) { // client failed to open file to upload
        // create response
        printf("Upload %s failed\n", filename);
        strcpy(buffer, "FAILURE");
    } else {
        // acknowledge receipt of size
        strcpy(buffer, "ACK");
        write(socket, buffer, strlen(buffer) + 1);

        // create file
        FILE *file = fopen(filename, "w");
        if (file == NULL) { // file could not be created
            // dump incoming file data
            free(buffer);
            buffer = (char*)malloc(size);
            read(socket, buffer, size);

            // create response
            free(buffer);
            buffer = (char*)malloc(BUFFERSIZE);
            printf("Upload %s failed\n", filename);
            strcpy(buffer, "FAILURE");
        } else {
            // read incoming data
            buffer = (char *) malloc(size);
            read(socket, buffer, size);

            // write data to file
            fwrite(buffer, size, 1, file);
            free(buffer);

            // create response
            buffer = (char *) malloc(BUFFERSIZE);
            printf("Upload %s succeeded\n", filename);
            strcpy(buffer, "SUCCESS");
            fclose(file);
        }
    }
    // send results to FTP client
    write(socket, buffer, strlen(buffer)+1);
    free(buffer);
}

/*******************************************************************************
 * deleteFile
 * Delete file from FTP server specified by FTP client
 * @param socket open socket to FTP client
 * @param filename filename to delete
 ******************************************************************************/
void deleteFile(unsigned int socket, char* filename) {
    printf("Deleting %s\n", filename);
    char *buffer;   // Buffer for transferring or receiving data over socket
    buffer = (char*)malloc(BUFFERSIZE);

    if (remove(filename) == 0) { // file deleted
        // create response
        printf("Delete %s succeeded\n", filename);
        strcpy(buffer, "SUCCESS");
    } else { // file delete failed
        // create response
        printf("Delete %s failed\n", filename);
        strcpy(buffer, "FAILURE");
    }
    // Send response to FTP Client
    write(socket, buffer, strlen(buffer)+1);
    free(buffer);
}

/*******************************************************************************
 * renameFile
 * Rename file on FTP server to new name specified by FTP client
 * @param socket open socket to FTP client
 * @param origFilename Original filename
 * @param newFilename New filename
 ******************************************************************************/
void renameFile(unsigned int socket, char* origFilename, char* newFilename) {
    printf("Renaming %s to %s\n", origFilename, newFilename);
    char *buffer;   // Buffer for transferring or receiving data over socket
    buffer = (char*)malloc(BUFFERSIZE);

    // open original file to verify it exists
    FILE *origFile = fopen(origFilename, "r");
    if (origFile == NULL) { // open failed
        // create response
        printf("Renaming %s to %s failed\n", origFilename, newFilename);
        strcpy(buffer, "FAILURE");
    } else {
        // close file
        fclose(origFile);
        if (rename(origFilename, newFilename) == 0) { // rename succeeded
            // create response
            printf("%s renamed to %s\n", origFilename, newFilename);
            strcpy(buffer, "SUCCESS");
        } else { // rename failed
            // create response
            printf("Renaming %s to %s failed\n", origFilename, newFilename);
            strcpy(buffer, "FAILURE");
        }
    }
    // send response to FTP client
    write(socket, buffer, strlen(buffer)+1);
    free(buffer);
}

/*******************************************************************************
 * downloadFile
 * Send file to FTP client
 * @param socket open socket to FTP client
 * @param filename file to send to FTP client
 ******************************************************************************/
void downloadFile(unsigned int socket, char* filename) {
    printf("Sending %s to FTP client\n", filename);
    char *buffer;   // Buffer for transferring or receiving data over socket

    // Open file to send to FTP client
    FILE *file = fopen(filename, "r");
    if (file == NULL) { // Failed to open file
        // Send size = -1, signifying error
        int size = -1;
        write(socket, &size, sizeof(int));

        // clear incoming data
        buffer = (char*)malloc(BUFFERSIZE);
        read(socket, buffer, BUFFERSIZE);

        // create response
        printf("Failed to send %s to FTP client\n", filename);
        strcpy(buffer, "FAILURE");
    } else {    // Succeeded at opening file
        struct stat fileStats;  // File stats

        // load file stats
        stat(filename, &fileStats);

        // copy filesize from fileStats
        int size = fileStats.st_size;

        // send filesize to FTP client
        write(socket, &size, sizeof(int));

        // wait for acknowledgement from FTP client
        buffer = (char*)malloc(BUFFERSIZE);
        read(socket, buffer, BUFFERSIZE);

        // load file to buffer
        free(buffer);
        buffer = (char*)malloc(size);
        fread(buffer, size, 1, file);

        // send file to FTP client
        write(socket, buffer, size);

        // wait for acknowledgement from FTP client
        free(buffer);
        buffer = (char*)malloc(BUFFERSIZE);
        read(socket, buffer, BUFFERSIZE);

        // create response
        printf("Sent %s to FTP client\n", filename);
        free(buffer);
        buffer = (char*)malloc(BUFFERSIZE);
        strcpy(buffer, "SUCCESS");

        // close file
        fclose(file);
    }
    // send results to FTP client
    write(socket, buffer, strlen(buffer)+1);
    free(buffer);
}
