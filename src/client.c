#include <stdio.h> 
#include <stdlib.h>
#include <unistd.h>
#include <string.h>  
#include <sys/types.h> 
#include <sys/socket.h>    
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>
#include <pthread.h> 
#include <math.h>
#include "client.h"
//#include "db.h"

#define BUFSIZE 8192

static int fd;
//static int maxlen = 2048;
static int connection_status = 0;
//static char send_buffer[BUFSIZE] = {0};
static char recv_buffer[BUFSIZE] = {0};
static pthread_t recv_thread;
static pthread_mutex_t mutex;

void client_enable() {
	connection_status = 1;
}

void client_disable() {
	connection_status = 0;
}

int get_client_enabled() {
	return connection_status;
}

/*
	Old client code, might revive in the future
*/

/*
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
	return 0;
}
*/

int client_sendall(int fd, char *data, int length) {
    int count = 0;
    while (count < length) {
        int n = send(fd, data + count, length, 0);
        if (n == -1) {
            return -1;
        }
        count += n;
        length -= n;
    }
    return 0;
}

void client_connect(char *hostname, int port) {
    if (!connection_status) {
        return;
    }
    struct hostent *host;
    struct sockaddr_in server;
    if ((host = gethostbyname(hostname)) == 0) {
        perror("gethostbyname");
        exit(1);
    }
    memset(&server, 0, sizeof(server));
	server.sin_addr.s_addr = inet_addr(hostname);
	server.sin_family = AF_INET;
	server.sin_port = htons(port);
    if ((fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket");
        exit(1);
    }
    if (connect(fd, (struct sockaddr *)&server, sizeof(server)) == -1) {
        perror("connect");
        exit(1);
    }
}

void client_send(char *data) {
    if (!connection_status) {
        return;
    }
    if (client_sendall(fd, data, strlen(data)) == -1) {
        perror("client_sendall");
        exit(1);
    }
    // pthread_mutex_lock(&mutex);
    // strcat(send_buffer, data);
    // pthread_mutex_unlock(&mutex);
}

int client_recv(char *data) {
    if (!connection_status) {
        return 0;
    }
    int result = 0;
    pthread_mutex_lock(&mutex);
    char *p = strstr(recv_buffer, "\n");
    if (p) {
        *p = '\0';
        strcpy(data, recv_buffer);
        memmove(recv_buffer, p + 1, strlen(p + 1) + 1);
        result = 1;
    }
    pthread_mutex_unlock(&mutex);
    return result;
}

// void *send_worker(void *arg) {
//     while (1) {
//         pthread_mutex_lock(&mutex);
//         int length = strlen(send_buffer);
//         if (length) {
//             if (client_sendall(fd, send_buffer, length) == -1) {
//                 perror("client_sendall");
//                 exit(1);
//             }
//             send_buffer[0] = '\0';
//         }
//         pthread_mutex_unlock(&mutex);
//     }
//     return NULL;
// }

void *recv_worker(void *arg) {
    while (1) {
        char data[BUFSIZE] = {0};
        if (recv(fd, data, BUFSIZE, 0) == -1) {
            perror("recv");
            exit(1);
        }
        pthread_mutex_lock(&mutex);
        strcat(recv_buffer, data);
        pthread_mutex_unlock(&mutex);
    }
    return NULL;
}

void client_start() {
    if (!connection_status) {
        return;
    }
    pthread_mutex_init(&mutex, NULL);
    // if (pthread_create(&send_thread, NULL, send_worker, NULL)) {
    //     perror("pthread_create");
    //     exit(1);
    // }
    if (pthread_create(&recv_thread, NULL, recv_worker, NULL)) {
        perror("pthread_create");
        exit(1);
    }
}

void client_stop() {
    if (!connection_status) {
        return;
    }
    // if (pthread_join(send_thread, NULL)) {
    //     perror("pthread_join");
    //     exit(1);
    // }
    if (pthread_join(recv_thread, NULL)) {
        perror("pthread_join");
        exit(1);
    }
    pthread_mutex_destroy(&mutex);
}

// int main(int argc, char **argv) {
//     client_connect(HOST, PORT);
//     client_start();
//     client_send("B,0,0,0,0,0,1\n");
//     client_send("C,0,0\n");
//     char data[BUFSIZE];
//     while (1) {
//         if (client_recv(data)) {
//             printf("%s\n", data);
//         }
//         sleep(1);
//     }
//     client_stop();
// }
