# Assignment details
Deadline: December 3, 2023, 12:59 pm.

After this date you can upload your work, but a penalty will be applied in the evaluation mark. 

The penalty is a percentage that monotonically increases from 0% up to 50% at the date of the next assignment's deadline.

 Follow these rules punctually and literally: no mercy for those who violate them 😈

All group member(s) upload only one archive file
No executables allowed
Update the groups list in the shared file so that the assignment can be put into relation with its authors.
Your work shall include (be both synthetic and complete):

sketch of the architecture
short definitions of all active components : what they do, which primitives used, which algorithms)
list of components, directories, files
makefile
instructions for installing and running
operational instructions
whatever else you think useful
What you upload cannot be substituted for no reason.

After submission, public your work on GITHUB.

# Assignment 1
This assignment represents the first part of the project for Advanced and Robot Programming course on UniGe in the winter semester 2023/2024. 

The work has been performed by a team of two: Iris Laanearu and Tomoha Neki

## Installation & Usage
For project's configuration we have used `Makefile`.

To build executables simply hit:
```
make
```
in the project directory.

To run the game hit:
```
make run
```
when this happens a 5 windows with konsole processes will launch, each one for different segment.

To remove the executables simply hit 
```
make clean
```


### Operational instructions, controls
To operate the drone use following keybindings
    `w` `e` `r`       
    `s` `d` `f`   
    `x` `c` `v`       



## Overview 

![Sketch of the architecture](architecture.jpg)

The first part assumes first 6 components:
- Master
- Server (blackboard using Posix Shared Memory)
- Window (User interface)
- Watchdog
- Drone
- Keyboard 

All of above mentioned components were created.

General overview of first assignment, tasks, what was accomplished

short definitions of all active components : what they do, which primitives used, which algorithms)

### Master
Master process is the father of all processes. It creates child processes by using `fork()` and runs them inside a wrapper program `Konsole` to display the current status, debugging messages until an additional thread/process for colleceting log messages.

After creating children, process stays in a infinte while loop awaiting termination of all processes, and when that happens - terminate itself.

### Server(Blackboard)
Blackboard communicates the other processes through shared memory and logs the information it receives.
It creates all segments of the shared memory and the semaphore.
And for watchdog communication, it writes its own PID to file for watchdog to reand and read a watchdog's PID from logfile.
In infinite loop, it reads all the data from the shared memory and updates the contents to the logfile.And also, it periodically sends a signal to the watchdog after certain number of iterations.
Upon existing the loop, it cleas up the segments of the shared memory and semaphore.

### Watchdog
Watchdog's job is to monitor the acitivities of all of the processes, which means at this point if processes are running and not closed.

During initialization it writes its own PID to a file for the other processes to read, and read PIDs of other processes from files.
It receives 'SIGUSR1' from other child processes,indicating that child processes inform watchdog about their acitivities.
In infinite loop, it monitors the elapsed time since the last data reception from each child process exceeds a timeout.
If timeout occurs, watchdog terminates all child prosesses.

### Window
Window process creates game window using ncurses.
The window displays a drone represented by the character "X", which moves within window based on user key input.(At first, the drone is printed in the middle of the main game window.)
It accesses the shared memory to get the data from the drone's position culculated by drone process and user key input.


### Drone

### Keyboard 


### Additional Comments
#### Constants.h ####
All the necessary constants and structures are defined here.



### Further Improvements
- [] Add new functionalities to window:
    - [] Freeze window
    - [] menu window
    - [] maybe something else
- [] Logging messages, Monitoring process
- [] Add special keycaps handling (eg. esc)
    - [] list them here

- [] next components
dont put pipes tho





