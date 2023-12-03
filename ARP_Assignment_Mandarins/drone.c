#include <fcntl.h>
#include <float.h>
#include <limits.h>
#include <math.h>
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

// Function to log a message to the logfile_drone.txt
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
  fp = fopen("../Log/logfile_drone.txt", "a");
  if (fp == NULL) {  // Check for errors
    perror("Error opening logfile");
    return;
  }

  // Print message with time and variables to the file
  fprintf(fp, "[%s] %s value1:%.2f value2:%.2f\n", time_str, message, y, x);

  // Close the file
  fclose(fp);
}

// Function to calculate the repulsive force
double repulsive_force(double coordinate) {
  double smaller_border = coordinate;                // Border near the 0 m
  double larger_border = (DRONE_AREA - coordinate);  // Border near the 100 m
  double force_rep = 0.0;                            // Repulsive force

  // Check if the drone is near the border
  if (smaller_border <= FORCE_FIELD) {  // Near the smaller border
    force_rep += ETA * pow(2, (1 / coordinate - 1 / smaller_border));
  } else if (larger_border <= FORCE_FIELD) {  // Near the larger border
    force_rep -= ETA * pow(2, (1 / coordinate - 1 / larger_border));
  }

  // Return the repulsive force
  return force_rep;
}

// Function to calculate the new coordinates
double command_force(FILE *fp, double co[3][2], double input_force, int dire) {
  // Initialize variables
  double force_rep = 0.0;           // Repulsive force
  double force_sum = 0.0;           // Sum of all forces
  double coordinate = co[0][dire];  // Current coordinate

  // Calculate the repulsive force
  force_rep = repulsive_force(coordinate);

  // Check if there is a repulsive force
  if (force_rep != 0.0) {
    // Log the direction and the repulsive force
    log_message(fp, "Against the border! Direction and repulsive force",
                (double)dire, force_rep);
  }
  // Retrieve the sum of forces (input + repulsive)
  force_sum = input_force + force_rep;
  // Log the sum of forces
  log_message(fp, "Sum of the forces in direction ", (double)dire, force_sum);

  // Calculate the general motion equation
  // Calculated in different parts for debugging purposes
  double part1 = pow(2, T) * force_sum;
  double part2 = co[1][dire] * (2 * M + T * K);
  double part3 = -co[2][dire] * M;
  double con = M + (T * K);

  coordinate = (part1 + part2 + part3) / con;

  // Check for overflow
  if (coordinate > DBL_MAX) {
    perror("OVERFLOW!");
    return -1;
  }

  // Return the new coordinate
  return coordinate;
}

