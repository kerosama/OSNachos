// addrspace.h 
//	Data structures to keep track of executing user programs 
//	(address spaces).
//
//	For now, we don't keep any information about address spaces.
//	The user level CPU state is saved and restored in the thread
//	executing the user program (see thread.h).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#ifndef ADDRSPACE_H
#define ADDRSPACE_H

#include "copyright.h"
#include "filesys.h"
#include "table.h"

#define UserStackSize		1024 	// increase this as necessary!

#define MaxOpenFiles 256
#define MaxChildSpaces 256

struct DiskLocation
{
	bool swap; //if true, page is in swapfile and no longer in executable
	int position; //the position of the start of the page in the executable or swapfile
};

class PTE : public TranslationEntry
{
	public:
		int byteOffset;
		DiskLocation diskLocation;
};

class AddrSpace {
  public:
    AddrSpace(OpenFile *executable);	// Create an address space,
					// initializing it with the program
					// stored in the file "executable"
    ~AddrSpace();			// De-allocate an address space

    void InitRegisters();		// Initialize user-level CPU registers,
					// before jumping to user code

    void SaveState();			// Save/restore address space-specific
    void RestoreState();		// info on a context switch
    Table fileTable;			// Table of openfiles

	void AddPages();

	PTE *PageTable;	// Assume linear page table translation
					// for now!
	
	OpenFile *spaceExec; //store constructor executable as member variable

	 unsigned int numPages;		// Number of pages in the virtual 
 private:
   
   
					// address space
	Lock* PTLock;

};

#endif // ADDRSPACE_H
