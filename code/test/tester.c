/* tester.c
 *  Simple program to test the file handling system calls
 */

#include "syscall.h"
 
 void test();

int main() {

	test();
}

void test4()
{
	Exec("../test/matmult", 40, 0);
	Exit(0);
}

void test(){
	int i;

	for(i = 0; i < 4; i++)
	{
		Exec("../test/matmult", 40, 0);
		Yield();
	}

  Exit(0);
}