int main() {
  /* VARIABLES */
  int ch;  // Key value
  // Force initialization
  double Fx = 0.0, Fy = .0;
  double real_y = DRONE_AREA / 2;
  double real_x = DRONE_AREA / 2;
  double new_real_y = 0.0, new_real_x = 0.0;
  // Initialize the struct
  struct shared_data data = {.real_y = 0.0, .real_x = 0.0, .ch = 0};
  // Save previous xi-1 and xi-2
  double coordinates[3][2] = {
      {real_y, real_x}, {real_y, real_x}, {real_y, real_x}};
  // Initialize the counter for signal sending
  int counter = 0;
  // Logfile
  FILE *fp;

  // Create a fifo to recieve the value of the key pressed
  char *myfifo = "/tmp/myfifo";
  mkfifo(myfifo, 0666);
  int fd;
  char str[MAX_LEN];
  char format_string[MAX_LEN] = "%d";

  /* ACCESSING SHARED MEMORY */
  // For creating the shared memory
  int shm_size = sizeof(struct shared_data);  // Get the size of the struct
  // Open the semaphore
  sem_t *sem_id = sem_open(SEM_PATH, 0);
  if (sem_id == SEM_FAILED) {  // Check for errors
    perror("sem_open failed");
    return -1;
  }

  // Open shared memory
  int shmfd = shm_open(SHM_PATH, O_RDWR, S_IRWXU | S_IRWXG);
  if (shmfd == -1) {  // Check for errors
    perror("shm_open failed");
    return -1;
  }

  // Create shared memory pointer and map
  struct shared_data *shm_ptr =
      mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  if (shm_ptr == MAP_FAILED) {  // Check for errors
    perror("mmap failed");
    return -1;
  }

  // For the use of select
  fd_set rdfds;
  int retval;
  // Set the timeout to 50 milliseconds
  struct timeval tv;
  tv.tv_sec = 0;
  tv.tv_usec = 50000;

  // Open the logfile to either create or delete the content
  fp = fopen("../Log/logfile_drone.txt", "w");
  if (fp == NULL) {  // Check for error
    perror("Error opening logfile");
    return -1;
  }
  fclose(fp);  // Close the file

  /* WATCHDOG */
  pid_t watchdog_pid;          // For watchdog PID
  pid_t drone_pid = getpid();  // Recieve the process PID

  // Get the file locations
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;

  // Open the drone process tmp file to write its PID
  FILE *pid_fp = fopen(fnames[DRONE_SYM], "w");
  if (pid_fp == NULL) {  // Check for errors
    perror("Error opening Drone tmp file");
    return -1;
  }
  fprintf(pid_fp, "%d", drone_pid);
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
  if (watchdog_fp == NULL) {  // Check for errors
    perror("Error opening watchdog PID file");
    return -1;
  }

  // Read the watchdog PID from the file
  if (fscanf(watchdog_fp, "%d", &watchdog_pid) != 1) {
    perror("Error reading Watchdog PID from file.\n");
    fclose(watchdog_fp);
    return -1;
  }

  // Close the file
  fclose(watchdog_fp);

  while (1) {
    // Open fifo for reading the key from the keyboard process
    fd = open(myfifo, O_RDONLY);
    if (fd == -1) {
      perror("Opening fifo failed ");
      return -1;
    }

    FD_ZERO(&rdfds);     // Reset the reading set
    FD_SET(fd, &rdfds);  // Put the fd in the set for monitoring
    retval = select(fd + 1, &rdfds, NULL, NULL, &tv);

    // Problem using select
    if (retval == -1) {
      perror("Select() failed ");
      return -1;
    }
    // Data is available
    else if (retval > 0 && FD_ISSET(fd, &rdfds)) {
      // Read from the file descriptor
      if ((read(fd, str, sizeof(str))) == -1) {
        perror("Reading from fifo failed ");
        return -1;
      }
      // Convert the string back to integer
      ch = atoi(str);
      // Log the recieved key
      log_message(fp, "Recieved key ", (double)ch, (double)ch);
      // Close fd
      close(fd);

      // Update the character position based on user input
      switch (ch) {
        // Move left up 'w'
        case KEY_LEFT_UP:
          Fy += -Force;
          Fx += -Force;
          break;
        // Move left 's'
        case KEY_LEFT_s:
          Fy = 0.0;
          Fx += -Force;
          break;
        // Move left down 'x'
        case KEY_LEFT_DOWN:
          Fy += Force;
          Fx += -Force;
          break;
        // Move up 'e'
        case KEY_UP_e:
          Fy += -Force;
          Fx = 0.0;
          break;
        // Move down 'c'
        case KEY_DOWN_c:
          Fy += Force;
          Fx = 0.0;
          break;
        // Move right down 'v'
        case KEY_RIGHT_DOWN:
          Fy += Force;
          Fx += Force;
          break;
        // Move right 'f'
        case KEY_RIGHT_f:
          Fy = 0;
          Fx += Force;
          break;
        // Move up right 'r'
        case KEY_RIGHT_UP:
          Fy += -Force;
          Fx += Force;
          break;
        // Break
        case KEY_STOP:
          Fy = 0;
          Fx = 0;
          break;
      }
    }
    // Data is not available
    else {
      // Set key to not pressed
      ch = 0;
    }

    // Log the values before calculating new coordinates
    log_message(fp, "Before recieving input force ", Fy, Fx);
    log_message(fp, "Before recieving coordinates ", real_y, real_x);

    // If esc is pressed exit process
    if (ch == ESCAPE) {
      // Update shared memory with the key pressed and forces
      data.ch = ch;
      sem_wait(sem_id);                  // Wait for semaphore
      memcpy(shm_ptr, &data, shm_size);  // Copy local to memory cells
      sem_post(sem_id);                  // Post the sempahore as soon as done
      break;                             // Leave the loop
    }
    // If 'K' is pressed restart game
    else if (ch == RESTART) {
      // Set the drone in the middle of the game arena
      new_real_y = DRONE_AREA / 2;
      new_real_x = DRONE_AREA / 2;
      // Log the reset and new coordinates
      log_message(fp, "RESET! New coordinates ", new_real_y, new_real_x);

      // Reset all previous coordinates
      coordinates[0][0] = new_real_x;
      coordinates[0][1] = new_real_y;
      coordinates[2][0] = new_real_x;
      coordinates[2][1] = new_real_y;
      coordinates[1][0] = new_real_x;
      coordinates[1][1] = new_real_y;

      // Appoint forces to zero
      Fy = 0;
      Fx = 0;
    }
    // Game continues
    else {
      // Calculate the new coordinates from the dynamic force
      // Force in the y direction
      coordinates[0][y_direction] =
          command_force(fp, coordinates, Fy, y_direction);
      // Force in the x direction
      coordinates[0][x_direction] =
          command_force(fp, coordinates, Fx, x_direction);

      // Calculate the new locations
      new_real_y = coordinates[0][y_direction];
      new_real_x = coordinates[0][x_direction];

      // Update previous coordinates
      coordinates[2][0] = coordinates[1][0];
      coordinates[2][1] = coordinates[1][1];
      coordinates[1][0] = new_real_x;
      coordinates[1][1] = new_real_y;
    }

    // Log the calculated forces and coordinates
    log_message(fp, "After recieving input force ", Fy, Fx);
    log_message(fp, "After recieving coordinates ", new_real_y, new_real_x);

    // Update shared memory with the key pressed and forces
    data.ch = ch;
    data.real_y = new_real_y;
    data.real_x = new_real_x;
    sem_wait(sem_id);                  // Wait for semaphore
    memcpy(shm_ptr, &data, shm_size);  // Copy local to memory cells
    sem_post(sem_id);                  // Post semaphore as soon as done

    // Send a signal after certain iterations have passed
    if (counter == PROCESS_SIGNAL_INTERVAL) {
      // Send signal to watchdog every process signal interval
      if (kill(watchdog_pid, SIGUSR1) < 0) {
        perror("kill");
      }
      // Set counter to zero (start over)
      counter = 0;
    } else {
      // Increment the counter
      counter++;
    }

    // Open the logfile
    fp = fopen("../Log/logfile_drone.txt", "a");
    if (fp == NULL) {  // Check for errors
      perror("Error opening logfile");
      return -1;
    }
    fprintf(fp, "\n");  // Start from the new row
    fclose(fp);         // Close the file

    // Wait for amount of time
    usleep(WAIT_TIME);
  }

  // Clean up
  shm_unlink(SHM_PATH);
  sem_close(sem_id);
  sem_unlink(SEM_PATH);

  // End the drone program
  return 0;
}