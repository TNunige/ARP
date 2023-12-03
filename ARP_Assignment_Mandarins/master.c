#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include "Include/constants.h"

// Function to spawn all the processes with using execvp()
int spawn(const char *program, char **arg_list) {
  execvp(program, arg_list);
  perror("Process execution failed");  // Check for errors
  return -1;
}

// Main function starts
int main() {
  // To recieve the exit statuses of the child processes
  int exit_status[NUM_PROCESSES + 1];  // With Watchdog
  int status;

  // Clear all file contents of watchdog file
  FILE *file_pw = fopen(PID_FILE_PW, "w");
  if (file_pw == NULL) {  // Check for errors
    perror("Error opening PID_FILE_PW");
    return -1;
  }
  fclose(file_pw);  // Close file

  // Clear all file contents of other process files
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;
  for (int i = 0; i < NUM_PROCESSES; i++) {
    FILE *file_sp = fopen(fnames[i], "w");
    if (file_sp == NULL) {  // Check for errors
      perror("Error opening a file in fnames");
      return 1;
    }
    fclose(file_sp);  // Close file
  }

  // Argument lists to pass to the command
  char *arg_list1[] = {"./watchdog", NULL};
  char *arg_list2[] = {"./blackboard", NULL};
  char *arg_list3[] = {"konsole","-e","./window", NULL};
  //char *arg_list3[] = {"konsole", "--geometry", "1300x850+100+100",
  //                     "-e",      "./window",   "-lncurses",
  //                     NULL};  // Using xdotool and wmctrl to create the konsole
  //                             // at a specific location
  char *arg_list4[] = {"./drone", "-lm", NULL};
  char *arg_list5[] = {"konsole","-e","./keyboard", NULL};
  //char *arg_list5[] = {"konsole", "--geometry", "400x850+1400+100",
  //                     "-e",      "./keyboard", "-lncurses",
  //                     NULL};  // Using xdotool and wmctrl to create the konsole
                               // at a specific location

  /* SPAWNING CHILDREN*/
  // Spawning watchdog
  pid_t id1 = fork();

  // Error check
  if (id1 < 0) {
    perror("Error using id1 fork");
    return -1;
  }
  // Child process
  else if (id1 == 0) {
    printf("Spawning watchdog \n");
    spawn(arg_list1[0], arg_list1);
  }

  // Spawning blackboard
  pid_t id2 = fork();

  // Error check
  if (id2 < 0) {
    perror("Error using id2 fork");
    return -1;
  }
  // Child process
  else if (id2 == 0) {
    printf("Spawning blackboard\n");
    spawn(arg_list2[0], arg_list2);
  }

  // Spawning window
  pid_t id3 = fork();

  // Error check
  if (id3 < 0) {
    perror("Error using id3 fork");
    return -1;
  }
  // Child process
  else if (id3 == 0) {
    printf("Spawning window\n");
    spawn(arg_list3[0], arg_list3);
  }

  // Spawning drone
  pid_t id4 = fork();

  // Error check
  if (id4 < 0) {
    perror("Error using id4 fork");
    return -1;
  }
  // Child process
  else if (id4 == 0) {
    printf("Spawning drone\n");
    spawn(arg_list4[0], arg_list4);
  }

  // Spawning keyboard
  pid_t id5 = fork();

  // Error check
  if (id5 < 0) {
    perror("Error using id5 fork");
    return -1;
  }
  // Child process
  else if (id5 == 0) {
    printf("Spawning keyboard\n");
    spawn(arg_list5[0], arg_list5);
  }

  // Wait for all children to terminate
  for (int i = 0; i < (NUM_PROCESSES + 1); i++) {
    wait(&status);
    exit_status[i] = WEXITSTATUS(status);
    wait(NULL);
  }

  // Print children termination statuses
  for (int i = 0; i < (NUM_PROCESSES + 1); i++) {
    printf("Process %d terminated with status: %d\n", i + 1, exit_status[i]);
  }

  // End the master process
  return 0;
}