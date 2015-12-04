#include "syscall.h"

#define true 1
#define false 0

#define BLANK 0

#define SetMV 0
#define GetMV 1

#define ClerkState 0
#define LineCount 1


#define NumApplicationClerks 1
#define NumPictureClerks 1
#define NumPassportClerks 1
#define NumCashiers 1

#define Ready 0
#define Busy 1
#define Break 2

#define EnterLine 1
#define ExitLine -1

#define Regular 0
#define Bribe 1

#define ApplicationClerk 0
#define PictureClerk 1
#define PassportClerk 2
#define Cashier 3

#define ApplicationLock 0
#define PictureLock 1
#define PassportLock 2
#define CashierLock 2

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

int ssn;
int money;
int bribed;
int applicationAccepted;
int pictureTaken;
int certified;	
int done;

char* vars = "                       ";
int DoFunc(int func, int type, int num, int var1, int var2, int var3, int var4)
{	
	vars[0] = var1 + '0';
	
	if(num > 1)
	{
		vars[2] = var2 + '0';

		if(num > 2)
		{
			vars[4] = var3 + '0';

			if(num > 3)
			{
				vars[6] = var4 + '0';
			}
		}
	}

	switch(func)
	{
		case 0:			
			SetMonitorVal(type, num, vars);
		break;
		case 1:
			return GetMonitorVal(type, num, vars);
		break;

	}
}

void Init()
{
	int i;
	int lastLock;
	int lastCV;

	ssn = 0;
	money = 500 + 500*Rand(3);
	bribed = false;
	applicationAccepted = false;
	pictureTaken = false;
	certified = false;
	done = false;

	lastLock = 4;
	lastCV = 0;

	for(i = 0; i < NumApplicationClerks; i++)
	{
		ApplicationLineLocks[i] = lastLock + i;
		ApplicationLineCVs[i] = lastCV + i;
		ApplicationBribeCVs[i] = lastCV + i + 1;
	}
	lastLock += NumApplicationClerks;
	lastCV += (2*NumApplicationClerks);

	for(i = 0; i < NumPictureClerks; i++)
	{
		PictureLineLocks[i] = lastLock + i;
		PictureLineCVs[i] = lastCV + i;
		PictureBribeCVs[i] = lastCV + i + 1;
	}
	lastLock += NumPictureClerks;
	lastCV += (2*NumPictureClerks);

	for(i = 0; i < NumPassportClerks; i++)
	{
		PassportLineLocks[i] = lastLock + i;
		PassportLineCVs[i] = lastCV + i;
		PassportBribeCVs[i] = lastCV + i + 1;
	}
	lastLock += NumPassportClerks;
	lastCV += (2*NumPassportClerks);

	for(i = 0; i < NumCashiers; i++)
	{
		CashierLineLocks[i] = lastLock + i;
		CashierLineCVs[i] = lastCV + i;
		CashierBribeCVs[i] = lastCV + i + 1;
	}
	lastLock += NumCashiers;
	lastCV += (2*NumCashiers);
}

void joinApplicationLine()
{
	int myLine;

	Acquire(ApplicationLock);
	myLine = 0; /*DONT FORGET TO GET SMALLEST LINE!*/

	if(DoFunc(GetMV, ClerkState, 2, ApplicationClerk, myLine, BLANK, BLANK) == Ready)
	{
		if(bribed == true)
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" has entered bribe line for Application Clerk ", 46, ConsoleOutput);
			IntPrint(myLine);
			Write("\n", 1, ConsoleOutput);

			DoFunc(SetMV, LineCount, 4, ApplicationClerk, myLine, 0, Bribe); /*Make sure to adjust this for 2-digit ssns*/
			
		}
		else
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" has entered regular line ", 26, ConsoleOutput);
			IntPrint(myLine);
			Write(" for Application Clerk.\n", 24, ConsoleOutput);

			DoFunc(SetMV, LineCount, 4, ApplicationClerk, myLine, 0, Regular);
		}

		Wait(ApplicationLineCVs[myLine], ApplicationLock);
	}

	DoFunc(SetMV, ClerkState, 3, ApplicationClerk, myLine, Ready, BLANK);

	Release(ApplicationLock);

	Acquire(ApplicationLineLocks[myLine]);

	Signal(ApplicationLineCVs[myLine], ApplicationLineLocks[myLine]);

	Wait(ApplicationLineCVs[myLine], ApplicationLineLocks[myLine]);

	applicationAccepted = true;

	if(bribed == true)
	{
		DoFunc(SetMV, LineCount, 4, ApplicationClerk, myLine, ExitLine, Bribe);
	}
	else
	{
		DoFunc(SetMV, LineCount, 4, ApplicationClerk, myLine, ExitLine, Regular);
	}

	Signal(ApplicationLineCVs[myLine], ApplicationLineLocks[myLine]);
	Release(ApplicationLineLocks[myLine]);
}

