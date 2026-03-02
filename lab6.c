#include <stdio.h>
#include <stdlib.h>

typedef struct Node {
    int value;
    struct Node *next;
} Node;

static Node *node_new(int v) {
    Node *n = (Node *)malloc(sizeof(Node));
    if (!n) {
        perror("malloc");
        exit(1);
    }
    n->value = v;
    n->next = NULL;
    return n;
}

static Node *list_append_buggy(Node *head, int v) {
    Node *n = node_new(v);

    if (head == NULL) {
        return n;
    }

    Node *cur = head;


    while (cur != NULL) {
        cur = cur->next;
    }


    cur->next = n;

    return head;
}

static void list_print(Node *head) {
    for (Node *cur = head; cur != NULL; cur = cur->next) {
        printf("%d ", cur->value);
    }
    printf("\n");
}

static void list_free(Node *head) {
    while (head != NULL) {
        Node *tmp = head->next;
        free(head);
        head = tmp;
    }
}

int main(void) {
    int data[] = {10, 20, 30, 40};

    Node *head = NULL;
    for (size_t i = 0; i < sizeof(data) / sizeof(data[0]); i++) {
        head = list_append_buggy(head, data[i]); 
    }

    list_print(head);
    list_free(head);
    return 0;
}
