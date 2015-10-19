/* tester.c
 *  Simple program to test the file handling system calls
 */

#include "syscall.h"

 int lock;
 int condition;

 void test1();
 void test2();
 void test3();

int main() {
  OpenFileId fd;
  int bytesread;
  char buf[20];

    /*Create("testfile", 8);
    fd = Open("testfile", 8);

    Write("testing a write\n", 16, fd );
    Close(fd);


    fd = Open("testfile", 8);
    bytesread = Read( buf, 100, fd );
    Write( buf, bytesread, ConsoleOutput );
    Close(fd);*/

    test1();
    test2();
    test3();
}

void test1(){
  int id = CreateLock();
  int id2 = CreateCondition();
  int id3;
  DestroyLock(id);
  DestroyCondition(id2);
  id3 = CreateLock();
  Write("\nOnly 1 acquire should be called.\n", 36, ConsoleOutput);
  Acquire(id3);
  Acquire(id3);
  Release(id3);
  /*not reached*/
  /*Write("\nThis is not reached.", 21, ConsoleOutput);*/
}

void test2(){
  lockID = CreateLock();
  Fork(test2_t1);
  Acquire(lock);
  Release(lock);
}

void test2_t1(){
  Acquire(lock);
  DestroyLock(lock);
  Exit(0);
}

void test2_t2(){
  lockID = CreateLock();
  Yield();
  Release(lock);
  Exit(0);
}

void test3(){
  conditionID = CreateCondition();
  lockID = CreateLock();
  Fork(test3_t1);
  Fork(test3_t2);
  Wait(condition, lock);
  Broadcast(condition, lock);
}

void test3_t1(){
  lockID = CreateLock();
  Wait(condition, lock);
  Signal(condition, lock);
  Broadcast(condition, lock);
  DestroyCondition(condition);
  Exit(0);
}

void test3_t2(){
  condition = CreateCondition();
  Signal(condition, lock);
  Broadcast(condition, lock);
  Exit(0);
}