void joinPictureLine()
{
	int myLine;
	int likePic;

	Acquire(PictureLock);
	myLine = 0;

	if(DoFunc(GetMV, ClerkState, 2, PictureClerk, myLine, BLANK, BLANK) == Ready)
	{
		if(bribed == true)
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" has entered bribe line for Picture Clerk ", 42, ConsoleOutput);
			IntPrint(myLine);
			Write("\n", 1, ConsoleOutput);

			DoFunc(SetMV, LineCount, 4, PictureClerk, myLine, 0, Bribe); /*Make sure to adjust this for 2-digit ssns*/
			
		}
		else
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" has entered regular line ", 26, ConsoleOutput);
			IntPrint(myLine);
			Write(" for Picture Clerk.\n", 20, ConsoleOutput);

			DoFunc(SetMV, LineCount, 4, PictureClerk, myLine, EnterLine, Regular);
		}

		Wait(PictureLineCVs[myLine], PictureLock);
	}

	DoFunc(SetMV, ClerkState, 3, PictureClerk, myLine, Ready, BLANK);

	Release(PictureLock);

	Acquire(PictureLineLocks[myLine]);

	/*Customer may dislike picture and retake it*/
	likePic = 0;
	while(likePic == 0)
	{
		Signal(PictureLineCVs[myLine], PictureLineLocks[myLine]);

		Wait(PictureLineCVs[myLine], PictureLineLocks[myLine]);

		likePic = Rand(2);

		if(likePic == 0)
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" does not like their picture from Picture Clerk ", 48, ConsoleOutput);
			IntPrint(myLine);
			Write("\n", 1, ConsoleOutput);
		}
		else
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" likes their picture from Picture Clerk ", 40, ConsoleOutput);
			IntPrint(myLine);
			Write("\n", 1, ConsoleOutput);
		}
	}
	pictureTaken = true;

	if(bribed == true)
	{
		DoFunc(SetMV, LineCount, 4, PictureClerk, myLine, ExitLine, Bribe);
	}
	else
	{
		DoFunc(SetMV, LineCount, 4, PictureClerk, myLine, ExitLine, Regular);
	}

	Signal(PictureLineCVs[myLine], PictureLineLocks[myLine]);
	Release(PictureLineLocks[myLine]);
}

