#include "syscall.h"

#define true 1
#define false 0

#define BLANK 0

#define SetMV 0
#define GetMV 1

#define ClerkState 0
#define LineCount 1
#define FrontOfLine 2

#define CashierLock 0
#define NumCashierLines 1

#define Cashier 0

#define Ready 0
#define Busy 1
#define Break 2

#define Regular 0
#define Bribe 1

int CashierLineLocks[LineCount];

int CashierLineCVs[LineCount];
int CashierBribeCVs[LineCount];


int line;
int state;
int lineCount;
int bribeLineCount;
int money;

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

	line = 0;
	state = Ready;
	lineCount = 0;
	bribeLineCount = 0;
	money = 0;

	for(i = 0; i < NumCashierLines; i++)
	{
		CashierLineLocks[i] = 4 + i;
		CashierLineCVs[i] = i;
		CashierBribeCVs[i] = i + 1;
	}
}

void Run()
{
	int frontSSN;
	int lineType;
	while(true)
	{
		Acquire(CashierLock);

		bribeLineCount = DoFunc(GetMV, LineCount, 3, Cashier, line, Bribe, BLANK);
		lineCount = DoFunc(GetMV, LineCount, 3, Cashier, line, Regular, BLANK);
		if(bribeLineCount > 0)
		{
			Write("Cashier Clerk ", 18, ConsoleOutput);
			IntPrint(line);
			Write(" has received $500 from Customer\n ", 33, ConsoleOutput);

			lineType = Bribe;

			Signal(CashierBribeCVs[line], CashierLock);
			DoFunc(SetMV, ClerkState, 3, Cashier, line, Busy, BLANK);
		}
		else if(lineCount > 0)
		{
			Write("Cashier Clerk ", 18, ConsoleOutput);
			IntPrint(line);
			Write(" has signalled a Customer to come to the counter\n", 49, ConsoleOutput);

			lineType = Regular;

			Signal(CashierLineCVs[line], CashierLock);
			DoFunc(SetMV, ClerkState, 3, Cashier, line, Busy, BLANK);
		}
		else
		{
			DoFunc(SetMV, ClerkState, 3, Cashier, line, Break, BLANK);
		}

		Acquire(CashierLineLocks[line]);
		Release(CashierLock);

		Wait(CashierLineCVs[line], CashierLineLocks[line]);
			
		if(lineType == Bribe)
		{
			frontSSN = DoFunc(GetMV, FrontOfLine, 3, Cashier, line, Bribe, BLANK);
		}
		else
		{
			frontSSN = DoFunc(GetMV, FrontOfLine, 3, Cashier, line, Regular, BLANK);
		}

		Write("Customer ", 9, ConsoleOutput);
		IntPrint(frontSSN);
		Write(" has given SSN ", 15, ConsoleOutput);
		IntPrint(frontSSN);
		Write(" to Applicatoin Clerk ", 22, ConsoleOutput); 
		IntPrint(line);
		Write("\n", 1, ConsoleOutput);

		Signal(CashierLineCVs[line], CashierLineLocks[line]);

		Wait(CashierLineCVs[line], CashierLineLocks[line]);

		Release(CashierLineLocks[line]);
	}
}


int main()
{
	Init();
	Run();

	Exit(0);
}