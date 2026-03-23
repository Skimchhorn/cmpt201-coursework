/*
Questions to answer at top of client.c:
(You should not need to change the code in client.c)
1. What is the address of the server it is trying to connect to (IP address and port number).
127.0.0.1:8000
2. Is it UDP or TCP? How do you know?
TCP. The socket is created with SOCK_STREAM, which indicates a TCP socket. SOCK_DGRAM -> UDP socket 
3. The client is going to send some data to the server. Where does it get this data from? How can you tell in the code?
The client gets the data from standard input (STDIN_FILENO).
4. How does the client program end? How can you tell that in the code?
The client program ends when it finishes reading from standard input and closes the socket. and also exit(EXIT_SUCCESS)
*/

#include <arpa/inet.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define PORT 8000
#define BUF_SIZE 64
#define ADDR "127.0.0.1"

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

int main() {
  struct sockaddr_in addr;
  int sfd;
  ssize_t num_read;
  char buf[BUF_SIZE];

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET;
  addr.sin_port = htons(PORT);
  if (inet_pton(AF_INET, ADDR, &addr.sin_addr) <= 0) {
    handle_error("inet_pton");
  }

  int res = connect(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in));
  if (res == -1) {
    handle_error("connect");
  }

  while ((num_read = read(STDIN_FILENO, buf, BUF_SIZE)) > 1) {
    if (write(sfd, buf, num_read) != num_read) {
      handle_error("write");
    }
    printf("Just sent %zd bytes.\n", num_read);
  }

  if (num_read == -1) {
    handle_error("read");
  }

  close(sfd);
  exit(EXIT_SUCCESS);
}

#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#define BUF_SIZE 64
#define PORT 8000
#define LISTEN_BACKLOG 32

#define handle_error(msg)                                                      \
  do {                                                                         \
    perror(msg);                                                               \
    exit(EXIT_FAILURE);                                                        \
  } while (0)

// Shared counters for: total # messages, and counter of clients (used for
// assigning client IDs)
int total_message_count = 0;
int client_id_counter = 1;

// Mutexs to protect above global state.
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;
pthread_mutex_t client_id_mutex = PTHREAD_MUTEX_INITIALIZER;

struct client_info {
  int cfd;
  int client_id;
};

void *handle_client(void *arg) {
  struct client_info *client = (struct client_info *)arg;

  // TODO: print the message received from client
  char buf[BUF_SIZE];
  ssize_t num_read;
  while ((num_read = read(client->cfd, buf, BUF_SIZE)) > 0) {
    buf[num_read] = '\0';
    printf("Client ID %d: %s", client->client_id, buf);
    // TODO: increase total_message_count per message
    pthread_mutex_lock(&count_mutex);
    total_message_count++;
    printf("Total message count: %d\n", total_message_count);
    pthread_mutex_unlock(&count_mutex);
  }

  if (num_read == -1) {
    handle_error("read");
  }

  if (close(client->cfd) == -1) {
    handle_error("close");
  }

  return NULL;
}

int main() {
  struct sockaddr_in addr;
  int sfd;

  sfd = socket(AF_INET, SOCK_STREAM, 0);
  if (sfd == -1) {
    handle_error("socket");
  }

  memset(&addr, 0, sizeof(struct sockaddr_in));
  addr.sin_family = AF_INET; //ipv4 family
  addr.sin_port = htons(PORT); //sets the port number, converting from host byte order to network byte order
  addr.sin_addr.s_addr = htonl(INADDR_ANY); //sets the IP address to accept connections from any interface, converting from host byte order to network byte order

  if (bind(sfd, (struct sockaddr *)&addr, sizeof(struct sockaddr_in)) == -1) { //binds the socket to the specified address and port, allowing it to receive incoming connections on that address and port
    handle_error("bind");
  }

  if (listen(sfd, LISTEN_BACKLOG) == -1) { //tells socket to start listening for incoming connections, with a specified backlog of pending connections
    handle_error("listen");
  }

  for (;;) { //infinite loop to accept and handle incoming connections continuously
    // TODO: create a new thread when a new connection is encountered
      struct sockaddr_in client_addr;
      socklen_t client_addr_len = sizeof(struct sockaddr_in);
      int cfd = accept(sfd, (struct sockaddr *)&client_addr, &client_addr_len);
      if (cfd == -1) {
        handle_error("accept");
      }
  
      struct client_info *client = malloc(sizeof(struct client_info));
      if (client == NULL) {
        handle_error("malloc");
      }
      client->cfd = cfd;
  
      pthread_mutex_lock(&client_id_mutex);
      client->client_id = client_id_counter++;
      pthread_mutex_unlock(&client_id_mutex);
    // TODO: call handle_client() when launching a new thread, and provide
    // client_info
      pthread_t thread_id;
      if (pthread_create(&thread_id, NULL, handle_client, client) != 0) {
        handle_error("pthread_create");
      }
  }

  if (close(sfd) == -1) {
    handle_error("close");
  }

  return 0;
}
