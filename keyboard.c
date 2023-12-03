#include <fcntl.h>
#include <ncurses.h>
#include <semaphore.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <sys/select.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "Include/constants.h"

// Function to create a new ncurses window
WINDOW *create_newwin(int height, int width, int starty, int startx) {
  WINDOW *local_win;  // Initialize a new window

  // Create the window
  local_win = newwin(height, width, starty, startx);
  box(local_win, 0, 0);  // Give default characters (lines) as the box
  // Refresh window to show the box
  wrefresh(local_win);

  return local_win;
}

// Function to print the message on the inspection window
void print_message(WINDOW *win, const char *message, int *message_index,
                   int width) {
  // Clear the oldest message if the buffer is full
  if (*message_index >= MAX_MESSAGES) {
    wmove(win, 1, 1);
    wdeleteln(win);
    (*message_index)--;
  }

  // Print the new message at the top within the specified width
  mvwprintw(win, *message_index + 1, 1, "%-*s", width - 2, message);
  wrefresh(win);

  // Update the box
  box(win, 0, 0);
  wrefresh(win);

  // Increment the message_index
  (*message_index)++;
}

int main() {
  /* WATCHDOG */
  pid_t Watchdog_pid;             // For watchdog PID
  pid_t keyboard_pid = getpid();  // Recieve the process PID

  // Get the file locations
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;

  // Open the keyboard process tmp file to write its PID
  FILE *pid_fp = fopen(fnames[KEYBOARD_SYM], "w");
  if (pid_fp == NULL) {  // Check for errors
    perror("Error opening Window tmp file");
    return -1;
  }
  fprintf(pid_fp, "%d", keyboard_pid);
  fclose(pid_fp);  // Close file

  // Read watchdog pid
  FILE *watchdog_fp;
  struct stat sbuf;

  // Check if the file size is bigger than 0
  if (stat(PID_FILE_PW, &sbuf) == -1) {
    perror("error-stat");
    return -1;
  }
  // Waits until the file has data
  while (sbuf.st_size <= 0) {
    if (stat(PID_FILE_PW, &sbuf) == -1) {
      perror("error-stat");
      return -1;
    }
    usleep(50000);
  }

  // Open the watchdog tmp file
  watchdog_fp = fopen(PID_FILE_PW, "r");
  if (watchdog_fp == NULL) {
    perror("Error opening watchdog PID file");
    return -1;
  }

  // Read the watchdog PID from the file
  if (fscanf(watchdog_fp, "%d", &Watchdog_pid) != 1) {
    printf("Error reading Watchdog PID from file.\n");
    fclose(watchdog_fp);
    return -1;
  }

  // Close the file
  fclose(watchdog_fp);

  // Initialize the counter for signal sending
  int counter = 0;

  // Initialize variables for fifo
  int fd;
  // Create a fifo (named pipe) to send the value of the key pressed
  char *myfifo = "/tmp/myfifo";
  mkfifo(myfifo, 0666);
  char str[MAX_LEN];
  char format_string[MAX_LEN] = "%d";

  // Buffer for messages on the inspection window
  char messages[MAX_MESSAGES][MAX_LEN];
  int message_index = 0;

  // Values for creating the winow
  WINDOW *inspection_win;
  int height, width;
  int startx, starty, titlex;
  int ch;

  // Start ncurses mode
  initscr();
  cbreak();
  noecho();
  nodelay(stdscr, TRUE);  // make getch() non blocking

  // Print messages on the screen
  printw("KEYBOARD\n");
  printw("Press K to restart\n");
  printw("Press Esc to exit\n");
  printw("\n");
  refresh();

  // Enable the keyboard
  keypad(stdscr, TRUE);

  // Create the inspection window
  height = LINES * 0.6;
  width = COLS;
  starty = LINES - height;
  startx = 0;
  inspection_win = create_newwin(height, width, starty, startx);
  // Print the title
  titlex = (width - strlen("Inspection")) / 2;
  mvprintw(starty - 1, startx + titlex, "Inspection");
  wrefresh(inspection_win);
  wmove(inspection_win, starty - 1, startx - 1);  // Move the cursor
  refresh();

  while (1) {
    // Open fifo for writing
    fd = open(myfifo, O_WRONLY);
    if (fd == -1) {
      perror("Opening fifo failed ");
      return -1;
    }
    // Scan the user input
    ch = getch();
    refresh();

    sprintf(str, format_string, ch);  // Format it to a string
    write(fd, str, sizeof(str));      // Write the data to fifo
    usleep(WAIT_TIME);                // Wait
    close(fd);                        // Close fd

    // Check if the user has pressed a key
    if (ch != ERR) {
      // Check if esc button has been pressed and exit game
      if (ch == ESCAPE) {
        print_message(inspection_win, "Exiting the program...\n",
                      &message_index, width);
        sleep(1);  // Wait
        break;     // Exit loop
      }
      // Other key was pressed
      else {
        // Update the inspection window message based on the user input
        switch (ch) {
          // Move left up 'w'
          case KEY_LEFT_UP:
            print_message(inspection_win, "Moving up-left\n", &message_index,
                          width);
            break;
          // Move left 's'
          case KEY_LEFT_s:
            print_message(inspection_win, "Moving left\n", &message_index,
                          width);
            break;
          // Move left down 'x'
          case KEY_LEFT_DOWN:
            print_message(inspection_win, "Moving down-left\n", &message_index,
                          width);
            break;
          // Move up 'e'
          case KEY_UP_e:
            print_message(inspection_win, "Moving up\n", &message_index, width);
            break;
          // Move down 'c'
          case KEY_DOWN_c:
            print_message(inspection_win, "Moving down\n", &message_index,
                          width);
            break;
          // Move right down 'v'
          case KEY_RIGHT_DOWN:
            print_message(inspection_win, "Moving down-right\n", &message_index,
                          width);
            break;
          // Move right 'f'
          case KEY_RIGHT_f:
            print_message(inspection_win, "Moving right\n", &message_index,
                          width);
            break;
          // Move up right 'r'
          case KEY_RIGHT_UP:
            print_message(inspection_win, "Moving right-up\n", &message_index,
                          width);
            break;
          case KEY_STOP:
            print_message(inspection_win, "Break\n", &message_index, width);
            break;
          case RESTART:
            print_message(inspection_win, "Restart game\n", &message_index,
                          width);
            break;
        }
      }
    }
    // Refresh the screen
    refresh();

    // Send a signal after certain iterations have passed
    if (counter == PROCESS_SIGNAL_INTERVAL) {
      // send signal to watchdog every process signal interval
      if (kill(Watchdog_pid, SIGUSR1) < 0) {
        perror("kill");
      }
      // Set counter to zero (start over)
      counter = 0;
    } else {
      // Increment the counter
      counter++;
    }

    // Wait for a certain time
    usleep(WAIT_TIME);
  }
  // End ncurses mode
  endwin();

  // End the keyboard program
  return 0;
}