# Program: FTP server

* How to compile: 
`gcc ftpserver.c -o ftpserver -pthread`
`gcc ftpclient.c -o ftpclient`

* How to run: 
`./ftpserver [port]` 
`./ftpclient [host] [port]`

If you leave off host and port, the server defaults to port 8888 and the client connects to port 8888 on localhost.
