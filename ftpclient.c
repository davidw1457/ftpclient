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
 * *------------------------------------------------------------------------
 * */

// Size of default recieve/transmit buffer
const int BUFFERSIZE = 1024;

// function prototypes
void uploadFile(unsigned int, char*);
void deleteFile(unsigned int, char*);
void renameFile(unsigned int, char*);
void downloadFile(unsigned int, char*);
void menu(unsigned int);

int main(int argc, char *argv[]) {
    // BEGIN CODE FROM client.c //
    struct hostent *ptrh; /* pointer to a host table entry */
    struct protoent *ptrp; /* pointer to a protocol table entry */
    struct sockaddr_in sad; /* structure to hold an IP address */
    int sd; /* socket descriptor */
    int port; /* protocol port number */
    char *host; /* pointer to host name */
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
    // END CODE FROM client.c //

    // Open FTP Client menu=
    menu(sd);

    /* Close the socket. */
    closesocket(sd);
    /* Terminate the client program gracefully. */
    exit(0);
}

/*******************************************************************************
 * menu
 * Display menu and process client input
 * @param socket : open socket to FTP server
 ******************************************************************************/
void menu(unsigned int socket) {
    int choice = 0;     // store client choice
    char *allParams;    // store concatenated parameters
    char *input;        // store last input operation
    // initialize strings
    allParams = (char*)malloc(BUFFERSIZE);
    input = (char*)malloc(256);

    // Display menu until user chooses to exit
    while (choice != 5) {
        // display menu
        printf("Enter a choice:\n1 - UPLOAD\n2 - DOWNLOAD\n3 - DELETE\n4 - RENAME\n5 - EXIT\n");

        // get user choice
        fgets(input, 3, stdin);
        choice = atoi(input);

        // process choice
        switch(choice) {
            case 1: // UPLOAD
                // initialize allParams with task
                strcpy(allParams, "UPLOAD ");

                // request filename
                printf("Enter filename to upload: ");
                fgets(input, 256, stdin);

                // remove linebreak from filename
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';

                // add filename to allParams
                strcat(allParams, input);

                // upload file
                uploadFile(socket, allParams);
                break;
            case 2: // DOWNLOAD
                // initialize allParams with task
                strcpy(allParams, "DOWNLOAD ");

                // request filename
                printf("Enter filename to download: ");
                fgets(input, 256, stdin);

                // remove linebreak from filename
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';

                // add filename to allParams
                strcat(allParams, input);

                // download file
                downloadFile(socket, allParams);
                break;
            case 3: // DELETE
                // initialize allParams with task
                strcpy(allParams, "DELETE ");

                // request filename
                printf("Enter filename to delete: ");
                fgets(input, 256, stdin);

                // remove linebreak from filename
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';

                // add filename to allParams
                strcat(allParams, input);

                // delete file
                deleteFile(socket, allParams);
                break;
            case 4: // RENAME
                // initialize allParams with task
                strcpy(allParams, "RENAME ");

                // request original filename
                printf("Enter filename to rename: ");
                fgets(input, 256, stdin);

                // remove linebreak from filename
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';

                // add original filename to allParams
                strcat(allParams, input);
                strcat(allParams, " ");

                // request new filename
                printf("Enter new filename: ");
                fgets(input, 256, stdin);

                // remove linebreak from filename
                if (strlen(input) > 0 && input[strlen(input) - 1] == '\n')
                    input[strlen(input) - 1] = '\0';

                // add new filename to allParams
                strcat(allParams, input);

                // rename file
                renameFile(socket, allParams);
                break;
        }
    }
    // free memory
    free(allParams);
    free(input);
}

/*******************************************************************************
 * uploadFile
 * Upload specified file to FTP server
 * @param socket open socket to FTP server
 * @param params UPLOAD [filename]
 ******************************************************************************/
