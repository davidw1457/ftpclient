

# COSC 4333 Group 6 Project - FTP Server and Client 
The objective of system is a multithreaded FTP server that enables client-server communication through message-oriented structure.

## Group 6 Members
David Williams
Kristy Stallings
Sebastian Salois


## Installation

Ubuntu 21.10 was used for running and testing FTP server and client. 

gcc is needed to run the following commands to compile the C files. 

```bash
sudo apt-get install gcc
```

## Compile

```
# server 
gcc ftpserver.c -o ftpserver -pthread

# client
gcc ftpclient.c -o ftpclient

```

## Run

```
# server - defaults to port 8888 if you don't list port
./ftpserver [port]

# client - defaults to localhost and port 8888 if you don't list host and client
./ftpclient [host] [port]

```

## Instructions

User can select 1-5 on the client server to complete file operation to FTP server. 

```
1 - UPLOAD 
2 - DOWNLOAD
3 - DELETE
4 - RENAME
5 - EXIT
```
