/* syscalls.h 
 * 	Nachos system call interface.  These are Nachos kernel operations
 * 	that can be invoked from user programs, by trapping to the kernel
 *	via the "syscall" instruction.
 *
 *	This file is included by user programs and by the Nachos kernel. 
 *
 * Copyright (c) 1992-1993 The Regents of the University of California.
 * All rights reserved.  See copyright.h for copyright notice and limitation 
 * of liability and disclaimer of warranty provisions.
 */

#ifndef SYSCALLS_H
#define SYSCALLS_H

#include "copyright.h"

/* system call codes -- used by the stubs to tell the kernel which system call
 * is being asked for
 */
#define SC_Halt		0
#define SC_Exit		1
#define SC_Exec		2
#define SC_Join		3
#define SC_Create	4
#define SC_Open		5
#define SC_Read		6
#define SC_Write	7
#define SC_Close	8
#define SC_Fork		9
#define SC_Yield	10
#define SC_CreateLock 11
#define SC_DestroyLock 12
#define SC_Acquire 13
#define SC_Release 14
#define SC_CreateCondition 15
#define SC_DestroyCondition 16
#define SC_Signal 17
#define SC_Wait 18
#define SC_Broadcast 19
#define SC_Rand 20
#define SC_IntPrint 21
#define SC_CreateMonitor 22
#define SC_DestroyMonitor 23
#define SC_GetMonitorVal 24
#define SC_SetMonitorVal 25
#define SC_JobRequest 26
#define SC_SetClerkState 27
#define SC_GetClerkState 28
#define SC_IncrementLineCount 29

#define MAXFILENAME 256

#ifndef IN_ASM

/* The system call interface.  These are the operations the Nachos
 * kernel needs to support, to be able to run user programs.
 *
 * Each of these is invoked by a user program by simply calling the 
 * procedure; an assembly language stub stuffs the system call code
 * into a register, and traps to the kernel.  The kernel procedures
 * are then invoked in the Nachos kernel, after appropriate error checking, 
 * from the system call entry point in exception.cc.
 */

/* Stop Nachos, and print out performance stats */
void Halt();		
 

/* Address space control operations: Exit, Exec, and Join */

/* This user program is done (status = 0 means exited normally). */
void Exit(int status);	

/* A unique identifier for an executing user program (address space) */
typedef int SpaceId;	
 
/* Run the executable, stored in the Nachos file "name", and return the 
 * address space identifier
 */
SpaceId Exec(char *name, int len);
 
/* Only return once the the user program "id" has finished.  
 * Return the exit status.
 */
int Join(SpaceId id); 	
 

/* File system operations: Create, Open, Read, Write, Close
 * These functions are patterned after UNIX -- files represent
 * both files *and* hardware I/O devices.
 *
 * If this assignment is done before doing the file system assignment,
 * note that the Nachos file system has a stub implementation, which
 * will work for the purposes of testing out these routines.
 */
 
/* A unique identifier for an open Nachos file. */
typedef int OpenFileId;	

/* when an address space starts up, it has two open files, representing 
 * keyboard input and display output (in UNIX terms, stdin and stdout).
 * Read and Write can be used directly on these, without first opening
 * the console device.
 */

#define ConsoleInput	0  
#define ConsoleOutput	1  
 
/* Create a Nachos file, with "name" */
void Create(char *name, int size);

/* Open the Nachos file "name", and return an "OpenFileId" that can 
 * be used to read and write to the file.
 */
OpenFileId Open(char *name, int size);

/* Write "size" bytes from "buffer" to the open file. */
void Write(char *buffer, int size, OpenFileId id);

/* Read "size" bytes from the open file into "buffer".  
 * Return the number of bytes actually read -- if the open file isn't
 * long enough, or if it is an I/O device, and there aren't enough 
 * characters to read, return whatever is available (for I/O devices, 
 * you should always wait until you can return at least one character).
 */
int Read(char *buffer, int size, OpenFileId id);

/* Close the file, we're done reading and writing to it. */
void Close(OpenFileId id);



/* User-level thread operations: Fork and Yield.  To allow multiple
 * threads to run within a user program. 
 */

/* Fork a thread to run a procedure ("func") in the *same* address space 
 * as the current thread.
 */
void Fork(void (*func)());

/* Yield the CPU to another runnable thread, whether in this address space 
 * or not. 
 */
void Yield();	

/* Create a lock for use within a user program. Returns the id of the lock and takes
 * in what to name the lock as a parameter.
 */
int CreateLock();

/* Destroy a lock. The id parameter is to find the proper lock to delete.
 */
void DestroyLock(int id);

/* Acquire a lock. The id parameter finds the proper lock and acquires it. 
 * waits if not available.
 */
void Acquire(int id);

/* Relase a lock. The id parameter finds the proper lock and releases it.
 * If a thread is on the wait queue, it is put to ready.
 */
void Release(int id);

/* Create a Condition for use within a user program. Returns the id of the conditional and takes
 * in what to name the conditional as a parameter.
 */
int CreateCondition();

/* Destroy a Conditional. The id parameter is to find the proper conditional to delete.
*/
void DestroyCondition(int id);

/* Signal a thread to wake up. The id parameter finds the proper conditional to use.
 * Signal wakes up the necessary thread on the wait queue.
 */
void Signal(int id, int lock_id);

/* Waits on a lock within the conditional. The id parameter finds the proper conditional.
 * The thread puts itself on the necessary wait queue.
 */
void Wait(int id, int lock_id);

/* Broadcasts on a lock. The id parameter finds the proper conditional.
 * All threads on the wait queue are signaled
 */
void Broadcast(int id, int lock_id);

/*Get a random integer number from the kernel*/
int Rand(int mod);

/*Print an int using printf*/
void IntPrint(int i);

/* Creates a monitor and returns the index (id) of it
*/
int CreateMonitor();

/* Destroys a monitor at index id
*/
void DestroyMonitor(int id);

/*Returns the value of a monitor specified at index id*/
int GetMonitorVal(int type, int num, char* vars);

/*Set the monitor id's value to val*/
void SetMonitorVal(int type, int num, char* vars);

int JobRequest();

void SetClerkState(int type, int state, int line);

int GetClerkState(int type, int state);

int IncrementLineCount(int type, int line, int diff, int bribe);

#endif /* IN_ASM */

#endif /* SYSCALL_H */
