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
#include <arpa/inet.h>
#include "sqlite3/sqlite3.h"

#define BUFSIZE 2048
#define INFO 1
#define TRUE   1
#define FALSE  0


#define SERVER_DB_NAME "map.db"

static int maxlen = 2048;

static int db_enabled = 1;
static int cw, cx, cy, cz;
static sqlite3 *db;
static sqlite3_stmt *insert_block_stmt;
static sqlite3_stmt *select_block_stmt;
static sqlite3_stmt *update_chunk_stmt;

double rand_double() {
    return (double)rand() / (double)RAND_MAX;
}

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

void db_server_update_chunk(int p, int q) {
    sqlite3_reset(update_chunk_stmt);
    sqlite3_bind_int(update_chunk_stmt, 1, p);
    sqlite3_bind_int(update_chunk_stmt, 2, q);
    while (sqlite3_step(update_chunk_stmt) == SQLITE_ROW) {
		cx = sqlite3_column_int(update_chunk_stmt, 0);
		cy = sqlite3_column_int(update_chunk_stmt, 1);
		cz = sqlite3_column_int(update_chunk_stmt, 2);
        cw = sqlite3_column_int(update_chunk_stmt, 3);
        //map_set(map, x, y, z, w);
    }
}
 
int main(int argc , char *argv[])
{
	int opt = TRUE;
	int master_socket , addrlen , new_socket , client_socket[10] , max_clients = 10 , activity, i , valread , sd;
	int max_sd;
	int port;
	char *message = "Server\n";
	struct sockaddr_in address;
    float x = (rand_double() - 0.5) * 10000;
    float z = (rand_double() - 0.5) * 10000;
    float y = 0;
	float rx = 0;
	float ry = 0;
	char buffer[1025]; 
	
	fd_set readfds;
   
	if(argc == 2) {
		port = atoi(argv[1]);
	} else {
		port = 22500;
	}
   
	for (i = 0; i < max_clients; i++) {
		client_socket[i] = 0;
	}
      
	if((master_socket = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
		perror("socket failed");
		exit(EXIT_FAILURE);
	}

	if(setsockopt(master_socket, SOL_SOCKET, SO_REUSEADDR, (char *)&opt, sizeof(opt)) < 0) {
		perror("setsockopt");
		exit(EXIT_FAILURE);
	}

	address.sin_family = AF_INET;
	address.sin_addr.s_addr = INADDR_ANY;
	address.sin_port = htons(port);

	if (bind(master_socket, (struct sockaddr *)&address, sizeof(address))<0) {
		perror("bind failed");
		exit(EXIT_FAILURE);
	}
	printf("Listener on port %d\n", port);
     
	if (listen(master_socket, 3) < 0) {
		perror("listen");
		exit(EXIT_FAILURE);
	}

	printf("Initializing map database...\n");
	if (db_init()) {
        return -1;
    }
    
	addrlen = sizeof(address);
	printf("Waiting for connections...\n");
     
	while(1)
	{
		FD_ZERO(&readfds);
		FD_SET(master_socket, &readfds);
		max_sd = master_socket;

		for ( i = 0 ; i < max_clients ; i++)
		{
			sd = client_socket[i];
			if(sd > 0) { FD_SET(sd , &readfds); }
			if(sd > max_sd) { max_sd = sd; }
		}

		activity = select( max_sd + 1 , &readfds , NULL , NULL , NULL);
	
		if ((activity < 0) && (errno!=EINTR)) {
			printf("select error");
		}
          
		if (FD_ISSET(master_socket, &readfds)) {
			if ((new_socket = accept(master_socket, (struct sockaddr *)&address, (socklen_t*)&addrlen))<0) {
				perror("accept");
				exit(EXIT_FAILURE);
			}
         
			printf("New connection, socket fd is %d, ip is: %s, port: %d\n" , new_socket , inet_ntoa(address.sin_addr) , ntohs(address.sin_port));
			if(send(new_socket, message, strlen(message), 0) != strlen(message)) {
				perror("send");
			}

			for (i = 0; i < max_clients; i++) {
				if( client_socket[i] == 0 ) {
					client_socket[i] = new_socket;
					printf("Adding new connection to list of sockets as %d\n", i);  
					write(client_socket[i],"U,0,0,10,0",10); // send client position
					break;
				}
			}
		}
          
		for (i = 0; i < max_clients; i++) {
			sd = client_socket[i];
			if (FD_ISSET(sd , &readfds)) {
				if ((valread = recv(sd , buffer, 1024, 0)) > 0) {
					if(buffer[0] == 'B') { 
						// Check if requesting block update
						int p, q, x, y, z, w;
						snprintf(buffer, 1024, "B,%d,%d,%d,%d,%d,%d\n", p, q, x, y, z, w);
						printf("Client -> Server: Requested block change to %d at %d, %d, %d\n", w, x, y, z);
						db_insert_block(p, q, x, y, z, w);
						for (i = 0; i < max_sd; i++) {
							if (client_socket[i] != 0) {
								write(client_socket[i],buffer,strlen(buffer));
							}
						}
					}
					if(buffer[0] == 'C') { 
						// Check if requesting block update
						int p, q, x, y, z, w;
						printf("Client -> Server: Requested chunk update\n");
						snprintf(buffer, 1024, "C,%d,%d,%d,%d,%d,%d\n", p, q, x, y, z, w);
						db_server_update_chunk(p, q);
						for (i = 0; i < max_sd; i++) {
							if (client_socket[i] != 0) {
								write(client_socket[i],buffer,strlen(buffer));
							}
						}
					}
					bzero(buffer,sizeof(buffer));
				} else if(valread == 0) {
					getpeername(sd , (struct sockaddr*)&address, 
							(socklen_t*)&addrlen);
					printf("%s:%d disconnected\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
					close(sd);
					client_socket[i] = 0;
				} else {
					printf("%s:%d disconnected: socket error\n", inet_ntoa(address.sin_addr), ntohs(address.sin_port));
				}
			}
		}
	}
	printf("Saving map state...\n");
    db_save_state(x, y, z, rx, ry);
    printf("Closing map database...\n");
    db_close(); 
	return 0;
} 

