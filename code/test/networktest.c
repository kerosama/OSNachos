/* tester.c
 *  Simple program to test the file handling system calls
 */

#include "syscall.h"

 int lockID;
 int conditionID;
 int t1_l1;

 int t2_l1;		/* For mutual exclusion*/
 int t2_c1;	/*The condition variable to test*/

 int t3_l1;
 int t3_c1;

 int t4_l1;
 int t4_c1;

 int t5_l1;
 int t5_l2;
 int t5_c1;

 int t6_m1;
 int t6_m2;
 
 void TestSuite();

int main() {
	TestSuite();
}


/* --------------------------------------------------
// Test Suite
// --------------------------------------------------

// --------------------------------------------------
// t1_t1() -- test1 thread 1
//     This is the rightful lock owner
// --------------------------------------------------*/
void t1_t1() {
	int i;

	/*t1_l1 = CreateLock();*/
	Acquire(t1_l1);

	Write("t1_t1 Acquired Lock.\n", 22, ConsoleOutput);
	for(i = 0; i < 100000; i++);
	Write("t1_t1 Releasing Lock.\n", 22, ConsoleOutput);

	Release(t1_l1);

	Exit(0);
}

/* --------------------------------------------------
// t1_t2() -- test1 thread 2
//     This thread will wait on the held lock.
// --------------------------------------------------*/
void t1_t2() {
	int i;
	Write("t1_t2 Trying to Acquire Lock.\n", 31, ConsoleOutput);
	Acquire(t1_l1);
	Write("t1_t2 Acquired Lock.\n", 22, ConsoleOutput);

	for (i = 0; i < 10; i++);

	Write("t1_t2 Releasing Lock.\n", 22, ConsoleOutput);
	Release(t1_l1);

	Exit(0);
}

/* --------------------------------------------------
// t1_t3() -- test1 thread 3
//     This thread will try to release the lock illegally
// --------------------------------------------------*/
void t1_t3() {
	int i;
	for (i = 0; i < 3; i++) {
		Write("t1_t3 Trying to Release Lock.\n", 33, ConsoleOutput);
		Release(t1_l1);
	}
	Exit(0);
}

/* --------------------------------------------------
// Test 2 - see TestSuite() for details
// --------------------------------------------------

// --------------------------------------------------
// t2_t1() -- test 2 thread 1
//     This thread will signal a variable with nothing waiting
// --------------------------------------------------*/
void t2_t1() {
	Acquire(t2_l1);
	Write("t2_t1 Acquired Lock.\n", 22, ConsoleOutput);
	Signal(t2_c1, t2_l1);
	Write("t2_t1 Releasing Lock.\n", 22, ConsoleOutput);
	Release(t2_l1);

	Exit(0);
}

/* --------------------------------------------------
// t2_t2() -- test 2 thread 2
//     This thread will wait on a pre-signalled variable
// --------------------------------------------------*/
void t2_t2() {
	Acquire(t2_l1);
	Write("t2_t2 Acquired Lock.\n", 22, ConsoleOutput);
	Wait(t2_c1, t2_l1);
	Write("t2_t2 Releasing Lock.\n", 22, ConsoleOutput);
	Release(t2_l1);

	Exit(0);
}
/* --------------------------------------------------
// Test 3 - see TestSuite() for details
// --------------------------------------------------

// --------------------------------------------------
// t3_waiter()
//     These threads will wait on the t3_c1 condition variable.  Only
//     one t3_waiter will be released
// --------------------------------------------------*/
void t3_waiter() {
	Acquire(t3_l1);
	Write("t3_waiter Acquired Lock.\n", 27, ConsoleOutput);
	Wait(t3_c1, t3_l1);
	Write("t3_waiter Freed.\n", 17, ConsoleOutput);
	Release(t3_l1);

	Exit(0);
}

/* --------------------------------------------------
// t3_signaller()
//     This threads will signal the t3_c1 condition variable.  Only
//     one t3_signaller will be released
// --------------------------------------------------*/
void t3_signaller() {

	/* Don't signal until someone's waiting*/
	int i;
	for (i = 0; i < 5; i++);

	Acquire(t3_l1);
	Write("t3_signaller Acquired Lock.\n", 28, ConsoleOutput);
	Signal(t3_c1, t3_l1);
	Write("t3_signaller called Signal.\n", 30, ConsoleOutput);
	Release(t3_l1);
	Exit(0);
}

/* --------------------------------------------------
// Test 4 - see TestSuite() for details
// --------------------------------------------------

// --------------------------------------------------
// t4_waiter()
//     These threads will wait on the t4_c1 condition variable.  All
//     t4_waiters will be released
// --------------------------------------------------*/
void t4_waiter() {
	Acquire(t4_l1);
	Write("t4_waiter Acquired Lock.\n", 27, ConsoleOutput);
	Wait(t4_c1, t4_l1);
	Write("t4_waiter Freed.\n", 17, ConsoleOutput);
	Release(t4_l1);

	Exit(0);
}


