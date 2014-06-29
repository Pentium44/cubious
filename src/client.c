#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>   
#include <sys/socket.h>    
#include <arpa/inet.h> 
#include <math.h>
#include "client.h"
//#include "db.h"

#define BUFSIZE 1024

static int fd;
static int maxlen = 2048;
static int connection_status = 0;
struct sockaddr_in server;
char buffer[BUFSIZE], recv_buffer[BUFSIZE*2];

int client_connected() {
	return connection_status;
}

int client_connect(char *address, int port) {
	// create socket
	fd = socket(AF_INET, SOCK_STREAM, 0);
	if (fd == -1)
	{
		printf("Client: Could not create socket...\n");
		return -1;
	}
	printf("Client: Created socket...\n");
	
	server.sin_addr.s_addr = inet_addr(address);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
	
	if (connect(fd, (struct sockaddr *)&server, sizeof(server)) < 0)
	{
		printf("Client: Failed to connect to %s:%d\n", address, port);
		return -1;
	}
	printf("Client: Connected...\n");
	connection_status = 1;
	return 0;
} 

int client_close_connection() {
	close(fd);
	connection_status = 0;
	return 0;
}

int client_send(char *action, int x, int y, int z, int p, int q, int w) {
	char string[BUFSIZE];
	
	sprintf(string, "%d,%d,%d,%d,%d,%d", x, y, z, p, q, w);
	
	if(send(fd, string, strlen(string), 0) < 0)
	{
		printf("Send failed\n");
		client_close_connection();
		return -1;
	}
        
	if(recv(fd, recv_buffer, maxlen, 0) < 0)
	{
		printf("recv failed\n");
		client_close_connection();
		return -1;
	}
        
	printf("Server: %s\n",recv_buffer);
	//char separator[2] = ",";
	//char *split = strtok(recv_buffer,separator);
	//int var;
	//int values[6];
	//int i = 0;
	/*
	while(split != NULL) {
		sscanf (split, "%d", &var);
		values[i++] = var;
	}
	
	db_insert_block(values[1], values[2], values[3], values[4], values[5], values[6]);
	*/
	return 0;
}
