#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main() {
  char *buff = NULL;
  size_t cap = 0;
  ssize_t nread;

  while (1) {

    printf("HelloPlease enter some text:");

    fflush(stdout);

    nread = getline(&buff, &cap, stdin);
    if (nread == -1) {
      free(buff);
      return 1;
    }

    if (nread > 0) {
      printf("Token:\n");
      char *saveptr = NULL;
      char *token = strtok_r(buff, " ", &saveptr);
      while (tok != NULL) {
        printf("%s\n", token);
        token = strtok_r(buff, " ", &saveptr);
      }
    }
  }
  :
  free(buff);
  return 0;
}
