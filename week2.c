/**
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <wait.h>

int main() {
  pid_t pid = fork();

  if (pid) {
    int wstatus = 0;
    if (waitpid(pid, &wstatus, 0) == -1) {
      perror("waitpid");
      exit(EXIT_FAILURE);
    }

    if (WIFEXITED(wstatus)) {
      printf("Child done with exit status: %d\n", WEXITSTATUS(wstatus));
    } else {
      printf("Child did not exit normally.\n");
    }
  } else {
    if (execl("/usr/bin/exa", "-a", "-l", NULL) == -1) {
      perror("execl");
      exit(EXIT_FAILURE);
    }
  }

  return 0;
}
**/

#include <stdio.h>
#include <unistd.h>
#include
int main() {
  /** for (int i = 0; i < 3; i++) {

     int pid = fork();
     printf("my pid %d : %d \n", i, pid);
     sleep(30);
   }
   */

  /**  int sPid = fork();
    if (sPid != 0) {
      printf("PARENT: pid = %d, ")
    } else {
    }
    prinf
  */
  int pid = fork();
  if (pid != 0) {

    execlp("ls -a");
  } else {
    exec("ls - -l -h");
  }

  printf("%d\n", getpid());

  return 0;
}
