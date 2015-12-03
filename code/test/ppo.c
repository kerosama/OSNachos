#include "syscall.h"

#define true 1
#define false 0

#define  numCustomers 1
#define  numApplicationClerks 1

/*NUMBER OF EACH THREAD CREATED*/
int numCustomerThreads;
int numApplicationClerkThreads;

int numLocks;

/*CLERK LOCKS*/
int applicationLock;

/*LINE LOCKS*/
int applicationLineLocks[numApplicationClerks];

/*BRIBE LINE LOCKS*/
int applicationBribeLineLocks[numApplicationClerks];

/*LINE CV'S*/
int applicationLineCVs[numApplicationClerks];

/*BRIBE LINE CV'S*/
int applicationBribeLineCVs[numApplicationClerks];

/*LINE COUNTS*/
int applicationLineCounts[numApplicationClerks];

/*BRIBE LINE COUNTS*/
int applicationBribeLineCounts[numApplicationClerks];

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

struct ApplicationClerk
{
	int state; /* 0: available     1: busy       2: on break*/
	int lineCount;
	int bribeLineCount;
	int money;
	int line;
};

struct Customer customers[numCustomers];
struct ApplicationClerk applicationClerks[numApplicationClerks];

void createCustomer();
void runCustomer(int ssn);
void joinApplicationLine(int n);
int findSmallestApplicationLine();




void createCustomer()
{
	numCustomerThreads++;
	Write("C1\n", 4, ConsoleOutput);
			
	runCustomer(numCustomerThreads);
	Write("C2\n", 4, ConsoleOutput);

	Exit(0);
}

void runCustomer(int ssn)
{
	
	Write("Waiting to get in line...\n", 26, ConsoleOutput);
	joinApplicationLine(ssn);
	

	Exit(0);
}



void joinApplicationLine(int n)
{
	int myLine = 0;

	
	
	Write("Joining Applicaiton Line..\n", 27, ConsoleOutput);

	Acquire(applicationLock);
	myLine = findSmallestApplicationLine();

	if(applicationClerks[myLine].state == 1)
	{
		if(customers[n].bribed == true)
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

	/*customers[n].applicationAccepted = true;

	if(customers[n].bribed == true)
	{
		applicationBribeLineCounts[myLine]--;
	}
	else
	{
		applicationLineCounts[myLine]--;
	}*/

	Write("X\n",3, ConsoleOutput);
	Signal(applicationLineCVs[myLine], applicationLineLocks[myLine]);
	Write("Y\n",3, ConsoleOutput);
	Release(applicationLineLocks[myLine]);

	Release(applicationLock);
	Write("Joining Applicaiton Line2..\n", 28, ConsoleOutput);

	Exit(0);
}

void runApplicationClerk(int line)
{
	while(true)
	{
		Acquire(applicationLock);

		if(applicationBribeLineCounts[line] > 0)
		{
			Write("Application Clerk ", 18, ConsoleOutput);
			IntPrint(line);
			Write("has received $500 from Customer\n ", 32, ConsoleOutput);
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

	Exit(0);
}

void createApplicationClerk()
{
	numApplicationClerkThreads++;
	Write("A1\n", 4, ConsoleOutput);
	runApplicationClerk(numApplicationClerkThreads);
	Write("A2\n", 4, ConsoleOutput);

	Exit(0);
}


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

int main()
{
	int i;
	numCustomerThreads = -1;
	numApplicationClerkThreads = -1;

	/*Initialize Application Clerks here*/
	applicationLock = CreateLock();

	for(i = 0; i < numApplicationClerks; i++)
	{
		/*Basic application clerk info*/
		applicationClerks[i].state = 0;
		applicationClerks[i].lineCount = 0;
		applicationClerks[i].bribeLineCount = 0;
		applicationClerks[i].money = 0;
		applicationClerks[i].line = i;

		/*Application line locks and cvs*/
		applicationLineLocks[i] = CreateLock();
		applicationLineCVs[i] = CreateCondition();

		/*Fork(createApplicationClerk);*/
		/*Write("Creating Application Clerk...\n", 29, ConsoleOutput);*/
	}

	if(
	/*Initialize Customers here*/
	for(i = 0; i < numCustomers; i++)
	{
		customers[i].money = 500 + 500*Rand(3); /*random money 500, 1100, or 1600*/
		customers[i].ssn = i; /*ssn is id*/
		customers[i].applicationAccepted = false;
		customers[i].pictureTaken = false;
		customers[i].certified = false;
		customers[i].bribed = false;
		
		Exec("../test/customer", 40);
		/*Write("Creating Customer...\n", 28, ConsoleOutput);*/
		
		
	}

	Exit(0);
}

