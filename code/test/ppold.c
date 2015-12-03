




#include "syscall.h"

#define true 1
#define false 0

#define  numCustomers 1
#define  numApplicationClerks 1
#define  numPictureClerks 5
#define  numPassportClerks 5
#define  numCashiers 5
#define  numManagers 1 
#define  numSenators 10


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
int pictureLineLocks[numPictureClerks];
int passportLineLocks[numPassportClerks];
int cashierLineLocks[numCashiers];

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
int passportBribeLineCVs[numPassportClerks];
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

void joinApplicationLine(int n)
{
	int myLine;

	
	Write("Joining Applicaiton Line.. \n", 100, ConsoleOutput);

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

	customers[n].applicationAccepted = true;

	if(customers[n].bribed == true)
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

struct PictureClerk
{
	int state; /* 0: available     1: busy       2: on break*/
	int lineCount;
	int bribeLineCount;
	int money;
	int line;
};
struct PictureClerk pictureClerks[numPictureClerks];

struct PictureClerkMonitor
{
	int pictureTaken;
	int customerSSN;
};
struct PictureClerkMonitor picClerkMonitor[numPictureClerks];

int findSmallestPictureLine()
{
	int i;
	int smallest = 50;
	int smallestIndex = -1;
	for (i = 0; i < numPictureClerks; i++)
	{
		if (pictureLineCounts[i] < smallest)
		{
			smallest = pictureLineCounts[i];
			smallestIndex = i;
		}
	}
	return smallestIndex;
}

void joinPictureLine(int ssn)
{
	int myLine, likePicture = false;

	Acquire(pictureLock);
	myLine = findSmallestPictureLine();

	if (pictureClerks[myLine].state == 1)
	{
		if (customers[ssn].bribed == true)
		{
			pictureBribeLineCounts[myLine]++;

		}
		else
		{
			pictureLineCounts[myLine]++;
		}

		Wait(pictureLineCVs[myLine], pictureLock);
	}

	pictureClerks[myLine].state = 1;
	Release(pictureLock);

	Acquire(pictureLineLocks[myLine]);

	picClerkMonitor[myLine].pictureTaken = false;
	picClerkMonitor[myLine].customerSSN = ssn;

	while (likePicture == false)
	{
		Signal(pictureLineCVs[myLine], pictureLineLocks[myLine]);

		Wait(pictureLineCVs[myLine], pictureLineLocks[myLine]);

		likePicture = Rand(2);
		picClerkMonitor[myLine].pictureTaken = likePicture;
	}

	customers[ssn].pictureTaken = true;

	if (customers[ssn].bribed == true)
	{
		pictureBribeLineCounts[myLine]--;
	}
	else
	{
		pictureLineCounts[myLine]--;
	}

	Signal(pictureLineCVs[myLine], pictureLineLocks[myLine]);
	Release(pictureLineLocks[myLine]);
}

struct PassportClerk
{
	int state; /* 0: available     1: busy       2: on break*/
	int lineCount;
	int bribeLineCount;
	int money;
	int line;
};
struct PassportClerk passportClerks[numPassportClerks];

struct PassportClerkMonitor
{
	int canCertify;
	int passportCertified;
	int customerSSN;
};
struct PassportClerkMonitor passClerkMonitor[numPassportClerks];

int findSmallestPassportLine()
{
	int i;
	int smallest = 50;
	int smallestIndex = -1;
	for (i = 0; i < numPassportClerks; i++)
	{
		if (passportLineCounts[i] < smallest)
		{
			smallest = passportLineCounts[i];
			smallestIndex = i;
		}
	}
	return smallestIndex;
}

void joinPassportLine(int ssn)
{
	int myLine, yieldNum, i;

	Acquire(passportLock);
	myLine = findSmallestPassportLine();

	if (passportClerks[myLine].state == 1)
	{
		if (customers[ssn].bribed == true)
		{
			passportBribeLineCounts[myLine]++;

		}
		else
		{
			passportLineCounts[myLine]++;
		}

		Wait(passportLineCVs[myLine], passportLock);
	}

	passportClerks[myLine].state = 1;
	Release(passportLock);

	Acquire(passportLineLocks[myLine]);

	passClerkMonitor[myLine].passportCertified = false;
	passClerkMonitor[myLine].customerSSN = ssn;

	if (customers[ssn].pictureTaken && customers[ssn].applicationAccepted)
	{
		passClerkMonitor[myLine].canCertify = true;
		customers[ssn].certified = true;
	}
		
	else
	{
		yieldNum = Rand(900) + 100;
		for (i = 0; i < yieldNum; i++)
			Yield();
	}

	Signal(passportLineCVs[myLine], passportLineLocks[myLine]);
	Wait(passportLineCVs[myLine], passportLineLocks[myLine]);

	/*customers[ssn].certified = true;*/

	if (customers[ssn].bribed == true)
	{
		passportBribeLineCounts[myLine]--;
	}
	else
	{
		passportLineCounts[myLine]--;
	}

	Signal(passportLineCVs[myLine], passportLineLocks[myLine]);
	Release(passportLineLocks[myLine]);
}

struct Cashier
{
	int state; /* 0: available     1: busy       2: on break*/
	int lineCount;
	int bribeLineCount;
	int money;
	int line;
};
struct Cashier cashiers[numCashiers];

struct CashierMonitor
{
	int certified;
	int customerSSN;
};
struct CashierMonitor cashierMonitor[numCashiers];

int findSmallestCashierLine()
{
	int i;
	int smallest = 50;
	int smallestIndex = -1;
	for (i = 0; i < numCashiers; i++)
	{
		if (cashierLineCounts[i] < smallest)
		{
			smallest = cashierLineCounts[i];
			smallestIndex = i;
		}
	}
	return smallestIndex;
}

void joinCashierLine(int ssn)
{
	int myLine, yieldNum, i;

	Acquire(cashierLock);
	myLine = findSmallestCashierLine();

	if (cashiers[myLine].state == 1)
	{
		if (customers[ssn].bribed == true)
		{
			cashierBribeLineCounts[myLine]++;

		}
		else
		{
			cashierLineCounts[myLine]++;
		}

		Wait(cashierLineCVs[myLine], cashierLock);
	}

	cashiers[myLine].state = 1;
	Release(cashierLock);

	Acquire(cashierLineLocks[myLine]);

	cashierMonitor[myLine].certified = false;
	cashierMonitor[myLine].customerSSN = ssn;

	if (customers[ssn].certified)
	{
		cashierMonitor[myLine].certified = true;
		customers[ssn].done = true;
		customers[ssn].money -= 100;
	}

	else
	{
		yieldNum = Rand(900) + 100;
		for (i = 0; i < yieldNum; i++)
			Yield();
	}

	Signal(cashierLineCVs[myLine], cashierLineLocks[myLine]);
	Wait(cashierLineCVs[myLine], cashierLineLocks[myLine]);

	/*customers[ssn].certified = true;*/

	if (customers[ssn].bribed == true)
	{
		cashierBribeLineCounts[myLine]--;
	}
	else
	{
		cashierLineCounts[myLine]--;
	}

	Signal(cashierLineCVs[myLine], cashierLineLocks[myLine]);
	Release(cashierLineLocks[myLine]);
}

struct Manager 
{
	int pClerkMoney;
	int aClerkMoney;
	int ppClerkMoney;
	int cClerkMoney;
	int totalMoney;
};
struct Manager manager;

void updateTotalMoney() {
	int i = 0;
	
	manager.pClerkMoney = 0;
	manager.aClerkMoney = 0;
	manager.ppClerkMoney = 0;
	manager.cClerkMoney = 0;

	for (i = 0; i < numApplicationClerks; i++)
		manager.aClerkMoney += applicationClerks[i].money;
	for (i = 0; i < numPictureClerks; i++)
		manager.pClerkMoney += pictureClerks[i].money;
	for (i = 0; i < numPassportClerks; i++)
		manager.ppClerkMoney += passportClerks[i].money;
	for (i = 0; i < numCashiers; i++)
		manager.cClerkMoney += cashiers[i].money;

	manager.totalMoney = manager.pClerkMoney + manager.aClerkMoney + manager.ppClerkMoney + manager.cClerkMoney;
}

int getaClerkMoney() {
	updateTotalMoney();
	Write("Manager has counted a total of $", 32, ConsoleOutput);
	IntPrint(manager.aClerkMoney);
	Write("for ApplicationClerks.\n ", 23, ConsoleOutput);
	return manager.aClerkMoney;
}

int getpClerkMoney() {
	updateTotalMoney();
	Write("Manager has counted a total of $", 32, ConsoleOutput);
	IntPrint(manager.pClerkMoney);
	Write("for PictureClerks.\n ", 23, ConsoleOutput);
	return manager.pClerkMoney;
}

int getppClerkMoney() {
	updateTotalMoney();
	Write("Manager has counted a total of $", 32, ConsoleOutput);
	IntPrint(manager.ppClerkMoney);
	Write("for PassportClerks.\n ", 23, ConsoleOutput);
	return manager.ppClerkMoney;
}

int getcClerkMoney() {
	updateTotalMoney();
	Write("Manager has counted a total of $", 32, ConsoleOutput);
	IntPrint(manager.cClerkMoney);
	Write("for Cashiers.\n ", 14, ConsoleOutput);
	return manager.cClerkMoney;
}

int gettotalMoney() {
	updateTotalMoney();
	Write("Manager has counted a total of $", 32, ConsoleOutput);
	IntPrint(manager.totalMoney);
	Write("for the office.\n ", 15, ConsoleOutput);
	return manager.totalMoney;
}

void makeManager()
{
	numManagerThreads++;
}

void runCustomer(int ssn)
{
	
	Write("\n\nWaiting to get in line... \n\n", 40, ConsoleOutput);
	while(customers[ssn].done == false);
	{
		int randomLine = 0;/*Rand(4); /*generate random number here*/

		
		switch(randomLine)
		{
			case 0:
				Write("X...\n", 40, ConsoleOutput);
				/*if(customers[ssn].applicationAccepted == false)
				{*/
					Write("Y...\n", 40, ConsoleOutput);
					joinApplicationLine(ssn);
					Write("Z...\n", 40, ConsoleOutput);
				/*}*/
			break;
			case 1:
				if(customers[ssn].pictureTaken == false)
				{
					joinPictureLine(ssn);
				}
			break;
			case 2:
				if(customers[ssn].certified == false)
				{
					joinPassportLine(ssn);
				}
			break;
			case 3:
				if (customers[ssn].done == false)
					joinCashierLine(ssn); 
			break;
		}


	}
}

void createCustomer()
{
	numCustomerThreads++;
	Write("C1\n", 4, ConsoleOutput);
			
	runCustomer(numCustomerThreads);
	Write("C2\n", 4, ConsoleOutput);
}
struct Senator 
{
	int money;
	int ssn;
	int applicationAccepted;
	int pictureTaken;
	int certified;
	int bribed;
};
struct Senator senators[numSenators];

void runSenator(int ssn)
{
	senators[ssn].ssn = ssn;
}

void createSenator()
{
	numSenatorThreads++;
	/*runSenator(numSenatorThreads);*/
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
}

void createApplicationClerk()
{
	numApplicationClerkThreads++;
	Write("A1\n", 4, ConsoleOutput);
	runApplicationClerk(numApplicationClerkThreads++);
	Write("A2\n", 4, ConsoleOutput);
}


void runPictureClerk(int line)
{
	int yieldNum, i;
	while (true)
	{
		picClerkMonitor[line].pictureTaken = false;
		picClerkMonitor[line].customerSSN = -1;

		Acquire(pictureLock);

		if (pictureBribeLineCounts[line] > 0)
		{
			Write("Picture Clerk ", 14, ConsoleOutput);
			IntPrint(line);
			Write("has received $500 from Customer\n ", 32, ConsoleOutput);
			Signal(pictureBribeLineCVs[line], pictureLock);
			pictureClerks[line].state = 1;
		}
		else if (pictureLineCounts[line] > 0)
		{
			Signal(pictureLineCVs[line], pictureLock);
			pictureClerks[line].state = 1;
		}
		else
		{
			Write("Picture Clerk ", 14, ConsoleOutput);
			IntPrint(line);
			Write("is going on break\n", 18, ConsoleOutput);
			pictureClerks[line].state = 2; /*on break*/
		}

		Acquire(pictureLineLocks[line]);
		Release(pictureLock);

		Wait(pictureLineCVs[line], pictureLineLocks[line]);
		
		while (picClerkMonitor[line].pictureTaken == false)
		{
			yieldNum = 20 + Rand(80);
			for (i = 0; i < yieldNum; i++)
			{
				Yield();
			}

			Signal(pictureLineCVs[line], pictureLineLocks[line]);

			Wait(pictureLineCVs[line], pictureLineLocks[line]);

			Write("Picture Clerk ", 14, ConsoleOutput);
			IntPrint(line);

			if (picClerkMonitor[line].pictureTaken == true)
			{
				Write(" has been told that Customer ", 28, ConsoleOutput);
				IntPrint(picClerkMonitor[line].customerSSN);
				Write(" does like their picture\n", 24, ConsoleOutput);
			}
			else
			{
				Write(" has been told that Customer ", 28, ConsoleOutput);
				IntPrint(picClerkMonitor[line].customerSSN);
				Write(" does not like their picture\n", 24, ConsoleOutput);
			}
		}

		Release(pictureLineLocks[line]);

	}
}

void createPictureClerk()
{
	numPictureClerkThreads++;
	runPictureClerk(numPictureClerkThreads++);
}

void runPassportClerk(int line)
{
	int yieldNum, i;
	while (true)
	{
		passClerkMonitor[line].passportCertified = false;
		passClerkMonitor[line].customerSSN = -1;
		passClerkMonitor[line].canCertify = false;

		Acquire(passportLock);

		if (passportBribeLineCounts[line] > 0)
		{
			Write("Passport Clerk ", 14, ConsoleOutput);
			IntPrint(line);
			Write(" has received $500 from Customer\n ", 32, ConsoleOutput);
			Signal(passportBribeLineCVs[line], passportLock);
			passportClerks[line].state = 1;
		}
		else if (passportLineCounts[line] > 0)
		{
			Signal(passportLineCVs[line], passportLock);
			passportClerks[line].state = 1;
		}
		else
		{
			Write("Passport Clerk ", 14, ConsoleOutput);
			IntPrint(line);
			Write(" is going on break\n", 19, ConsoleOutput);
			passportClerks[line].state = 2; /*on break*/
		}

		Acquire(passportLineLocks[line]);
		Release(passportLock);

		Wait(passportLineCVs[line], passportLineLocks[line]);

		Write("Passport Clerk ", 14, ConsoleOutput);
		IntPrint(line);

		if (passClerkMonitor[line].canCertify == true)
		{
			Write(" has determined that Customer ", 28, ConsoleOutput);
			IntPrint(picClerkMonitor[line].customerSSN);
			Write(" does have both their application and picture completed.\n", 24, ConsoleOutput);
		}
		else
		{
			Write(" has been told that Customer ", 28, ConsoleOutput);
			IntPrint(picClerkMonitor[line].customerSSN);
			Write(" does not have both their application and picture completed.\n", 24, ConsoleOutput);
			yieldNum = 20 + Rand(80);
			for (i = 0; i < yieldNum; i++)
			{
				Yield();
			}
		}

		Signal(passportLineCVs[line], passportLineLocks[line]);

		Wait(passportLineCVs[line], passportLineLocks[line]);

		Release(passportLineLocks[line]);

	}
}

void createPassportClerk()
{
	numPassportClerkThreads++;
	runPassportClerk(numPassportClerkThreads++);
}

void runCashier(int line)
{
	int yieldNum, i;
	while (true)
	{
		cashierMonitor[line].certified = false;
		cashierMonitor[line].customerSSN = -1;

		Acquire(cashierLock);

		if (cashierBribeLineCounts[line] > 0)
		{
			Write("Cashier ", 8, ConsoleOutput);
			IntPrint(line);
			Write(" has received $500 from Customer\n ", 32, ConsoleOutput);
			Signal(cashierBribeLineCVs[line], cashierLock);
			cashiers[line].state = 1;
		}
		else if (cashierLineCounts[line] > 0)
		{
			Signal(cashierLineCVs[line], cashierLock);
			cashiers[line].state = 1;
		}
		else
		{
			Write("Cashier ", 8, ConsoleOutput);
			IntPrint(line);
			Write(" is going on break\n", 19, ConsoleOutput);
			cashiers[line].state = 2; /*on break*/
		}

		Acquire(cashierLineLocks[line]);
		Release(cashierLock);

		Wait(cashierLineCVs[line], cashierLineLocks[line]);

		Write("Cashier ", 8, ConsoleOutput);
		IntPrint(line);

		if (cashierMonitor[line].certified == true)
		{
			Write(" has verified that Customer ", 28, ConsoleOutput);
			IntPrint(picClerkMonitor[line].customerSSN);
			Write(" has been certified by a passport clerk.\n", 24, ConsoleOutput);

			cashiers[line].money += 100;
			Write("Cashier ", 8, ConsoleOutput);
			IntPrint(line);
			Write(" has receive $100 from Customer ", 28, ConsoleOutput);
			IntPrint(picClerkMonitor[line].customerSSN);
			Write(".\n", 22, ConsoleOutput);
		}
		else
		{
			Write(" has been told that Customer ", 28, ConsoleOutput);
			IntPrint(picClerkMonitor[line].customerSSN);
			Write(" has not been certified by a passport clerk.\n", 24, ConsoleOutput);
			yieldNum = 20 + Rand(80);
			for (i = 0; i < yieldNum; i++)
			{
				Yield();
			}
		}

		Signal(cashierLineCVs[line], cashierLineLocks[line]);

		Wait(cashierLineCVs[line], cashierLineLocks[line]);

		Release(cashierLineLocks[line]);

	}
}

void createCashier()
{
	numCashierThreads++;
	runCashier(numCashierThreads++);
}


int main()
{
	int i = 0;
	
	/*initialize number of created threads*/
	numCustomerThreads = -1;
	numApplicationClerkThreads = -1;

	
	/*Initialize Customers here*/
	/*for(i = 0; i < numCustomers; i++)
	{
		customers[i].money = 500 + 500*Rand(3); /*random money 500, 1100, or 1600
		customers[i].ssn = i; /*ssn is id
		customers[i].applicationAccepted = false;
		customers[i].pictureTaken = false;
		customers[i].certified = false;
		customers[i].bribed = false;*/
		
		
		Write("Creating Customer...\n", 28, ConsoleOutput);
		createCustomer();
	/*}

	/*Initialize Application Clerks here*/
	/*for(i = 0; i < numApplicationClerks; i++)
	{
		applicationClerks[i].state = 0;
		applicationClerks[i].lineCount = 0;
		applicationClerks[i].bribeLineCount = 0;
		applicationClerks[i].money = 0;
		applicationClerks[i].line = i;


		/*Fork(createApplicationClerk);
		Write("Created Application Clerk\n", 28, ConsoleOutput);
	}


	/*Initialize Picture Clerks here
	for (i = 0; i < numPictureClerks; i++)
	{
		pictureClerks[i].state = 0;
		pictureClerks[i].lineCount = 0;
		pictureClerks[i].bribeLineCount = 0;
		pictureClerks[i].money = 0;
		pictureClerks[i].line = i;

		/*Fork((void(*)())createPictureClerk);
	}
	
	/*Initialize Passport Clerks here
	for (i = 0; i < numPassportClerks; i++)
	{
		passportClerks[i].state = 0;
		passportClerks[i].lineCount = 0;
		passportClerks[i].bribeLineCount = 0;
		passportClerks[i].money = 0;
		passportClerks[i].line = i;

		/*Fork((void(*)())createPassportClerk);
	}

	/*Initialize Manager here
	manager.pClerkMoney = 0;
	manager.aClerkMoney = 0;
	manager.ppClerkMoney = 0;
	manager.cClerkMoney = 0;
	manager.totalMoney = 0;
	/*Fork((void(*)())makeManager);
  
	/*Initialize Senators here
	for (i = 0; i < numSenators; i++)
	{
		senators[i].money = 500 + 500 * Rand(3); /*random money 500, 1100, or 1600
		senators[i].ssn = i; /*ssn is id
		senators[i].applicationAccepted = false;
		senators[i].pictureTaken = false;
		senators[i].certified = false;
		senators[i].bribed = false;

		/*Fork((void(*)())createSenator);
	}*/
}
