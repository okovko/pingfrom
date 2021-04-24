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

#include "shared.h"

// wrapper around getaddrinfo for client and server
struct addrinfo *get_server_info(const char *host, const char *port) {
  struct addrinfo hints, *ai;
  memset(&hints, 0, sizeof hints);
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (host == NULL) {
    hints.ai_flags = AI_PASSIVE;
  }
  if (getaddrinfo(host, port, &hints, &ai) != 0) {
    perror("getaddrinfo");
    exit(1);
  }
  return ai;
}

// wrapper around socket and bind/connect for client and server
int get_socket(const char *host, const char *port, conn_bind_t fp) {
  struct addrinfo *ai = get_server_info(host, port);
  while (ai != NULL) {
    int fd = socket(ai->ai_family, ai->ai_socktype, ai->ai_protocol);
    if (fd == -1) {
      perror("socket");
      continue;
    }
    if ((*fp)(fd, ai->ai_addr, ai->ai_addrlen) == -1) {
      if (close(fd) == -1) {
        perror("close");
        exit(1);
      }
      perror("bind");
      continue;
    }
    freeaddrinfo(ai);
    return fd;
  }
  freeaddrinfo(ai);
  return -1;
}

// create a message for use with send_msg
msg create_msg(char *buf, int sz) {
  msg m;
  m.sz = sz;
  m.buf = (char *)malloc(sz);
  if (!m.buf) {
    perror("malloc");
  }
  memcpy(m.buf, buf, sz);
  return m;
}

// send message across active connection
void send_msg(int conn_fd, msg m) {
  // gotta remember host size
  int send_sz = m.sz;
  // and convert m.sz to network size 
  m.sz = htonl(m.sz);

  // send the size
  int filled = 0;
  while (filled < sizeof m.sz) {
    int sent = send(conn_fd, (char *)(&m.sz) + filled, sizeof m.sz - filled, 0);
    if (sent == -1) {
      perror("send");
      exit(1);
    }
    if (sent == 0) {
      break;
    }
    filled += sent;
  }

  // send the buffer
  filled = 0;
  while (filled < send_sz) {
    int sent = send(conn_fd, (char *)(m.buf) + filled, send_sz - filled, 0);
    if (sent == -1) {
      perror("send");
      exit(1);
    }
    if (sent == 0) {
      break;
    }
    filled += sent;
  }
}

// receive a message over an active connection
msg receive_msg(int conn_fd) {
  // get the size of the message, it's the first 4 bytes
  int net_sz = 0;
  int filled = 0;
  while (filled < sizeof net_sz) {
    int received = recv(conn_fd, &net_sz + filled, sizeof net_sz - filled, 0);
    if (received == -1) {
      perror("recv");
      exit(1);
    }
    if (received == 0) {
      break;
    }
    filled += received;
  }

  // mindful to convert to host byte order
  int sz = ntohl(net_sz);

  // now can allocate a buffer for the rest of the message
  char *buf = (char *)malloc(sz);
  if (!buf) {
    perror("malloc");
  }
  memset(buf, 0, sz);

  // receive the message
  filled = 0;
  while (filled < sz) {
    int received = recv(conn_fd, buf + filled, sz - filled, 0);
    if (received == -1) {
      perror("recv");
      exit(1);
    }
    if (received == 0) {
      break;
    }
    filled += received;
  }
  
  // get the message back to the caller
  msg m;
  m.sz = filled;
  m.buf = buf;
  return m;
}
