#include <iostream>
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

#include <string.h>

#include "shared.h"

static int conn_fd = -1;
static char buf[512];

// global because signal handler has to flush it
// and buffered because we can't know how much ping
// will send us
void global_buffered_receive() {
  // receive over socket from server's stdout and print
  memset(buf, 0, sizeof buf);
  int filled = 0;
  while (filled < sizeof buf) {
    int received = recv(conn_fd, buf + filled, sizeof buf - filled, 0);
    if (received < 0) {
      perror("recv");
      exit(1);
    }
    if (received == 0) {
      break;
    }
    filled += received;
    if (filled == sizeof buf) {
      fwrite(buf, 1, sizeof buf, stdout);
      memset(buf, 0, sizeof buf);
      filled = 0;
    }
  }
}

// 
void sig_handler(int signum) {

  // single shot best effort; this handler is called once
  signal(SIGINT, SIG_DFL);

  // don't have to check if conn_fd is valid because this handler
  // is only registered after conn_fd is known to be valid

  // just send anything to let the server parent process know to SIGINT its child process
  char junk_byte[] = {1};
  send(conn_fd, junk_byte, 1, 0);

  // the issue is that signal handlers are supposed to be
  // re-entrant, but buf is a global variable
  // TODO: figure out how to pass an argument to a signal handler
  // to avoid this problem (pass it a local buffer)
  // TODO: figure out if fwrite is async-signal-safe (use write if not)
  fwrite(buf, 1, sizeof buf, stdout);

  // localbuf to allow the signal handler to be reentrant
  // (once the other problems are fixed)
  char local_buf[512];
  memset(local_buf, 0, sizeof local_buf);
  recv(conn_fd, local_buf, sizeof local_buf, 0);
  fwrite(local_buf, 1, sizeof local_buf, stdout);
}

int main(int argc, char **argv) {
  // ===================================================================== //
  // the following segment of code just gets name, host, port from argv[1] //
  // and domain from argv[2]                                               //
  // ======================================================================//

  if (argc != 3) {
    perror("argc");
    exit(1);
  }
  int len = strlen(argv[1]);
  char *start = argv[1];
  char *at = strchr(argv[1], '@');
  char *colon = strchr(argv[1], ':');
  if (!at || !colon) {
    perror("at, colon");
    exit(1);
  }

  char *name = (char *)malloc(at - start + 1);
  if (!name) {
    perror("malloc");
    exit(1);
  }
  memcpy(name, start, at - start);
  name[at - start] = '\0';
  char *host = (char *)malloc(colon - at);
  if (!host) {
    perror("malloc");
    exit(1);
  }
  memcpy(host, at + 1, colon - at - 1);
  host[colon - at - 1] = '\0';
  char *port = (char *)malloc(len - (colon - start));
  if (!port) {
    perror("malloc");
    exit(1);
  }
  memcpy(port, colon + 1, len - (colon - start) - 1);
  port[len - (colon - start) - 1] = '\0';

  char *domain = argv[2];

  // ===================================================================== //

  // make a connection, and send a message
  conn_fd = get_socket(host, port, connect);
  msg m_send = create_msg(domain, strlen(domain));
  send_msg(conn_fd, m_send);

  // we want to let the user press Ctrl-C to SIGINT the server's ping process
  signal(SIGINT, sig_handler);

  // print the results of ping
  global_buffered_receive();

  // deallocate strings copied from cmd args
  free(name);
  free(host);
  free(port);

  return 0;
}
