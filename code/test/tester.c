/* tester.c
 *  Simple program to test the file handling system calls
 */

#include "syscall.h"

 int lockID;
 int conditionID;

 
void test1();
 void test2();
 void test3();

int main() {
  /*OpenFileId fd;
  int bytesread;
  char buf[20];*/

    /*Create("testfile", 8);
    fd = Open("testfile", 8);

    Write("testing a write\n", 16, fd );
    Close(fd);


    fd = Open("testfile", 8);
    bytesread = Read( buf, 100, fd );
    Write( buf, bytesread, ConsoleOutput );
	Close(fd);*/
	/*int num;
	num = Rand(3);
	if (num == 0)
		Write("0!\n", 3, ConsoleOutput);
	else if (num == 1)
		Write("1!\n", 3, ConsoleOutput);
	else if (num == 2)
		Write("2!\n", 3, ConsoleOutput);*/

    /* test1();*/
	/*test3();*/

	/*int id = CreateLock();
	int id2, cv_id, mv_id;
	Acquire(id);
	Release(id);
	DestroyLock(id);

	id2 = CreateLock();
	cv_id = CreateCondition();
	DestroyCondition(cv_id);

	mv_id = CreateMonitor();
	DestroyMonitor(mv_id);*/

	test3();
}


void test2_t1(){
	Acquire(lockID);
	DestroyLock(lockID);
	Exit(0);
}

void test2_t2(){
	/*lockID = CreateLock();*/
	Yield();
	Release(lockID);
	Exit(0);
}

void test1()
{
  int id = CreateLock();
  int id2 = CreateCondition();
  int id3 = CreateLock();
  Write("Test1.\n", 8, ConsoleOutput);
  DestroyLock(id);
  DestroyCondition(id2);
  /*id3 = CreateLock();*/
  /*Write("Only 1 acquire should be called.\n", 36, ConsoleOutput);*/
  Acquire(id3);
 /* Write("THERE.\n", 7, ConsoleOutput);*/
  /*Acquire(id3);*/
  Release(id3);
  DestroyLock(id3);
  /*not reached*/
  /*Write("\nThis is not reached.", 21, ConsoleOutput);*/
}

void test2(){
  Write("Test2222.\n", 12, ConsoleOutput);
  lockID = CreateLock();
 /* Write("Test22.\n", 8, ConsoleOutput);*/
  Exec("../test/sort", 12);
  Fork(test2_t1);
  Acquire(lockID);
  Release(lockID);
}

void test3_t1(){
	lockID = CreateLock();
	Wait(conditionID, lockID);
	Signal(conditionID, lockID);
	Broadcast(conditionID, lockID);
	DestroyCondition(conditionID);
	Exit(0);
}

void test3_t2(){
	conditionID = CreateCondition();
	Signal(conditionID, lockID);
	Broadcast(conditionID, lockID);
	Exit(0);
}

void test4()
{
	Exec("../test/matmult", 40);
	Exit(0);
}

void test3(){
	
  
  Write("Test3.\n", 8, ConsoleOutput);
 /*Exec("../test/matmult", 40);
  Exec("../test/matmult", 40);*/
  
  Fork(test4);
  Fork(test4);
   Fork(test4);
  Fork(test4);
   Fork(test4);
  Fork(test4);
   Fork(test4);
  Fork(test4);

  Exit(0);
 /* Fork(test3_t2);*/
 /* Wait(conditionID, lockID);*/
  /*Broadcast(conditionID, lockID);*/
}



