//Kimchhorn Sambath

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <errno.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>


#define SERVER_IP "127.0.0.1"
#define SERVER_PORT 5000
#define MSG_SIZE 128
#define NUM_MSG_PER_CLIENT 5

const char *messages[NUM_MSG_PER_CLIENT] = {
    "Hello",
    "Apple",
    "Car",
    "Green",
    "Dog"
};

int main_client(void) {
    int sockfd;
    struct sockaddr_in server_addr;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("client socket");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("client connect");
        close(sockfd);
        return 1;
    }

    for (int i = 0; i < NUM_MSG_PER_CLIENT; i++) {
        send(sockfd, messages[i], strlen(messages[i]) + 1, 0);
        printf("Sent: %s\n", messages[i]);
        usleep(100000);
    }

    close(sockfd);
    return 0;
}

#define MAX_CLIENTS 4
#define BACKLOG 10

typedef struct msg_node {
    char msg[MSG_SIZE];
    struct msg_node *next;
} msg_node_t;

typedef struct client_info {
    int client_fd;
    pthread_t tid;
    int run;
} client_info_t;

typedef struct server_state {
    int listen_fd;
    int run_acceptor;

    client_info_t clients[MAX_CLIENTS];
    int num_clients;

    msg_node_t *head;
    msg_node_t *tail;
    int num_messages;

    pthread_mutex_t list_mutex;
    pthread_mutex_t count_mutex;
    pthread_mutex_t client_mutex;
} server_state_t;

void make_socket_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return;
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

void add_to_list(server_state_t *server, msg_node_t *node) {
    if (server->head == NULL) {
        server->head = node;
        server->tail = node;
    } else {
        server->tail->next = node;
        server->tail = node;
    }
}

void *run_client(void *arg) {
    client_info_t *client = ((client_info_t **)arg)[0];
    server_state_t *server = ((server_state_t **)arg)[1];
    free(arg);

    char buffer[MSG_SIZE];

    while (client->run) {
        memset(buffer, 0, sizeof(buffer));
        ssize_t n = recv(client->client_fd, buffer, sizeof(buffer) - 1, 0);

        if (n > 0) {
            msg_node_t *node = malloc(sizeof(msg_node_t));
            if (node != NULL) {
                memset(node, 0, sizeof(msg_node_t));
                strncpy(node->msg, buffer, MSG_SIZE - 1);
                node->next = NULL;

                pthread_mutex_lock(&server->list_mutex);
                add_to_list(server, node);
                pthread_mutex_unlock(&server->list_mutex);

                pthread_mutex_lock(&server->count_mutex);
                server->num_messages++;
                pthread_mutex_unlock(&server->count_mutex);

                printf("Collected: %s\n", node->msg);
            }
        } else if (n == 0) {
            break; // client disconnected
        } else {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);
                continue;
            } else {
                break;
            }
        }
    }

    return NULL;
}

void *run_acceptor(void *arg) {
    server_state_t *server = (server_state_t *)arg;

    while (server->run_acceptor) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server->listen_fd, (struct sockaddr *)&client_addr, &client_len);

        if (client_fd < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                usleep(10000);
                continue;
            } else {
                perror("accept");
                break;
            }
        }

        make_socket_nonblocking(client_fd);

        pthread_mutex_lock(&server->client_mutex);

        if (server->num_clients >= MAX_CLIENTS) {
            pthread_mutex_unlock(&server->client_mutex);
            close(client_fd);
            printf("Not accepting any more clients!\n");
            continue;
        }

        int idx = server->num_clients;
        server->clients[idx].client_fd = client_fd;
        server->clients[idx].run = 1;
        server->num_clients++;

        printf("Client connected!\n");

        // create one thread per connected client
        void **thread_args = malloc(sizeof(void *) * 2);
        if (thread_args != NULL) {
            thread_args[0] = &server->clients[idx];
            thread_args[1] = server;

            pthread_create(&server->clients[idx].tid, NULL, run_client, thread_args);
        }

        pthread_mutex_unlock(&server->client_mutex);
    }

    // clean shutdown for client threads
    pthread_mutex_lock(&server->client_mutex);
    for (int i = 0; i < server->num_clients; i++) {
        server->clients[i].run = 0;
    }
    pthread_mutex_unlock(&server->client_mutex);

    for (int i = 0; i < server->num_clients; i++) {
        pthread_join(server->clients[i].tid, NULL);
        close(server->clients[i].client_fd);
    }

    return NULL;
}

void print_messages(server_state_t *server) {
    pthread_mutex_lock(&server->list_mutex);

    msg_node_t *curr = server->head;
    while (curr != NULL) {
        printf("Collected: %s\n", curr->msg);
        curr = curr->next;
    }

    pthread_mutex_unlock(&server->list_mutex);
}

void free_messages(server_state_t *server) {
    pthread_mutex_lock(&server->list_mutex);

    msg_node_t *curr = server->head;
    while (curr != NULL) {
        msg_node_t *tmp = curr;
        curr = curr->next;
        free(tmp);
    }

    server->head = NULL;
    server->tail = NULL;

    pthread_mutex_unlock(&server->list_mutex);
}

int main(void) {
    // If you want this file to act as client instead, temporarily use:
    // return main_client();

    server_state_t server;
    memset(&server, 0, sizeof(server));

    pthread_mutex_init(&server.list_mutex, NULL);
    pthread_mutex_init(&server.count_mutex, NULL);
    pthread_mutex_init(&server.client_mutex, NULL);

    server.run_acceptor = 1;
    server.num_clients = 0;
    server.num_messages = 0;
    server.head = NULL;
    server.tail = NULL;

    server.listen_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server.listen_fd < 0) {
        perror("server socket");
        return 1;
    }

    int opt = 1;
    setsockopt(server.listen_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    server_addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(server.listen_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("bind");
        close(server.listen_fd);
        return 1;
    }

    if (listen(server.listen_fd, BACKLOG) < 0) {
        perror("listen");
        close(server.listen_fd);
        return 1;
    }

    make_socket_nonblocking(server.listen_fd);

    pthread_t acceptor_tid;
    pthread_create(&acceptor_tid, NULL, run_acceptor, &server);

    // wait until enough messages are received
    while (1) {
        int count_now;

        pthread_mutex_lock(&server.count_mutex);
        count_now = server.num_messages;
        pthread_mutex_unlock(&server.count_mutex);

        if (count_now >= MAX_CLIENTS * NUM_MSG_PER_CLIENT) {
            printf("Collected: %d\n", count_now);
            break;
        }

        usleep(10000);
    }

    printf("All messages were collected!\n");

    server.run_acceptor = 0;
    pthread_join(acceptor_tid, NULL);

    close(server.listen_fd);

    print_messages(&server);
    free_messages(&server);

    pthread_mutex_destroy(&server.list_mutex);
    pthread_mutex_destroy(&server.count_mutex);
    pthread_mutex_destroy(&server.client_mutex);

    return 0;
}