joinPassportLine()
{
	int myLine;
	int yieldCalls;
	int i;

	Acquire(PassportLock);
	myLine = 0; /*DONT FORGET TO GET SMALLEST LINE!*/

	if(DoFunc(GetMV, ClerkState, 2, PassportClerk, myLine, BLANK, BLANK) == Ready)
	{
		if(bribed == true)
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" has entered bribe line for Passport Clerk ", 43, ConsoleOutput);
			IntPrint(myLine);
			Write("\n", 1, ConsoleOutput);

			DoFunc(SetMV, LineCount, 4, PassportClerk, myLine, 0, Bribe); /*Make sure to adjust this for 2-digit ssns*/
			
		}
		else
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" has entered regular line ", 26, ConsoleOutput);
			IntPrint(myLine);
			Write(" for Passport Clerk.\n", 21, ConsoleOutput);

			DoFunc(SetMV, LineCount, 4, PassportClerk, myLine, EnterLine, Regular);
		}

		Wait(PassportLineCVs[myLine], PassportLock);
	}

	DoFunc(SetMV, ClerkState, 3, PassportClerk, myLine, Ready, BLANK);

	Release(PassportLock);

	Acquire(PassportLineLocks[myLine]);

	Signal(PassportLineCVs[myLine], PassportLineLocks[myLine]);

	Wait(PassportLineCVs[myLine], PassportLineLocks[myLine]);

	if(bribed == true)
	{
		DoFunc(SetMV, LineCount, 4, ApplicationClerk, myLine, ExitLine, Bribe);
	}
	else
	{
		DoFunc(SetMV, LineCount, 4, ApplicationClerk, myLine, ExitLine, Regular);
	}


	if(applicationAccepted == true && pictureTaken == true)
	{
		certified = true;
	}
	else
	{
		Write("Customer ", 9, ConsoleOutput);
		IntPrint(ssn);
		Write(" has gone to Passport Clerk ", 28, ConsoleOutput);
		IntPrint(myLine);
		Write(" too soon. They are going to the back of the line\n", 50, ConsoleOutput);
		
		yieldCalls = 100 + Rand(901);
		for(i = 0; i < yieldCalls; i++)
		{
			Yield();
		}
	}

	
	Signal(ApplicationLineCVs[myLine], ApplicationLineLocks[myLine]);
	Release(ApplicationLineLocks[myLine]);
}

void joinCashierLine()
{
	int myLine;
	int yieldCalls;
	int i;

	Acquire(CashierLock);
	myLine = 0; /*DONT FORGET TO GET SMALLEST LINE!*/

	if(DoFunc(GetMV, ClerkState, 2, Cashier, myLine, BLANK, BLANK) == Ready)
	{
		if(bribed == true)
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" has entered bribe line for Cashier ", 36, ConsoleOutput);
			IntPrint(myLine);
			Write("\n", 1, ConsoleOutput);

			DoFunc(SetMV, LineCount, 4, Cashier, myLine, 0, Bribe); /*Make sure to adjust this for 2-digit ssns*/
			
		}
		else
		{
			Write("Customer ", 9, ConsoleOutput);
			IntPrint(ssn);
			Write(" has entered regular line ", 26, ConsoleOutput);
			IntPrint(myLine);
			Write(" for Cashier\n", 13, ConsoleOutput);

			DoFunc(SetMV, LineCount, 4, Cashier, myLine, EnterLine, Regular);
		}

		Wait(CashierLineCVs[myLine], CashierLock);
	}

	DoFunc(SetMV, ClerkState, 3, Cashier, myLine, Ready, BLANK);

	Release(CashierLock);

	Acquire(CashierLineLocks[myLine]);

	Signal(CashierLineCVs[myLine], CashierLineLocks[myLine]);

	Wait(CashierLineCVs[myLine], CashierLineLocks[myLine]);

	if(bribed == true)
	{
		DoFunc(SetMV, LineCount, 4, Cashier, myLine, ExitLine, Bribe);
	}
	else
	{
		DoFunc(SetMV, LineCount, 4, Cashier, myLine, ExitLine, Regular);
	}

	if(certified == false)
	{
		Write("Customer ", 9, ConsoleOutput);
		IntPrint(ssn);
		Write(" has gone to Cashier ", 21, ConsoleOutput);
		IntPrint(myLine);
		Write(" too soon. They are going to the back of the line\n", 50, ConsoleOutput);

		yieldCalls = 100 + Rand(901);
		for(i = 0; i < yieldCalls; i++)
		{
			Yield();
		}
	}
	else
	{
		done = true;
	}	

	Signal(CashierLineCVs[myLine], CashierLineLocks[myLine]);
	Release(CashierLineLocks[myLine]);
}

int main()
{
	Write("ID\n", 5, ConsoleOutput);
	
	Init();
	
	joinApplicationLine();
	joinPictureLine();
	joinPassportLine();
	joinCashierLine();

	Exit(0);
}