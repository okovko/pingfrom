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

/*
  wrapper around listen for server
 */
void listen_socket(int fd, int backlog) {
  if (fd != -1) {
    if (listen(fd, backlog) == -1) {
      perror("listen");
      exit(1);
    }
  }
}

/*
  wrapper around accept for server
 */
int accept_conn(int server_fd) {
  struct sockaddr_storage conn_sa;
  memset(&conn_sa, 0, sizeof conn_sa);
  socklen_t conn_size = sizeof conn_sa;
  int conn_fd = accept(server_fd, (struct sockaddr *)&conn_sa, &conn_size);
  if (conn_fd == -1) {
    perror("accept");
  }
  return conn_fd;
}

int main(int argc, char **argv) {
  int server_fd = get_socket(NULL, argv[1], bind);
  listen_socket(server_fd, 4);
  
  while (true) {
    int conn_fd = accept_conn(server_fd);
    pid_t pid = fork();
    if (pid < 0) {
      perror("fork");
      exit(1);
    }
    if (pid == 0) {
      // don't need server_fd for child
      if (close(server_fd) == -1) {
        perror("close");
        exit(1);
      }

      // receive message
      msg m = receive_msg(conn_fd);

      // args to execvp have to be null terminated
      // domain freed when execvp completes
      char* domain = (char *)malloc(m.sz + 1);
      memcpy(domain, m.buf, m.sz);
      free(m.buf);
      domain[m.sz] = '\0';

      // set up args for ping
      char *argv[3];
      argv[0] = (char *)"ping";
      argv[1] = domain;
      argv[2] = NULL;

      // dup stdout and stderr to the socket
      if (dup2(conn_fd, STDOUT_FILENO) == -1) {
        perror("dup2");
        exit(1);
      }
      if (dup2(conn_fd, STDERR_FILENO) == -1) {
        perror("dup2");
        exit(1);
      }
      if (close(conn_fd) == -1) {
        perror("close");
        exit(1);
      }
      
      // run ping cmd that sends to socket
      if (execvp(argv[0], argv) == -1) {
        perror("execvp");
        exit(1);
      }
    }
    if (pid > 0) {
      // a quick hack to make this work,
      // otherwise the parent process eats
      // part of the client's message
      printf("don't press ctrl-c yet\n");
      sleep(3);
      printf("now you can press ctrl-c\n");
      char junk_byte[] = {0};
      recv(conn_fd, junk_byte, 1, 0);
      if (junk_byte[0] == 1) {
        kill(pid, SIGINT);
      }
    }
  }
}
