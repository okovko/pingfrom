#ifndef __SHARED_H_
#define __SHARED_H_

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>
#include <signal.h>

/*
  conn_bind_t to make get_socket generic for client/server
 */
typedef int (*conn_bind_t)(int, const struct sockaddr *, socklen_t);

struct msg {
  int sz;
  char *buf;
} __attribute__((packed));

int get_socket(const char *host, const char *port, conn_bind_t fp);
msg create_msg(char *buf, int sz);
msg receive_msg(int conn_fd);
void send_msg(int conn_fd, msg m);

struct addrinfo *get_server_info(const char *host, const char *port);

#endif // __SHARED_H_