void uploadFile(unsigned int socket, char* params) {
    char *buffer;   // Buffer for transferring or receiving data over socket
    char *filename; // Filename to upload

    // Send upload command to FTP server
    write(socket, params, strlen(params)+1);

    // parse filename from params
    filename = strtok(params, " ");
    filename = strtok(NULL, " ");

    // Open file to upload
    FILE *file = fopen(filename, "r");

    // If file opens, upload it
    if (file != NULL) {
        struct stat fileStats;  // File stats

        // Load file stats
        stat(filename, &fileStats);

        // copy filesize from fileStats
        int size = fileStats.st_size;

        // send filesize to FTP server
        write(socket, &size, sizeof(int));

        // wait for response from FTP server
        buffer = (char*)malloc(BUFFERSIZE);
        read(socket, buffer, BUFFERSIZE);

        // load file to buffer
        free(buffer);
        buffer = (char*)malloc(size);
        fread(buffer, size, 1, file);

        // send file to FTP server
        write(socket, buffer, size);

        // Read results from server
        free(buffer);
        buffer = (char*)malloc(BUFFERSIZE);
        read(socket, buffer, BUFFERSIZE);

        // close file
        fclose(file);
    } else { // File failed to open
        // Send size = -1, signifying error
        int size = -1;
        write(socket, &size, sizeof(int));

        // read results from FTP server
        buffer = (char*)malloc(BUFFERSIZE);
        read(socket, buffer, BUFFERSIZE);
    }
    // Output results
    printf("%s\n", buffer);

    // free memory
    free(buffer);
}

/*******************************************************************************
 * deleteFile
 * Delete specified file from FTP server
 * @param socket open socket to FTP server
 * @param params DELETE [filename]
 ******************************************************************************/
void deleteFile(unsigned int socket, char* params) {
    char *buffer;   // Buffer for transferring or receiving data over socket
    buffer = (char*)malloc(BUFFERSIZE);

    // send command to FTP server
    write(socket, params, strlen(params)+1);

    // read result from FTP server
    read(socket, buffer, BUFFERSIZE);

    // output results
    printf("%s\n", buffer);

    // free memory
    free(buffer);
}

/*******************************************************************************
 * renameFile
 * Rename file with new filename on FTP server
 * @param socket open socket to FTP server
 * @param params RENAME [original filename] [new filename]
 ******************************************************************************/
void renameFile(unsigned int socket, char* params){
    char *buffer;   // Buffer for transferring or receiving data over socket
    buffer = (char*)malloc(BUFFERSIZE);

    // send rename command to FTP server
    write(socket, params, strlen(params)+1);

    // Read results from FTP server
    read(socket, buffer, BUFFERSIZE);

    // Output results
    printf("%s\n", buffer);
    free(buffer);
}

/*******************************************************************************
 * downloadFile
 * Download file from FTP server
 * @param socket open socket to FTP server
 * @param params DOWNLOAD [filename]
 ******************************************************************************/
void downloadFile(unsigned int socket, char* params) {
    char *buffer;   // Buffer for transferring or receiving data over socket
    char *filename; // Filename to download
    int size;       // Size of file
    buffer = (char*)malloc(BUFFERSIZE);

    // send command to FTP server
    write(socket, params, strlen(params)+1);

    // read filesize from FTP server
    read(socket, buffer, sizeof(int));
    size = (int)buffer[0];

    // acknowledge receipt of size
    strcpy(buffer,"ACK");
    write(socket, buffer, strlen(buffer)+1);

    // If size = -1, server could not open file
    if (size == -1) {
        // read results from server
        read(socket, buffer, BUFFERSIZE);
    } else {
        // parse filename from params
        filename = strtok(params, " ");
        filename = strtok(NULL, " ");

        // create file
        FILE *file = fopen(filename, "w");

        if (file == NULL) { // unable to create file
            printf("unable to create %s\n", filename);
            // clear incoming data
            free(buffer);
            buffer = (char*)malloc(size);
            read(socket, buffer, size);
        } else {    // file created successfully
            // read file contents from FTP server
            free(buffer);
            buffer = (char*)malloc(size);
            read(socket, buffer, size);

            // write contents to file
            fwrite(buffer, size, 1, file);

            // close file
            fclose(file);
        }

        // Acknowledge receipt of file
        free(buffer);
        buffer = (char *) malloc(BUFFERSIZE);
        strcpy(buffer, "ACK");
        write(socket, buffer, strlen(buffer) + 1);

        // read results from FTP server
        read(socket, buffer, BUFFERSIZE);
    }
    // Output result
    printf("%s\n", buffer);
    free(buffer);
}