/* --------------------------------------------------
// t2_signaller()
//     This thread will broadcast to the t4_c1 condition variable.
//     All t4_waiters will be released
// --------------------------------------------------*/
void t4_signaller() {
	int i;
	/* Don't broadcast until someone's waiting*/
	for (i = 0; i < 5; i++);

	Acquire(t4_l1);
	Write("t4_signaller Acquired Lock.\n", 28, ConsoleOutput);
	Broadcast(t4_c1, t4_l1);
	Write("t4_signaller called Broadcast.\n", 35, ConsoleOutput);
	Release(t4_l1);

	Exit(0);
}
/* --------------------------------------------------
// Test 5 - see TestSuite() for details
// --------------------------------------------------
Lock t5_l1("t5_l1");		// For mutual exclusion
Lock t5_l2("t5_l2");		// Second lock for the bad behavior
Condition t5_c1("t5_c1");	// The condition variable to test
Semaphore t5_s1("t5_s1", 0);	// To make sure t5_t2 acquires the lock after
// t5_t1

// --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a condition under t5_l1
// --------------------------------------------------*/
void t5_t1() {
	Acquire(t5_l1);
	Write("t5_t1 Acquired Lock.\n", 22, ConsoleOutput);
	Wait(t5_c1, t5_l1);
	Write("t5_t1 Releasing Lock.\n", 22, ConsoleOutput);
	Release(t5_l1);

	Exit(0);
}

/* --------------------------------------------------
// t5_t1() -- test 5 thread 1
//     This thread will wait on a t5_c1 condition under t5_l2, which is
//     a Fatal error
// --------------------------------------------------*/
void t5_t2() {
	Acquire(t5_l1);
	Acquire(t5_l2);
	Write("t5_t2 Acquired Lock.\n", 22, ConsoleOutput);
	Signal(t5_c1, t5_l2);
	Write("t5_t2 Releasing Lock2.\n", 26, ConsoleOutput);
	Release(t5_l2);
	Write("t5_t2 Releasing Lock1.\n", 26, ConsoleOutput);
	Release(t5_l1);
	Exit(0);
}

/* Test 6 - Monitors*/

void t6_t1()
{
	int fail_monitor = -1;
	int value;

	Write("Starting Test6.\n", 22, ConsoleOutput);

	SetMonitorVal(t6_m1, 5);
	value = GetMonitorVal(t6_m1);

	Write("Monitor Value: ", 16, ConsoleOutput);
	IntPrint(value);

	DestroyMonitor(t6_m1);

	Write("Setting monitor - Should fail.\n", 32, ConsoleOutput);
	SetMonitorVal(fail_monitor, -1);
	Write("Getting monitor - Should fail.\n", 32, ConsoleOutput);
	GetMonitorVal(fail_monitor);
	Write("Destroying monitor - Should fail.\n", 35, ConsoleOutput);
	DestroyMonitor(fail_monitor);

	Exit(0);
}

/* --------------------------------------------------
// TestSuite()
//     This is the main thread of the test suite.  It runs the
//     following tests:
//
//       1.  Show that a thread trying to release a lock it does not
//       hold does not work
//
//       2.  Show that Signals are not stored -- a Signal with no
//       thread waiting is ignored
//
//       3.  Show that Signal only wakes 1 thread
//
//	 4.  Show that Broadcast wakes all waiting threads
//
//       5.  Show that Signalling a thread waiting under one lock
//       while holding another is a Fatal error
//
//     Fatal errors terminate the thread in question.
// --------------------------------------------------*/
void TestSuite() {
	int i;

	t1_l1 = CreateLock();
	t2_l1 = CreateLock();
	t3_l1 = CreateLock();
	t3_c1 = CreateCondition();	

	/*Test 1*/
	
	Write("Starting Test1.\n", 22, ConsoleOutput);

	/*Uncomment for Lock test*/
	/*Acquire(t1_l1);

	Write("t1_t1 Acquired Lock.\n", 22, ConsoleOutput);
	for (i = 0; i < 300000; i++);
	Write("t1_t1 Releasing Lock.\n", 22, ConsoleOutput);

	Release(t1_l1);

	Exit(0);*/

	/*Uncomment for CV Test (Comment above)*/
	Acquire(t3_l1);
	Write("t3_waiter Acquired Lock.\n", 27, ConsoleOutput);
	Wait(t3_c1, t3_l1);
	Write("t3_waiter Freed.\n", 17, ConsoleOutput);
	Release(t3_l1);

	Exit(0);
}