#include "syscall.h"

#define true 1
#define false 0

#define NumCustomers 1
#define NumApplicationClerks 1
#define NumPictureClerks 1
#define NumPassportClerks 1
#define NumCashiers 1

int clerkLocks[4];

int ApplicationLineLocks[NumApplicationClerks];
int PictureLineLocks[NumPictureClerks];
int PassportLineLocks[NumPassportClerks];
int CashierLineLocks[NumCashiers];

int ApplicationLineCVs[NumApplicationClerks];
int PictureLineCVs[NumPictureClerks];
int PassportLineCVs[NumPassportClerks];
int CashierLineCVs[NumCashiers];

int ApplicationBribeCVs[NumApplicationClerks];
int PictureBribeCVs[NumPictureClerks];
int PassportBribeCVs[NumPassportClerks];
int CashierBribeCVs[NumCashiers];

int main()
{
	int i;
	int clientJobs = 0;
	int newJob;

	CreateMonitor();

	/*Create clerk locks*/
	for(i = 0; i < 4; i++)
	{
		clerkLocks[i] = CreateLock();
	}

	/*Create Application Clerk Locks and CVs*/
	for(i = 0; i < NumApplicationClerks; i++)
	{
		ApplicationLineLocks[i] = CreateLock();
		ApplicationLineCVs[i] = CreateCondition();
		ApplicationBribeCVs[i] = CreateCondition();
	}

	/*Create Picture Clerk Locks and CVs*/
	for(i = 0; i < NumPictureClerks; i++)
	{
		PictureLineLocks[i] = CreateLock();
		PictureLineCVs[i] = CreateCondition();
		PictureBribeCVs[i] = CreateCondition();
	}

	/*Create Passport Clerk Locks and CVs*/
	for(i = 0; i < NumPassportClerks; i++)
	{
		PassportLineLocks[i] = CreateLock();
		PassportLineCVs[i] = CreateCondition();
		PassportBribeCVs[i] = CreateCondition();
	}

	/*Create Cashier Locks and CVs*/
	for(i = 0; i < NumCashiers; i++)
	{
		CashierLineLocks[i] = CreateLock();
		CashierLineCVs[i] = CreateCondition();
		CashierBribeCVs[i] = CreateCondition();
	}
	
	/*Exec customers and clerks, number of each predefined above*/
			
		Write("Exec customer\n", 16, ConsoleOutput);
		Exec("../test/customer", 40);
	
	
		Write("Exec application Clerk\n", 24, ConsoleOutput);
		Exec("../test/ApplicationClerk", 40);
	
		Write("Exec picture Clerk\n", 24, ConsoleOutput);
		Exec("../test/PictureClerk", 40);
	
		Write("Exec passport Clerk\n", 24, ConsoleOutput);
		Exec("../test/PassportClerk", 40);
			
		Write("Exec Cashier\n", 24, ConsoleOutput);
		Exec("../test/Cashier", 40);
	

	
}