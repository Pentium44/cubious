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
#include "db.h"
#include "sqlite3/sqlite3.h"

#define BUFSIZE 2048
#define INFO 1

static int maxlen = 2048;

static int db_enabled = 1;
static sqlite3 *db;
static sqlite3_stmt *insert_block_stmt;
static sqlite3_stmt *select_block_stmt;
static sqlite3_stmt *update_chunk_stmt;

int db_init() {
    static const char *create_query =
        "create table if not exists state ("
        "   x float not null,"
        "   y float not null,"
        "   z float not null,"
        "   rx float not null,"
        "   ry float not null"
        ");"
        "create table if not exists block ("
        "    p int not null,"
        "    q int not null,"
        "    x int not null,"
        "    y int not null,"
        "    z int not null,"
        "    w int not null"
        ");"
        "create index if not exists block_pq_idx on block(p, q);"
        "create index if not exists block_xyz_idx on block (x, y, z);"
        "create unique index if not exists block_pqxyz_idx on block (p, q, x, y, z);";

    static const char *insert_block_query =
        "insert or replace into block (p, q, x, y, z, w) "
        "values (?, ?, ?, ?, ?, ?);";

    static const char *select_block_query =
        "select w from block where x = ? and y = ? and z = ?;";

    static const char *update_chunk_query =
        "select x, y, z, w from block where p = ? and q = ?;";

    int rc;
    rc = sqlite3_open(SERVER_DB_NAME, &db);
    if (rc) return rc;
    rc = sqlite3_exec(db, create_query, NULL, NULL, NULL);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, insert_block_query, -1, &insert_block_stmt, NULL);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, select_block_query, -1, &select_block_stmt, NULL);
    if (rc) return rc;
    rc = sqlite3_prepare_v2(db, update_chunk_query, -1, &update_chunk_stmt, NULL);
    if (rc) return rc;
    return 0;
}

void db_close() {
    sqlite3_finalize(insert_block_stmt);
    sqlite3_finalize(select_block_stmt);
    sqlite3_finalize(update_chunk_stmt);
    sqlite3_close(db);
}

void db_save_state(float x, float y, float z, float rx, float ry) {
    static const char *query =
        "insert into state (x, y, z, rx, ry) values (?, ?, ?, ?, ?);";
    sqlite3_stmt *stmt;
    sqlite3_exec(db, "delete from state;", NULL, NULL, NULL);
    sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    sqlite3_bind_double(stmt, 1, x);
    sqlite3_bind_double(stmt, 2, y);
    sqlite3_bind_double(stmt, 3, z);
    sqlite3_bind_double(stmt, 4, rx);
    sqlite3_bind_double(stmt, 5, ry);
    sqlite3_step(stmt);
    sqlite3_finalize(stmt);
}

int db_load_state(float *x, float *y, float *z, float *rx, float *ry) {
    static const char *query =
        "select x, y, z, rx, ry from state;";
    int result = 0;
    sqlite3_stmt *stmt;
    sqlite3_prepare_v2(db, query, -1, &stmt, NULL);
    if (sqlite3_step(stmt) == SQLITE_ROW) {
        *x = sqlite3_column_double(stmt, 0);
        *y = sqlite3_column_double(stmt, 1);
        *z = sqlite3_column_double(stmt, 2);
        *rx = sqlite3_column_double(stmt, 3);
        *ry = sqlite3_column_double(stmt, 4);
        result = 1;
    }
    sqlite3_finalize(stmt);
    return result;
}

void db_insert_block(int p, int q, int x, int y, int z, int w) {
    sqlite3_reset(insert_block_stmt);
    sqlite3_bind_int(insert_block_stmt, 1, p);
    sqlite3_bind_int(insert_block_stmt, 2, q);
    sqlite3_bind_int(insert_block_stmt, 3, x);
    sqlite3_bind_int(insert_block_stmt, 4, y);
    sqlite3_bind_int(insert_block_stmt, 5, z);
    sqlite3_bind_int(insert_block_stmt, 6, w);
    sqlite3_step(insert_block_stmt);
}

int db_select_block(int x, int y, int z) {
    sqlite3_reset(select_block_stmt);
    sqlite3_bind_int(select_block_stmt, 1, x);
    sqlite3_bind_int(select_block_stmt, 2, y);
    sqlite3_bind_int(select_block_stmt, 3, z);
    if (sqlite3_step(select_block_stmt) == SQLITE_ROW) {
        return sqlite3_column_int(select_block_stmt, 0);
    }
    else {
        return -1;
    }
}

void db_update_chunk(Map *map, int p, int q) {
    sqlite3_reset(update_chunk_stmt);
    sqlite3_bind_int(update_chunk_stmt, 1, p);
    sqlite3_bind_int(update_chunk_stmt, 2, q);
    while (sqlite3_step(update_chunk_stmt) == SQLITE_ROW) {
        int x = sqlite3_column_int(update_chunk_stmt, 0);
        int y = sqlite3_column_int(update_chunk_stmt, 1);
        int z = sqlite3_column_int(update_chunk_stmt, 2);
        int w = sqlite3_column_int(update_chunk_stmt, 3);
        map_set(map, x, y, z, w);
    }
}

void write_file(int type, char *string) {
	
	int fd ;
	char logbuffer[BUFSIZE*2];
	
	if((fd = open("get.txt", O_CREAT| O_WRONLY | O_APPEND,0644)) >= 0) {
		switch(type) {
			case INFO:
				(void)sprintf(logbuffer,"[Write]: %s", string);
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
		if(buffer[0] == 'B') { // Check if requesting block update
			int p, q, x, y, z, w;
			snprintf(buffer, 1024, "B,%d,%d,%d,%d,%d,%d\n", p, q, x, y, z, w);
			db_insert_block(p, q, x, y, z, w);
			// This is not a complete funtion, doesn't
			// notify other connections of change
		}
		if(buffer[0] == 'C') { // Check if requesting chunk update
			// Nothing here at the moment
		}
		bzero(buffer,read_size+1);
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

