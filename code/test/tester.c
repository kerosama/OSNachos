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
	Exec("../test/matmult", 40);
	Exit(0);
}

void test(){
	
  
  Fork(test4);
  Exec("../test/matmult", 40);
   Fork(test4);
  Exec("../test/matmult", 40);
   Fork(test4);
  Exec("../test/matmult", 40);
   Fork(test4);
  Exec("../test/matmult", 40);

  Exit(0);
}



