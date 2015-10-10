// synch.cc 
//	Routines for synchronizing threads.  Three kinds of
//	synchronization routines are defined here: semaphores, locks 
//   	and condition variables (the implementation of the last two
//	are left to the reader).
//
// Any implementation of a synchronization routine needs some
// primitive atomic operation.  We assume Nachos is running on
// a uniprocessor, and thus atomicity can be provided by
// turning off interrupts.  While interrupts are disabled, no
// context switch can occur, and thus the current thread is guaranteed
// to hold the CPU throughout, until interrupts are reenabled.
//
// Because some of these routines might be called with interrupts
// already disabled (Semaphore::V for one), instead of turning
// on interrupts at the end of the atomic operation, we always simply
// re-set the interrupt state back to its original value (whether
// that be disabled or enabled).
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "synch.h"
#include "system.h"
#include <iostream>

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	Initialize a semaphore, so that it can be used for synchronization.
//
//	"debugName" is an arbitrary name, useful for debugging.
//	"initialValue" is the initial value of the semaphore.
//----------------------------------------------------------------------

Semaphore::Semaphore(char* debugName, int initialValue)
{
    name = debugName;
    value = initialValue;
    queue = new List;
}

//----------------------------------------------------------------------
// Semaphore::Semaphore
// 	De-allocate semaphore, when no longer needed.  Assume no one
//	is still waiting on the semaphore!
//----------------------------------------------------------------------

Semaphore::~Semaphore()
{
    delete queue;
}

//----------------------------------------------------------------------
// Semaphore::P
// 	Wait until semaphore value > 0, then decrement.  Checking the
//	value and decrementing must be done atomically, so we
//	need to disable interrupts before checking the value.
//
//	Note that Thread::Sleep assumes that interrupts are disabled
//	when it is called.
//----------------------------------------------------------------------

void
Semaphore::P()
{
    IntStatus oldLevel = interrupt->SetLevel(IntOff);	// disable interrupts
    
    while (value == 0) { 			// semaphore not available
	queue->Append((void *)currentThread);	// so go to sleep
	currentThread->Sleep();
    } 
    value--; 					// semaphore available, 
						// consume its value
    
    (void) interrupt->SetLevel(oldLevel);	// re-enable interrupts
}

//----------------------------------------------------------------------
// Semaphore::V
// 	Increment semaphore value, waking up a waiter if necessary.
//	As with P(), this operation must be atomic, so we need to disable
//	interrupts.  Scheduler::ReadyToRun() assumes that threads
//	are disabled when it is called.
//----------------------------------------------------------------------

void
Semaphore::V()
{
    Thread *thread;
    IntStatus oldLevel = interrupt->SetLevel(IntOff);

    thread = (Thread *)queue->Remove();
    if (thread != NULL)	   // make thread ready, consuming the V immediately
	scheduler->ReadyToRun(thread);
    value++;
    (void) interrupt->SetLevel(oldLevel);
}

// Dummy functions -- so we can compile our later assignments 
// Note -- without a correct implementation of Condition::Wait(), 
// the test case in the network assignment won't work!
Lock::Lock(char* debugName) 
{
	name = debugName;
	acquired = false;
	queue = new List;
}
Lock::~Lock() 
{
	delete queue;
}

void Lock::Acquire(char* debugName) 
{

	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	

	if(isHeldByCurrentThread())
	{	
		(void) interrupt->SetLevel(oldLevel);
		
		return;

	}
	else if(acquired)
	{			
		std::cout << debugName << " waiting to acquire lock " << name << std::endl;
		queue->Append((void*)currentThread);
		currentThread->Sleep();	

	}
	else
	{
			
		std::cout <<  debugName <<  " acquired lock " << name << std::endl;
		owner = currentThread;
		acquired = true;
		
	}
	
	(void) interrupt->SetLevel(oldLevel);
	

}

void Lock::Release(char* debugName) 
{
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	
	if(isHeldByCurrentThread())
	{
		acquired = false;
		owner = NULL;
		std::cout << debugName << " released lock " << name << std::endl;
	}
	else
	{
		std::cout <<  debugName <<  ": Lock " << name << " is not held by current thread!" << std::endl;
	}

	if(!queue->IsEmpty())
	{
		Thread* thread = (Thread *)queue->Remove();
		scheduler->ReadyToRun(thread);
	}	
	
	(void) interrupt->SetLevel(oldLevel);
}

bool Lock::isHeldByCurrentThread()
{
	if(currentThread == owner)
	{
		return true;
	}
	return false;
}


Condition::Condition(char* debugName) 
{ 
	name = debugName;
	waitQueue = new List;
}
Condition::~Condition() 
{ 
	delete waitQueue;
}

void Condition::Wait(char* debugName, Lock* conditionLock) 
{ 
	std::cout <<  debugName <<  "Waiting on condition " << name << std::endl;
	//ASSERT(FALSE); 
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(conditionLock == NULL)
	{
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	if(conditionLock != waitingLock)
	{
		if(waitingLock == NULL)
		{
			waitingLock = conditionLock;
		}
		else
		{
			(void) interrupt->SetLevel(oldLevel);
			return;
		}
	}

	conditionLock->Release("");
	waitQueue->Append((void*)currentThread);
	currentThread->Sleep();
	conditionLock->Acquire("");
	(void) interrupt->SetLevel(oldLevel);
	return;
}

void Condition::Signal(char* debugName, Lock* conditionLock) 
{ 
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	if(waitQueue->IsEmpty())
	{		
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	if(waitingLock != conditionLock)
	{
		std::cout << "waitinglock and parameter lock do not match" << std::endl;
		(void) interrupt->SetLevel(oldLevel);
		return;
	}

	std::cout <<  debugName << "has signaled lock " << conditionLock->getName() << " with condition " << name << std::endl;
	Thread* thread = (Thread *)waitQueue->Remove();
	scheduler->ReadyToRun(thread);

	(void) interrupt->SetLevel(oldLevel);
	return;
}
void Condition::Broadcast(Lock* conditionLock) 
{ 
	IntStatus oldLevel = interrupt->SetLevel(IntOff);
	while(!waitQueue->IsEmpty())
	{
		Thread* thread = (Thread *)waitQueue->Remove();
		scheduler->ReadyToRun(thread);
	}
	(void) interrupt->SetLevel(oldLevel);

}
