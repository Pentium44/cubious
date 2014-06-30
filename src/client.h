/*
	Old client code, might revive in the future
*/
/*

int client_connect(char *address, int port);
int client_close_connection();
int client_send(char *action, int x, int y, int z, int p, int q, int w);
int client_connected();

*/

void client_enable();
void client_disable();
int get_client_enabled();
void client_connect(char *hostname, int port);
void client_send(char *data);
int client_recv(char *data);
void client_start();
void client_stop();
