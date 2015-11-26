#define MAX 5

#include "syscall.h"

int wakingUp[]={0,0,0,0,0};

char nameOfLock[20]; 
char nameOfCond[20];
int lockCountorGuest;

int lockOfMain;
int conditionOfMain;

int integ;
int locksArr[MAX];
int condsArr[MAX];

int guestCount = 0;
int awake = 0;

void TestFunction()
{
	int identification; 

	AcquireLock(lockCountorGuest);
	identification = guestCount++;

	ReleaseLock(lockCountorGuest);
	
	Write("NUMBER OF THREAD",20 , 1);
	WriteNum(identification);Write("FORKING \n",20,1);

	if(identification == MAX-1)
	{
		AcquireLock(lockOfMain);
		awake = 1;

		SignalCV(conditionOfMain, lockOfMain);
		ReleaseLock(lockOfMain);
	}

	/*THREAD WAITING ON ITS LOCK*/
	AcquireLock(locksArr[identification]);

	/*THREAD WAITING ON ITS LOCK*/
	while(wakingUp[identification]==0)
		WaitCV(condsArr[identification],locksArr[identification]);
	ReleaseLock(locksArr[identification]);

	Write("WAKING : ",8,1);

	WriteNum(identification);

	Write("\n",1,1);

	Exit(0);
}

int main()
{
	int i, lockName;

	lockCountorGuest = CreateLock("condition lock",6);
	lockOfMain = CreateLock("main lock",8);
	conditionOfMain = CreateCondition("main condition",8);

	for(i=0; i<MAX; i++)
	{
		wakingUp[integ] = 0;
		Concatenate("Lock identification",sizeof("lock identification"),integ,lockName);
		locksArr[integ] = CreateLock(lockName,sizeof(lockName));

		Concatenate("Condition identification",sizeof("condition identification"),integ,nameOfCond);
		condsArr[integ] = CreateCondition(nameOfCond,sizeof(nameOfCond));
	} 
	for(integ=0;integ<MAX;integ++)
	{
		Fork(TestFunction);
	}
	AcquireLock(lockOfMain);
	while(awake == 0)
		Wait(conditionOfMain, lockOfMain);

	Release(lockOfMain);
		
	for(i=0; i<MAX; i++)
	{
		AcquireLock(locksArr[integ]);

		wakingUp[integ]=1;
		Signal(condsArr[i],locksArr[integ]);

		Release(locksArr[integ]);
	}
	
	Write("ACQUIRING LOCK ID\n",50,1);
	Acquire(-1);
	Acquire(33);
	
	/*MORE TESTS ON CV*/
	Wait(-1,1);
	Wait(34,1);
	Signal(55,-1);
	Exit(0);
}
