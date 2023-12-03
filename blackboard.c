#include <fcntl.h>
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

// Function to log a message to the logfile_memory.txt
void log_message(FILE *fp, const char *message, struct shared_data *data) {
  // Get the current time
  time_t now = time(NULL);
  struct tm *timenow;
  time(&now);
  timenow = gmtime(&now);

  // Format time as a string
  char time_str[26];
  strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", timenow);

  // Open the logfile
  fp = fopen("../Log/logfile_memory.txt", "a");
  if (fp == NULL) {  // Check for errors
    perror("Error opening logfile");
    return;
  }

  // Print message with time and variables to the file
  fprintf(fp, "[%s] %s ch:%d y:%f x:%f\n", time_str, message, data->ch,
          data->real_y, data->real_x);

  // Close the file
  fclose(fp);
}

// Main function starts
int main() {
  /* VARIABLES */
  // Initialize struct
  struct shared_data data;
  // Initialize the counter for signal sending
  int counter = 0;
  // Logfile for logfile_memory.txt
  FILE *fp;

  /*CREATING SHARED MEMORY FOR OTHER PROCESSES*/
  // Initialize a semaphore
  sem_t *sem_id = sem_open(SEM_PATH, O_CREAT, S_IRUSR | S_IWUSR, 0);
  if (sem_id == SEM_FAILED) {  // Check for errors
    perror("sem_open failed");
    return -1;
  }
  // Initialize the semaphore to 0 until shared memory is initated
  sem_init(sem_id, 1, 0);

  // Create a shared memory object
  int shmfd = shm_open(SHM_PATH, O_CREAT | O_RDWR, S_IRWXU | S_IRWXG);
  if (shmfd < 0) {  // Check for errors
    perror("Opening shared memory failed");
    return -1;
  }
  // Get the size of the struct of shared_data
  int shm_size = sizeof(struct shared_data);
  ftruncate(shmfd, shm_size);  // Truncate size of shared memory

  // Map pointer to shared memory
  struct shared_data *shm_ptr =
      mmap(NULL, shm_size, PROT_READ | PROT_WRITE, MAP_SHARED, shmfd, 0);
  if (shm_ptr == MAP_FAILED) {  // Check for errors
    perror("mmap failed");
    return -1;
  }
  // Post the semaphore
  sem_post(sem_id);

  // Open the logfile to either create or delete the content
  fp = fopen("../Log/logfile_memory.txt", "w");
  if (fp == NULL) {  // Check for errors
    perror("Error opening logfile");
    return -1;
  }
  // Close the file
  fclose(fp);

  /* WATCHDOG */
  pid_t Watchdog_pid;               // For watchdog PID
  pid_t blackboard_pid = getpid();  // Recieve the process PID

  // Get the file locations
  char *fnames[NUM_PROCESSES] = PID_FILE_SP;

  // Open the drone process tmp file to write its PID
  FILE *pid_fp = fopen(fnames[BLACKBOARD_SYM], "w");
  if (pid_fp == NULL) {  // Check for errors
    perror("Error opening Blackboard tmp file");
    return -1;
  }
  fprintf(pid_fp, "%d", blackboard_pid);
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
  if (fscanf(watchdog_fp, "%d", &Watchdog_pid) != 1) {
    printf("Error reading Watchdog PID from file.\n");
    fclose(watchdog_fp);
    return -1;
  }

  // Close the file
  fclose(watchdog_fp);

  // Start the loop
  while (1) {
    // Recieve all the values from the shared memory
    sem_wait(sem_id);                  // Wait for semaphore
    memcpy(&data, shm_ptr, shm_size);  // Copy memory to local cells
    sem_post(sem_id);                  // Post semaphore as soon as done

    // Check if esc button has been pressed
    if (data.ch == ESCAPE) {
      // Log the exit with the last data recieved
      log_message(fp, "Exiting the program at states ", &data);
      sleep(1);  // Wait for 1 second
      break;     // Leave the loop
    }

    // Update the recieved data from shared memory to the logfile
    log_message(fp, "Drone moved to ", &data);

    // Send a signal after certain iterations have passed
    if (counter == PROCESS_SIGNAL_INTERVAL) {
      // Send signal to watchdog every process_signal_interval
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

  // Clean up
  shm_unlink(SHM_PATH);
  sem_close(sem_id);
  sem_unlink(SEM_PATH);
  munmap(shm_ptr, shm_size);

  // End the blackboard program
  return 0;
}
