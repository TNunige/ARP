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
  // Initialize a new window
  WINDOW *local_win;

  // Create the window
  local_win = newwin(height, width, starty, startx);
  // Draw a box around the window
  box(local_win, 0, 0);  // Give default characters (lines) as the box
  // Refresh window to show the box
  wrefresh(local_win);

  return local_win;
}

// Function to print the character at a desired location
void print_character(WINDOW *win, int y, int x, char *character) {
  // Print the character at the desired location in blue
  wattron(win, COLOR_PAIR(1));
  mvwprintw(win, y, x, "%c", *character);
  wattroff(win, COLOR_PAIR(1));
  wrefresh(win);

  // Update box of the window
  box(win, 0, 0);
  wrefresh(win);
}

// Function to log a message to the logfile_window.txt
void log_message(FILE *fp, const char *message, double y, double x) {
  // Get the current time
  time_t now = time(NULL);
  struct tm *timenow;
  time(&now);
  timenow = gmtime(&now);

  // Format time as a string
  char time_str[26];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timenow);

  // Open the logfile
  fp = fopen("../Log/logfile_window.txt", "a");
  if (fp == NULL) {  // Check for errors
    perror("Error opening logfile");
    return;
  }

  // Print message with time and variables to the file
  fprintf(fp, "[%s] %s y:%.2f x:%.2f\n", time_str, message, y, x);

  // Close the file
  fclose(fp);
}

// Main function starts
int main() {
  /* VARIABLES */
  // Values for creating the winow
  WINDOW *main_win, *inspection_win;
  int height1, width1;
  int startx1, starty1, titlex1;
  int height2, width2, startx2, starty2, titlex2;
  // Initialize the struct outside the while loop
  struct shared_data data = {.real_y = 0.0, .real_x = 0.0, .ch = 0};
  // Logfile
  FILE *fp;
  // For coordinates of the drone
  double old_realy = 0.0, old_realx = 0.0;
  int main_x = 0, main_y = 0;
  int ch;
  // Initialize the counter for signal sending
  int counter = 0;

  // Open the logfile to either create or delete the content
  fp = fopen("../Log/logfile_window.txt", "w");
  if (fp == NULL) {  // Check for error
    perror("Error opening logfile");
    return -1;
  }
  // Close the file
  fclose(fp);

  /* ACCESSING SHARED MEMORY */
  // Get the size of the struct
  int shm_size = sizeof(struct shared_data);
  // Open the semaphore
  sem_t *sem_id = sem_open(SEM_PATH, 0);
  if (sem_id == SEM_FAILED) {  // Check for errors
    perror("sem_open failed");
    return -1;
  }
  // Open shared memory
  int shmfd = shm_open(SHM_PATH, O_RDWR, S_IRWXU | S_IRWXG);
  if (shmfd == -1) {  // Check for errors
    perror("Opening shared memory failed");
    return -1;
  }
  // Map pointer to shared memory
  struct shared_data *shm_ptr =
      mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  if (shm_ptr == MAP_FAILED) {  // Check for errors
    perror("mmap failed");
    return -1;
  }

  /* Start ncurses mode */
  initscr();
  cbreak();
  noecho();

  // Enable color
  start_color();
  init_pair(1, COLOR_BLUE, COLOR_BLACK);  // Color pair for blue

  // Print out a welcome message on the screen
  printw("Welcome to the Drone simulator\n");
  refresh();

  /* CREATING WINDOWS */
  // Create the main game window
  height1 = LINES * 0.8;
  width1 = COLS;
  starty1 = (LINES - height1) / 2;
  startx1 = 0;
  main_win = create_newwin(height1, width1, starty1, startx1);
  // Print the title of the window
  titlex1 = (width1 - strlen("Drone game")) / 2;
  mvprintw(starty1 - 1, startx1 + titlex1, "Drone game");
  wrefresh(main_win);  // Refresh window

  // Print "SCORE" under the main window
  mvprintw(starty1 + height1, startx1, "SCORE");
  refresh();

  /* PRINTING THE DRONE */
  // Real life to ncurses window scaling factors
  double scale_y = (double)DRONE_AREA / height1;
  double scale_x = (double)DRONE_AREA / width1;

  // Setting the real coordinates to middle of the geofence
  double real_y = DRONE_AREA / 2;
  double real_x = DRONE_AREA / 2;

  // Convert real coordinates for ncurses window
  main_y = (int)(real_y / scale_y);
  main_x = (int)(real_x / scale_x);
  // Add the character in the middle of the window
  print_character(main_win, main_y, main_x, "X");
  // Update character coordinates to the logfile
  log_message(fp, "Printed character (main) ", (double)main_y, (double)main_x);

  /* WATCHDOG */
  pid_t Watchdog_pid;           // For watchdog PID
  pid_t window_pid = getpid();  // Recieve the process PID

  // Get the file locations
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;

  // Open the window process tmp file to write its PID
  FILE *pid_fp = fopen(fnames[WINDOW_SYM], "w");
  if (pid_fp == NULL) {  // Check for errors
    perror("Error opening Window tmp file");
    return -1;
  }
  fprintf(pid_fp, "%d", window_pid);
  fclose(pid_fp);  // Close file

  // Read watchdog PID
  FILE *watchdog_fp = NULL;
  struct stat sbuf;

  // Check if the file size is bigger than 0
  if (stat(PID_FILE_PW, &sbuf) == -1) {
    perror("error-stat");
    return -1;
  }
  // Wait until the file has data
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

  // Start the loop
  while (1) {
    // Recieve key pressed and new coordinates from the shared memory
    sem_wait(sem_id);                  // Wait for semaphore
    memcpy(&data, shm_ptr, shm_size);  // Copy memory to local cells
    sem_post(sem_id);                  // Post semaphore as soon as done
    ch = data.ch;                      // Get the key pressed by the user
    real_y = data.real_y;              // Get the new y coordinate (real life)
    real_x = data.real_x;              // Get the new x coordinate (real life)

    // Update the read data to logfile
    log_message(fp, "Recieved from drone (real) ", real_y, real_x);

    // Delete the drone at the current position
    print_character(main_win, main_y, main_x, " ");

    // Check if the esc button has been pressed
    if (ch == ESCAPE) {
      // Log the exit of the program and the last positon of the drone
      log_message(fp, "Exiting the program at states ", real_y, real_x);
      sleep(1);
      break;  // Leave the loop
    }

    // Convert the real coordinates for the ncurses window using scales
    main_y = (int)(real_y / scale_y);
    main_x = (int)(real_x / scale_x);

    // Add the character to the desired position
    print_character(main_win, main_y, main_x, "X");

    // Update the new location to the logfile
    log_message(fp, "Printed character (main) ", (double)main_y,
                (double)main_x);

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

  // Clean up semaphores and shared memory
  shm_unlink(SHM_PATH);
  sem_close(sem_id);
  sem_unlink(SEM_PATH);
  munmap(shm_ptr, shm_size);

  // End the window program
  return 0;
}
