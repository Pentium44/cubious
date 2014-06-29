
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <signal.h>
#include <fcntl.h>
#include <sys/types.h> 
#include <sys/socket.h>
#include <netinet/in.h>

#define BUFSIZE 1024
#define INFO 1

static int maxlen = 2048;

void write_file(int type, char *string) {
	
	int fd ;
	char logbuffer[BUFSIZE*2];
	
	if((fd = open("get.txt", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		switch(type) {
			case INFO:
				(void)sprintf(logbuffer,"[From client]: %s", string);
				(void)write(fd,logbuffer,strlen(logbuffer)); 
				(void)write(fd,"\n",1);      
				break;
		}
		(void)close(fd);
	}
}

void error(const char *msg)
{
    perror(msg);
    exit(1);
}

void listen_to_connection(int fd) {
	char buffer[BUFSIZE*4];
	int read_size;
	while((read_size = recv(fd, buffer, maxlen, 0)) > 0 ) {
		write(fd, buffer, read_size);
	}
	
	if(read_size == 0)
	{
		//fflush(stdout);
		write_file(INFO, "Client closed socket.");
		exit(0);
	}
	else if(read_size == -1)
	{
		write_file(INFO, "Failed to receive data from socket.");
		exit(1);
	}
	
	exit(0);
}

int main(int argc, char **argv)
{
	int i, port, pid, listenfd, socketfd, hit;
	socklen_t length;
	static struct sockaddr_in cli_addr; 
	static struct sockaddr_in serv_addr;

	if( argc < 2  || argc > 2 || !strcmp(argv[1], "-?") ) {
		printf("No port specified\n");
		exit(1);
	}

	printf("Starting server...\n");

	if(fork() != 0)
		return 1; 
	(void)signal(SIGCLD, SIG_IGN); 
	(void)signal(SIGHUP, SIG_IGN); 
	for(i=0;i<32;i++)
		(void)close(i);	
	(void)setpgrp();	

	if((listenfd = socket(AF_INET, SOCK_STREAM,0)) <0) {
		write_file(INFO, "Failed to open socket");
		return -1;
	}
		
	port = atoi(argv[1]);
	if(port < 0 || port >60000) {
		write_file(INFO, "Failed to start on port");
		return -1;
	}
		
	//printf("Setting up socket...\n");
	serv_addr.sin_family = AF_INET;
	serv_addr.sin_addr.s_addr = htonl(INADDR_ANY);
	serv_addr.sin_port = htons(port);
	if(bind(listenfd, (struct sockaddr *)&serv_addr,sizeof(serv_addr)) <0) {
		write_file(INFO, "Failed to bind to port");
		return -1;
	}
	
	if( listen(listenfd,64) <0) {
		write_file(INFO, "Failed to listen to socket");
		return -1;
	}

	for(hit=1; ;hit++) {
		length = sizeof(cli_addr);
		if((socketfd = accept(listenfd, (struct sockaddr *)&cli_addr, &length)) < 0) {
			write_file(INFO, "Failed to accept connection...");
			return -1;
		}

		if((pid = fork()) < 0) {
			write_file(INFO, "Failed to fork()");
			return -1;
		}
		else {
			if(pid == 0) {
				(void)close(listenfd);
				listen_to_connection(socketfd);
			} else {
				(void)close(socketfd);
			}
		}
	}
	return 0;
}

