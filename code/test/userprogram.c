/*// threadtest.cc 
//  Simple test case for the threads assignment.
//
//  Create two threads, and have them context switch
//  back and forth between themselves by calling Thread::Yield, 
//  to illustratethe inner workings of the thread system.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.


/*
  Need to do:

    - need to add run() for clerks 
    - create interactions between client - application clerk first 
  
  How to share/communicate data between customer/clerk? 
   - customers should be created with index as a member variable // need to add
   - create a global vector of the class  //need to add
    - when customer acquires lock from clerk, customer passes clerk their index with a function
      - store index in a vector // should be same size as lineCount
    

*/





#include "copyright.h"
#include "syscall.h"

#define true 1
#define false 0

#define  numCustomers 50
#define  numApplicationClerks 5
#define  numPictureClerks 5
#define  numPassportClerks 5
#define  numCashiers 5
#define  numManagers 1 
#define  NumSenators 10

/*NUMBER OF EACH THREAD CREATED*/
int numCustomerThreads;
int numApplicationClerkThreads;
int numPictureClerkThreads;
int numPassportClerkThreads;
int numCashierThreads;
int numManagerThreads;
int numSenatorThreads;

/*CLERK LOCKS*/
int applicationLock;
int pictureLock;
int passportLock;
int cashierLock;

/*LINE LOCKS*/
int applicationLineLocks[numApplicationClerks];
int pictureLineLineLocks[numPictureClerks];
int passportLineLineLocks[numPassportClerks];
int cashierLineLineLocks[numCashiers];

/*BRIBE LINE LOCKS*/
int applicationBribeLineLocks[numApplicationClerks];
int pictureBribeLineLocks[numPictureClerks];
int passportBribeLineLocks[numPassportClerks];
int cashierBribeLineLocks[numCashiers];

/*LINE CV'S*/
int applicationLineCVs[numApplicationClerks];
int pictureLineCVs[numPictureClerks];
int passportLineCVs[numPassportClerks];
int cashierLineCVs[numCashiers];

/*BRIBE LINE CV'S*/
int applicationBribeLineCVs[numApplicationClerks];
int pictureBribeLineCVs[numPictureClerks];
int passporBribetLineCVs[numPassportClerks];
int cashierBribeLineCVs[numCashiers];

/*LINE COUNTS*/
int applicationLineCounts[numApplicationClerks];
int pictureLineCounts[numPictureClerks];
int passportLineCounts[numPassportClerks];
int cashierLineCounts[numCashiers];

/*BRIBE LINE COUNTS*/
int applicationBribeLineCounts[numApplicationClerks];
int pictureBribeLineCounts[numPictureClerks];
int passportBribeLineCounts[numPassportClerks];
int cashierBribeLineCounts[numCashiers];

struct Customer
{
	int ssn;
	int money;
	int bribed;
	int applicationAccepted;
	int pictureTaken;
	int certified;	
	int done;
};

struct Customer customers[numCustomers];

/*HOLD CUSTOMER SSNs INFO*/
struct customerSSNs
{
  int line;
  int ssn;
};

/*HOLD BRIBE CUSTOMER SSNs INFO*/
struct bribeCustomerSSNs
{
  int line;
  int ssn;
};

struct ApplicationClerk
{
	int state; /* 0: available     1: busy       2: on break*/
	int lineCount;
	int bribeLineCount;
	int money;
	int line;
};

struct ApplicationClerk applicationClerks[numApplicationClerks];

int findSmallestApplicationLine()
{
	int i;
	int smallest = 50;
	int smallestIndex = -1;
	for(i = 0; i < numApplicationClerks; i++)
	{
		if(applicationLineCounts[i] < smallest)
		{
			smallest = applicationLineCounts[i];
			smallestIndex = i;
		}
	}
	return smallestIndex;
}

void joinApplicationLine(int ssn)
{
	int myLine;

	Acquire(applicationLock);
	myLine = findSmallestApplicationLine();

	if(applicationClerks[myLine].state == 1)
	{
		if(customers[ssn].bribed == true)
		{
			applicationBribeLineCounts[myLine]++;
			
		}
		else
		{
			applicationLineCounts[myLine]++;
		}

		Wait(applicationLineCVs[myLine], applicationLock);
	}

	applicationClerks[myLine].state = 1;
	Release(applicationLock);

	Acquire(applicationLineLocks[myLine]);
	
	Signal(applicationLineCVs[myLine], applicationLineLocks[myLine]);

	Wait(applicationLineCVs[myLine], applicationLineLocks[myLine]);

	customers[ssn].applicationAccepted = true;

	if(customers[ssn].bribed == true)
	{
		applicationBribeLineCounts[myLine]--;
	}
	else
	{
		applicationLineCounts[myLine]--;
	}

	Signal(applicationLineCVs[myLine], applicationLineLocks[myLine]);
	Release(applicationLineLocks[myLine]);
}

void runCustomer(int ssn)
{
	while(customers[ssn].done == false);
	{
		int randomLine = 0; /*generate random number here*/
		switch(randomLine)
		{
			case 0:
				if(customers[ssn].applicationAccepted == false)
				{
					joinApplicationLine(ssn);
				}
			break;
			case 1:
				if(customers[ssn].pictureTaken == false)
				{
				}
			break;
			case 2:
				if(customers[ssn].certified == false)
				{
				}
			break;
			case 3:
			break;
		}


	}
}

void createCustomer()
{
	numCustomerThreads++;
	runCustomer(numCustomerThreads);
}



void runApplicationClerk(int line)
{
	while(true)
	{
		Acquire(applicationLock);

		if(applicationBribeLineCounts[line] > 0)
		{
			
			Signal(applicationBribeLineCVs[line], applicationLock);
			applicationClerks[line].state = 1;
		}
		else if(applicationLineCounts[line] > 0)
		{
			Signal(applicationLineCVs[line], applicationLock);
			applicationClerks[line].state = 1;
		}
		else
		{
			applicationClerks[line].state = 2; /*on break*/
		}

		Acquire(applicationLineLocks[line]);
		Release(applicationLock);

		Wait(applicationLineCVs[line], applicationLineLocks[line]);

		Signal(applicationLineCVs[line], applicationLineLocks[line]);

		Wait(applicationLineCVs[line], applicationLineLocks[line]);

		Release(applicationLineLocks[line]);

	}
}

void createApplicationClerk()
{
	numApplicationClerkThreads++;
	runApplicationClerk(numApplicationClerkThreads++);
}




int main()
{
	int i = 0;

	/*initialize number of created threads*/
	numCustomerThreads = -1;
	numApplicationClerkThreads = -1;

	/*Initialize Customers here*/
	for(i = 0; i < numCustomers; i++)
	{
		customers[i].money = 500 /*DO RANDOM HERE*/;
		customers[i].ssn = i; /*ssn is id*/
		customers[i].applicationAccepted = false;
		customers[i].pictureTaken = false;
		customers[i].certified = false;
		customers[i].bribed = false;

		Fork((void(*)())createCustomer);
	}

	/*Initialize Application Clerks here*/
	for(i = 0; i < numApplicationClerks; i++)
	{
		applicationClerks[i].state = 0;
		applicationClerks[i].lineCount;
		applicationClerks[i].bribeLineCount;
		applicationClerks[i].money;
		applicationClerks[i].line = i;

		Fork((void(*)())createApplicationClerk);
	}

	
	
	
  

}
