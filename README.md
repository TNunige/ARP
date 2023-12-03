# Assignment description
The project is an interactive simulator for a drone with two degrees of freedom.
# Assignment 1
This assignment represents the first part of the project for Advanced and Robot Programming course on UniGe in the winter semester 2023/2024. 

The work has been performed by a team of two: Iris Laanearu and Tomoha Neki

## Installation & Usage
For the project's configuration, we have used `Makefile`.

To build executables, run this :
```
make
```
in the project directory.

Then move to `Build`:
```
cd /Build
```
To start the game, run this:
```
./master
```

To remove the executables and logs, run this:
```
make clean
```

```
make clean logs
```
### Additional tools ###
Currently, programmers have implemented a way to control the size and positions of the console windows. This is possible using xdotool and wmctrl. 
Download the tools in your command window like this: 

```
sudo apt-get update
```
 
```
sudo apt-get install xdotool wmctrl
```
 
To use the positioning and sizing of this drone game certain lines in the master.c code have to commented and uncommented.
Comment and uncomment certain lines for these tools to work. Here is a relevant part in the master.c code.
<img width="466" alt="image" src="https://github.com/TNunige/ARP/assets/145358917/a6bbf306-94e9-4b7a-830d-713408cf3c1c">


 
Comment out `arg_list3[]` and `arg_list5` argument lists that are passed as a command to the terminal. Uncomment all currently commented lines.
Then it is also possible to manually change the dimensions of the console windows to match the computer screen of the user. This can be done here:
As for the programmers, there is a possibility of doing the sizing for different computer screens and then it should work better because the sizing depends on the screen size of the computer used for the game. This requires extra work and is not implemented for this course.




###  Operational instructions, controls ###
To operate the drone use the following keybindings
- `w`: move left up
- `s`: move left
- `x`: move left down
- `e`: move up
- `c`: move down
- `r`: move right up
- `f`: move right
- `v`: move right down
- `d`: brake
- `k`: restart
- `esc` : exit



## Overview 

![Architecture scheme](https://github.com/TNunige/ARP/assets/145358917/d91aa4d7-c7de-46dd-9d3c-9e5030673532)

The first part includes  6 processes:
- Master
- Blackboard
- Window (User interface)
- Watchdog
- Drone
- Keyboard (User interface)

All of the above mentioned components were created.

### Master
Master process is the father of all processes. It creates child processes by using `fork()`. It runs keyboard and window inside a wrapper program `Konsole`.

After creating children, the process waits termination of all processes and prints the termination status.

### Server(Blackboard)
Blackboard communicates the other processes through shared memory and logs the information it receives.
It creates all segments of the shared memory and the semaphore.
For watchdog communication, the process writes its  PID to a temporary file for the watchdog to read. Then, it reads a watchdog's PID from a temporary watchdog file. 
The process checks the data exchanged in the shared memory and updates the contents to a logfile. Additionally, it periodically sends a signal to the watchdog after a certain number of iterations.
Upon exiting the loop, it clears up the segments of the shared memory and semaphore and terminates.

### Watchdog
The watchdog process's job is to monitor the activities of all the other processes (except the master). Watchdog monitors window, drone, keyboard, and blackboard processes.
At the beginning of the process, it writes its PID to a temporary file for the other processes to read its PID and reads the PIDs of other monitored child processes from temporary files.
We have chosen to implement a watchdog that only receives signals from monitored processes. It receives signal `SIGUSR1` from other child processes, indicating that child processes are active during the monitored period.
In the infinite loop, it monitors the elapsed time since the last data reception from each child process. When the elapsed time exceeds the timeout, the watchdog sends `SIGKILL` signals to all monitored processes and terminates all child processes.

### Window
The window process creates the game window and the drone character using ncurses. The game window features a drone represented by the character "X", which moves based on user key input received from the keyboard process using a FIFO as a communication channel. At first, the drone is printed in the middle of the game window.
It accesses the shared memory to retrieve the data on the drone's updated position calculated by the Drone process based on the user key input.
Subsequently, it updates the drone's position and prints the character "X" on the window.
Also, it periodically sends a signal to the watchdog process to inform its activity.

### Drone
The drone process models the drone character movement by dynamically calculating the force impacting the drone based on the user key input (direction), command, and repulsive force. The repulsive force is activated when the drone is near the game window borders.
It calculates the forces based on the received user key from the keyboard process and writes it in a FIFO (named pipe). It utilizes the following dynamic motion equation:[equation] to determine the new position of the drone taking into account the sum of input forces and repulsive forces.
Then, the Drone process updates shared memory with the drone's new position and the user key input. Additionally, it periodically sends a signal to the watchdog process to inform its activity.

### Keyboard 
The keyboard handles user key inputs and displays messages on the inspection window.
It scans user key inputs by using `getch()` command and sends the values of the pressed key to the drone process through a FIFO (named pipe).
Also, it periodically sends a signal to the watchdog process to inform its activity.

### Constants.h ###
All the necessary constants and structures are defined here.

### Improvements ###
This assignment 1 submission is not a finalized version of the drone simulator and therefore it has improvements to be worked on for the next assignments.

-	The ncurses interface is not working properly every time you run the game. Sometimes the window box lines or other components of the interface bug. Running it more times helps to correct them but doesn’t guarantee the display intended by the programmers. Especially when using other tools like xdotool and wmctrl to control the size and position of the konsole window on the computer screen.
- Currently the watchdog does not check for the escape key but it could be an improvement to exit the game sooner when user has initiated it.
- The watchdog does not terminate the console of the window and keyboard processes and only the process itself. This could be improved for the next assignment for watchdog to also receive the PIDs of the consoles and send a SIGKILL when a process has timed out. There is no need to actively check for the signals from the console PIDs. 

   








