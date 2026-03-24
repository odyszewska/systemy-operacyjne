# Operating Systems – Laboratory Assignments

This repository contains my solutions to 12 laboratory assignments completed for the **Operating Systems** course.

The projects were implemented in **C** and cover core operating-systems topics such as process management, signals, inter-process communication, multithreading, synchronization, and socket programming.

## Topics covered

- **Makefiles** and C development environment
- **Static, shared, and dynamically loaded libraries**
- **File processing**
- **Process creation and control**
- **Signals**
- **Unnamed and named pipes**
- **Message queues**
- **Shared memory and semaphores**
- **POSIX threads**
- **Thread synchronization**
- **Socket-based client-server communication**
- **Stream and datagram protocols**

## Repository structure

Each laboratory assignment is placed in a separate directory:

- `DyszewskaOliwia-cw01`
- `DyszewskaOliwia-cw02`
- `DyszewskaOliwia-cw03`
- `DyszewskaOliwia-cw04`
- `DyszewskaOliwia-cw05`
- `DyszewskaOliwia-cw06`
- `DyszewskaOliwia-cw07`
- `DyszewskaOliwia-cw08`
- `DyszewskaOliwia-cw09`
- `DyszewskaOliwia-cw10`
- `DyszewskaOliwia-cw11`
- `DyszewskaOliwia-cw12`

Most directories include source files and a `Makefile` for building the assignment.

## Laboratory overview

### Lab 1 – Build environment and debugging
Created a simple countdown program in C, prepared a `Makefile`, and practiced debugging with `gdb`/`lldb` and an IDE.

### Lab 2 – Libraries in Unix
Implemented a library related to the **Collatz conjecture** and used it in three ways:
- static linking
- shared linking
- dynamic loading at runtime

### Lab 3 – File processing
Implemented `flipper`, a program that processes text files from a source directory and reverses characters in each line before saving the results to an output directory.

### Lab 4 – Processes
Worked with `fork()`, `wait()`, and `execl()` to create child processes, inspect parent-child relationships, and demonstrate process replacement.

### Lab 5 – Signals
Implemented examples of signal handling with `SIGUSR1`, including ignoring, handling, and masking signals, as well as communication between two processes using signals.

### Lab 6 – Pipes and numerical integration
Computed a numerical approximation of an integral using multiple child processes and pipes, including both unnamed and named pipe variants.

### Lab 7 – Message queues
Built a simple client-server chat using message queues for inter-process communication.

### Lab 8 – Shared memory and semaphores
Implemented a print-system simulation with multiple users and printers synchronized through shared memory and semaphores.

### Lab 9 – Threads
Reimplemented numerical integration using **POSIX threads** instead of processes.

### Lab 10 – Synchronization problem
Solved a synchronization scenario involving a doctor, patients, and pharmacists using **mutexes** and **condition variables**.

### Lab 11 – Stream socket chat
Implemented a client-server chat using **stream sockets**, supporting commands such as `LIST`, `2ALL`, `2ONE`, and `STOP`.

### Lab 12 – Datagram socket chat
Modified the previous chat application to use a **datagram protocol** instead of a stream protocol.

---

## Build and run

Most tasks can be built using the included `Makefile`.

Example:

```bash
cd DyszewskaOliwia-cw01
make
./countdown
```
For other labs, use the appropriate targets defined in the local Makefile
