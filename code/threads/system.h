// system.h 
//	All global variables used in Nachos are defined here.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef SYSTEM_H
#define SYSTEM_H

#include "copyright.h"
#include "utility.h"
#include "thread.h"
#include "scheduler.h"
#include "interrupt.h"
#include "stats.h"
#include "timer.h"
#include "../userprog/bitmap.h"
#include "../userprog/addrspace.h"
#include "synch.h"


#define SWAP_SIZE 50000
// Initialization and cleanup routines
extern void Initialize(int argc, char **argv); 	// Initialization,
						// called before anything else
extern void Cleanup();				// Cleanup, called when
						// Nachos is done.
#ifdef VM
extern bool isFIFO;
#endif

class IPTEntry : public TranslationEntry 
{
	 public:
		AddrSpace* owner;
};

class IPT
{
	public:
		IPT();
		~IPT();
		IPTEntry* ipTable;

};



extern Thread *currentThread;			// the thread holding the CPU
extern Thread *threadToBeDestroyed;  		// the thread that just finished
extern Scheduler *scheduler;			// the ready list
extern Interrupt *interrupt;			// interrupt status
extern Statistics *stats;			// performance metrics
extern Timer *timer;				// the hardware alarm clock
extern BitMap *mmBitMap;			//the pagetable bitmap
extern IPT *mIPT;
extern OpenFile *swapFile;
extern BitMap *swapMap;
extern Lock *memoryLock;
extern Lock *iptLock;
extern Lock *swapLock;

#ifdef USER_PROGRAM
#include "machine.h"
extern Machine* machine;	// user program memory and registers
#endif

#ifdef FILESYS_NEEDED 		// FILESYS or FILESYS_STUB 
#include "filesys.h"
extern FileSystem  *fileSystem;
#endif

#ifdef FILESYS
#include "synchdisk.h"
extern SynchDisk   *synchDisk;
#endif

#ifdef NETWORK
#include "post.h"
extern PostOffice* postOffice;
extern int net_name;
extern int numberOfServers;
#endif



#endif // SYSTEM_H
