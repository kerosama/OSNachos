// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <sstream>

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include "synch.h"
#include "addrspace.h"
#include <cmath>
#include <time.h>
#include <stdio.h>
#include <iostream>


using namespace std;

#ifdef NETWORK
#include "post.h"
#include "network.h"

PacketHeader clOutPktHdr, clInPktHdr;	
MailHeader clOutMailHdr, clInMailHdr;

char serverResponse[MaxMailSize];
char *clRequest;
#endif

struct Lock_Struct {
	Lock* lock;
};

struct Lock_Struct mainLock;


int num_processes;
class Process
{
	public:
		Process(Thread* thread, AddrSpace* space)
		{
			id = num_processes;
			num_processes++;
			
			threads[0] = thread;
			num_threads = 1;
			addrSpace = space;
			thread->space = space;
		}			

		~Process(){}

		void addThread(Thread* thread)
		{
			addrSpace->AddPages();
			threads[num_threads] = thread;
			num_threads++;
		}
	
		AddrSpace* addrSpace;
	private:
		int id;
		
		int num_threads;
		Thread* threads[50];
		//Table* pageTable;
};

//Private Variables

int num_processes_max = 50;
SpaceId current_process_num = 0;
Process *processTable[50];
int num_thr = 0; //Number of current threads (for use in exit)
int rnd = 0;
int currentTLB = 0;
int currentIPT = -1;

Lock *lock_arr[100];
bool lock_in_use[100];
bool should_delete_lock[100]; //Think this needs to be implemented to delete lock if currently being used
int current_lock_num = 0;

Condition **cond_arr = new Condition*[100];
bool cond_in_use[100];
bool should_delete_cond[100]; //Think this needs to be implemented to delete lock if currently being used
int current_cond_num = 0;

int current_monitor_num = 0;

int copyin(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes from the current thread's virtual address vaddr.
    // Return the number of bytes so read, or -1 if an error occors.
    // Errors can generally mean a bad virtual address was passed in.
    bool result;
    int n=0;			// The number of bytes copied in
    int *paddr = new int;

    while ( n >= 0 && n < len) {
      result = machine->ReadMem( vaddr, 1, paddr );
      while(!result) // FALL 09 CHANGES
	  {
   			result = machine->ReadMem( vaddr, 1, paddr ); // FALL 09 CHANGES: TO HANDLE PAGE FAULT IN THE ReadMem SYS CALL
	  }	
      
	 // printf("%d\n", *paddr);
	   //printf("%s\n", buf[n]);
      buf[n++] = *paddr;
     
      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    delete paddr;
    return len;
}

int copyout(unsigned int vaddr, int len, char *buf) {
    // Copy len bytes to the current thread's virtual address vaddr.
    // Return the number of bytes so written, or -1 if an error
    // occors.  Errors can generally mean a bad virtual address was
    // passed in.
    bool result;
    int n=0;			// The number of bytes copied in

    while ( n >= 0 && n < len) {
      // Note that we check every byte's address
      result = machine->WriteMem( vaddr, 1, (int)(buf[n++]) );

      if ( !result ) {
	//translation failed
	return -1;
      }

      vaddr++;
    }

    return n;
}

void Create_Syscall(unsigned int vaddr, int len) {
    // Create the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  No
    // way to return errors, though...
    char *buf = new char[len+1];	// Kernel buffer to put the name in

    if (!buf) return;

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Create\n");
	delete buf;
	return;
    }

    buf[len]='\0';

    fileSystem->Create(buf,0);
	num_thr++;
    delete[] buf;
    return;
}

int Open_Syscall(unsigned int vaddr, int len) {
    // Open the file with the name in the user buffer pointed to by
    // vaddr.  The file name is at most MAXFILENAME chars long.  If
    // the file is opened successfully, it is put in the address
    // space's file table and an id returned that can find the file
    // later.  If there are any errors, -1 is returned.
    char *buf = new char[len+1];	// Kernel buffer to put the name in
    OpenFile *f;			// The new open file
    int id;				// The openfile id

    if (!buf) {
	printf("%s","Can't allocate kernel buffer in Open\n");
	return -1;
    }

    if( copyin(vaddr,len,buf) == -1 ) {
	printf("%s","Bad pointer passed to Open\n");
	delete[] buf;
	return -1;
    }

    buf[len]='\0';

    f = fileSystem->Open(buf);
    delete[] buf;

    if ( f ) {
	if ((id = currentThread->space->fileTable.Put(f)) == -1 )
	    delete f;
	return id;
    }
    else
	return -1;
}

void Write_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one. For disk files, the file is looked
    // up in the current address space's open file table and used as
    // the target of the write.
    
    char *buf;		// Kernel buffer for output
    OpenFile *f;	// Open file for output

    if ( id == ConsoleInput) return;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer for write!\n");
	return;
    } else {
        if ( copyin(vaddr,len,buf) == -1 ) {
	    printf("%s","Bad pointer passed to to write: data not written\n");
	    delete[] buf;
	    return;
	}
    }

    if ( id == ConsoleOutput) {
      for (int ii=0; ii<len; ii++) {
	printf("%c",buf[ii]);
      }

    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    f->Write(buf, len);
	} else {
	    printf("%s","Bad OpenFileId passed to Write\n");
	    len = -1;
	}
    }

    delete[] buf;
}

int Read_Syscall(unsigned int vaddr, int len, int id) {
    // Write the buffer to the given disk file.  If ConsoleOutput is
    // the fileID, data goes to the synchronized console instead.  If
    // a Write arrives for the synchronized Console, and no such
    // console exists, create one.    We reuse len as the number of bytes
    // read, which is an unnessecary savings of space.
    char *buf;		// Kernel buffer for input
    OpenFile *f;	// Open file for output

    if ( id == ConsoleOutput) return -1;
    
    if ( !(buf = new char[len]) ) {
	printf("%s","Error allocating kernel buffer in Read\n");
	return -1;
    }

    if ( id == ConsoleInput) {
      //Reading from the keyboard
      scanf("%s", buf);

      if ( copyout(vaddr, len, buf) == -1 ) {
	printf("%s","Bad pointer passed to Read: data not copied\n");
      }
    } else {
	if ( (f = (OpenFile *) currentThread->space->fileTable.Get(id)) ) {
	    len = f->Read(buf, len);
	    if ( len > 0 ) {
	        //Read something from the file. Put into user's address space
  	        if ( copyout(vaddr, len, buf) == -1 ) {
		    printf("%s","Bad pointer passed to Read: data not copied\n");
		}
	    }
	} else {
	    printf("%s","Bad OpenFileId passed to Read\n");
	    len = -1;
	}
    }

    delete[] buf;
    return len;
}

void Close_Syscall(int fd) {
    // Close the file associated with id fd.  No error reporting.
    OpenFile *f = (OpenFile *) currentThread->space->fileTable.Remove(fd);

    if ( f ) {
      delete f;
    } else {
      printf("%s","Tried to close an unopen file\n");
    }
}


void Exit_Syscall(int status) {
	// If there are other threads, finish the thread, otherwise call halt
	// and stop the user program.
	printf("num_thr: %d\n", num_thr);
	printf("Output: %d\n", status);
	if (num_thr > 0) {
		num_thr--;

		//invalidate ipt entries for this space
		iptLock->Acquire("");
		for(int i = 0; i < NumPhysPages; i++)
		{
			if(mIPT->ipTable[i].owner == currentThread->space)
			{
				mIPT->ipTable[i].valid = false;
			}
		}
		iptLock->Release("");

		currentThread->Finish();		
	}
	else
	{
		//clear the swapfile
		char buf[PageSize];
		memset(buf, ' ', PageSize);
		for(int i = 0; i < SWAP_SIZE; i++)
		{			
			swapFile->WriteAt(buf, PageSize, i*PageSize);
		}
		//end program
		interrupt->Halt();
	}
}

void exec_thread(int arg) {
	//printf("running thread - exec.\n");

	currentThread->space->InitRegisters();		// set the initial register values
	currentThread->space->RestoreState();		// load page table register
	machine->Run();			// jump to the user progam
}

SpaceId Exec_Syscall(char *name) {
	/* Exec - Takes in name of process and returns the id of the space for the process.
	*  The process table is updated with the space and a new exec thread is forked.
	*  Function is based on progtest.cc's StartProcess(char *) function.
	*/
	//printf("inside exec syscall\n");

	OpenFile *executable = fileSystem->Open(name);
	if (executable == NULL) {
		printf("Unable to open file %s\n", name);
		return -1;
	}

	AddrSpace* space = new AddrSpace(executable);

	Thread* t = new Thread("exec thread");
	num_thr++;
	Process* p = new Process(t, space);
	processTable[num_processes] = p;
	//printf("num processes: %d\n", num_processes);
	t->Fork(exec_thread, 0);

	//printf("finished exec syscall\n");
	return num_processes - 1;
}

void kernel_thread(int va)
{
	machine->WriteRegister(PCReg, va);
	machine->WriteRegister(NextPCReg, va + 4);
	currentThread->space->RestoreState();
	//printf("%d\n", currentThread->space->numPages);
	machine->WriteRegister(StackReg, currentThread->space->numPages * PageSize - 16);
	machine->Run();
}

void Fork_Syscall(int va)
{
	//printf("inside fork syscall\n");
	Thread* kernelThread = new Thread("kernel thread");	
	num_thr++;	
	kernelThread->space = currentThread->space;
	if (processTable[current_process_num] == NULL)
	{
		processTable[current_process_num] = new Process(currentThread, currentThread->space);
		
	}
	//printf("%d-", currentThread->space->numPages);
	processTable[current_process_num]->addThread(kernelThread);
	//printf("%d\n", currentThread->space->numPages);
	//printf("%d %d", (int)*func, machine->ReadRegister(4));
	printf("finished fork syscall\n");
	//printf("va: %d\n", va);
	kernelThread->Fork(kernel_thread, va);


}

#ifdef NETWORK

int MsgSentToServer() {
	clOutPktHdr.to = 0;
	clOutPktHdr.from = net_name;
	clOutMailHdr.to = 0;
	clOutMailHdr.from = net_name;

	clOutMailHdr.length = strlen(clRequest) + 1;

	bool success = postOffice->Send(clOutPktHdr, clOutMailHdr, clRequest);

	if (!success) {
		printf("The postOfficeClient Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
		interrupt->Halt();
		return -1;
	}
	else {
		printf("Msg Sent!");
		return 0;
	}
}

//MESSAGE RECEIVED BY CLIENT FROM SERVER
void MsgRcvedFromServer() {
	printf("\n MESSAGE RECEIVED \n");
	postOffice->Receive(net_name, &clInPktHdr, &clInMailHdr, serverResponse);

	printf("Got \"%s\" from %d, box %d\n", clRequest, clInPktHdr.from, clInMailHdr.from);
	fflush(stdout);
}

void CreateMessage(char* request, char* name)
{
	char buffer[50];

	//CREATE MESSAGE IN PARTICULAR FORMAT TO BE SENT OVER TO SERVER 
	sprintf(buffer, "%s %s", request, name);
	int length = strlen(buffer);
	clRequest = new char[length]; 
	strcpy(clRequest,buffer); //CREATING CLIENT REQUEST
}

int CreateLock_Syscall() {
	printf("Syscall in Exception.cc - Creating Lock\n");
	char name[2];
	sprintf(name, "%d", current_lock_num++);
	CreateMessage("CreateLock", name);

	//REQUEST CREATE LOCK TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return -1;
	}
	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();

	int requestStatus = atoi (serverResponse);

	printf("CreateLock-Received Msg: %s\n", serverResponse);
	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;

	if (atoi(temp.c_str()) < 0)
		printf("Server out of locks.\n");

	//REQUEST FAILED If -1
	if(requestStatus == -1){
		printf("\n FAILURE TO CREATE LOCK\n");
		return -1;
	}

	return atoi(temp.c_str());
	return requestStatus;
	
	/*Lock *temp = new Lock(name);
	lock_arr[current_lock_num] = temp;
	lock_in_use[current_lock_num] = false;
	should_delete_lock[current_lock_num] = false;
	current_lock_num++;

	printf("lock name: %s\n", lock_arr[current_lock_num-1]->getName());
	return (current_lock_num - 1);
	//return -1;*/
}

void DestroyLock_Syscall(int id) {
	printf("Syscall in Exception.cc - Destroying Lock\n");
	char name[2]; //LOCK NAME
	sprintf(name, "%d", id);
	CreateMessage("DestroyLock", name);

	//REQUEST DESTROY LOCK TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("DestroyLock-Received Msg: %s\n", serverResponse);
	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Error: Destroy Lock Syscall - id does not exist.\n");

	/*if (id > current_lock_num || id < 0) {
	printf("Error: Destroy Lock Syscall - id does not exist.\n");
	return;
	}
	if (lock_in_use[id])
	should_delete_lock[id] = true;
	else
	delete lock_arr[id];*/
}

void Acquire_Syscall(int id) {
	printf("Syscall in Exception.cc - Acquiring Lock\n");
	char name[2];
	sprintf(name, "%d", id);
	CreateMessage("AcquireLock", name);

	//REQUEST ACQUIRE LOCK TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();

	printf("AcquireLock-Received Msg: %s\n", serverResponse);
	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Error: Acquire Lock Syscall - id does not exist.\n");
	/*if (id > current_lock_num || id < 0) 
	{
		printf("Error: Acquire Lock Syscall - id does not exist.\n");
		return;
	}
	lock_arr[id]->Acquire("except");
	lock_in_use[id] = true;*/
}

void Release_Syscall(int id) {
	printf("Syscall in Exception.cc - Releasing Lock\n");
	char name[2];
	sprintf(name, "%d", id);


	CreateMessage("ReleaseLock", name);

	//REQUEST RELEASE LOCK TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();

	printf("ReleaseLock-Received Msg: %s\n", serverResponse);
	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Error: Release Lock Syscall - id does not exist.\n");
	/*if (id > current_lock_num || id < 0) {
	printf("Error: Release Lock Syscall - id does not exist.\n");
	return;
	}
	lock_in_use[id] = false;
	lock_arr[id]->Release("except");
	printf("2\n");
	if (should_delete_lock[id])
	delete lock_arr[id];*/
}

int CreateCondition_Syscall() {
	printf("Syscall in Exception.cc - Creating CV\n");
	char name[2];
	sprintf(name, "%d", current_cond_num++);
	CreateMessage("CreateCV", name);

	//REQUEST CREATE CV TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return -1;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("CreateCV-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Server out of CVs.\n");

	return atoi(temp.c_str());

	int requestStatus = atoi (serverResponse);
	if(requestStatus == -1){
	printf("\n FAILURE TO CREATE LOCK\n");
	return -1;
	}


	/*Condition *tempCond = new Condition("temp");
	cond_arr[current_cond_num++] = tempCond;
	return current_cond_num - 1;*/
}

void DestroyCondition_Syscall(int id) {
	printf("Syscall in Exception.cc - Destroying CV\n");
	char name[2];
	sprintf(name, "%d", id);
	CreateMessage("DestroyCV", name);

	//REQUEST DESTROY CV TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("DestroyCV-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Error: Destroy Condition Syscall - id does not exist.\n");

	/*if (id > current_cond_num || id < 0) 
	{
		printf("Error: Destroy Condition Syscall - id does not exist.\n");
		return;
	}
	if (cond_in_use[id])
		should_delete_cond[id] = true;
	else
		delete cond_arr[id];*/
}

void Signal_Syscall(int id, int lock_id) {
	printf("Syscall in Exception.cc - Signalling CV\n");
	char ids[5];
	sprintf(ids, "%d %d", id, lock_id);

	CreateMessage("SignalCV", ids);

	//REQUEST SIGNAL TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("Signal-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Error: Signal Condition Syscall - id or lock id does not exist.\n");
	if (id > current_cond_num || id < 0) {
	printf("Error: Signal Condition Syscall - id does not exist.\n");
	return;
	}
	
	/*if (lock_id > current_lock_num || lock_id < 0) {
	printf("Error: Signal Condition Syscall - lock id does not exist.\n");
	return;
	}
	if (should_delete_cond[id])
		delete cond_arr[id];
	else
		cond_arr[id]->Signal("", lock_arr[lock_id]);*/
}

void Wait_Syscall(int id, int lock_id) {
	printf("Syscall in Exception.cc - Waiting CV\n");
	char ids[5]; //LOCK NAME
	sprintf(ids, "%d %d", id, lock_id);
	CreateMessage("WaitCV", ids);

	//REQUEST WAIT TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("Wait-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;

	if (atoi(temp.c_str()) < 0)
		printf("Error: Wait Condition Syscall - id or lock id does not exist.\n");
	/*if (id > current_cond_num || id < 0) {
	printf("Error: Wait Condition Syscall - id does not exist.\n");
	return;
	}
	if (lock_id > current_lock_num || lock_id < 0) {
	printf("Error: Wait Condition Syscall - lock id does not exist.\n");
	return;
	}
	//if (should_delete_cond[id])
	//delete cond_arr[id];
	//else
	cond_arr[id]->Wait("", lock_arr[lock_id]);*/
}

void Broadcast_Syscall(int id, int lock_id) {
	printf("Syscall in Exception.cc - Broadcast CV\n");
	char ids[5];
	sprintf(ids, "%d %d", id, lock_id);
	CreateMessage("BroadcastCV", ids);

	//REQUEST BROADCAST TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("Broadcast-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Error: Broadcast Condition Syscall - id or lock id does not exist.\n");
	/*if (id > current_cond_num || id < 0) {
	printf("Error: Broadcast Condition Syscall - id does not exist.\n");
	return;
	}
	if (lock_id > current_lock_num || lock_id < 0) {
	printf("Error: Broadcast Condition Syscall - lock id does not exist.\n");
	return;
	}
	//if (should_delete_cond[id])
	//delete cond_arr[id];
	//else
	cond_arr[id]->Broadcast(lock_arr[lock_id]);*/
}

// Creates a new monitor (sent to server)
// returns the index (id) of the monitor
int CreateMonitor_Syscall()
{
	printf("Syscall in Exception.cc - Creating Monitor\n");
	//Lock *tempLock = new Lock("temp");
	char name[2]; //LOCK NAME
	sprintf(name, "%d", current_monitor_num++);
	//strcat(name, " lock");

	CreateMessage("CreateMonitor", name);

	//REQUEST CREATE MONITOR TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return -1;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("CreateMonitor-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Server out of Monitors.\n");

	return atoi(temp.c_str());
}

// Destroys a monitor at the specified id
void DestroyMonitor_Syscall(int id)
{
	printf("Syscall in Exception.cc - Destroying Monitor\n");
	char name[2];
	sprintf(name, "%d", id);

	CreateMessage("DestroyMonitor", name);

	//REQUEST DESTROY MONITOR TO SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("DestroyMonitor-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Monitor does not exist on Server.\n");
}

// Gets the variable from monitor id
// returns the value of the monitor
int GetMonitorVal_Syscall(int id)
{
	printf("Syscall in Exception.cc - Getting Monitor Value\n");
	char name[2];
	sprintf(name, "%d", id);

	CreateMessage("GetMonitorVal", name);

	//REQUEST GET MONITOR VAL FROM SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return -1;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("GetMonitorVal-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Monitor does not exist on Server.\n");

	return atoi(temp.c_str());
}

// Sets the monitor variable to val at monitor id
void SetMonitorVal_Syscall(int id, int val)
{
	printf("Syscall in Exception.cc - Setting Monitor Value\n");
	char name[5];
	sprintf(name, "%d %d", id, val);

	CreateMessage("SetMonitorVal", name);

	//REQUEST SET MONITOR VALUE ON SERVER
	int successful = MsgSentToServer();

	if (successful == -1){
		printf("FAILURE TO PROPERLY SEND REQUEST TO SERVER\n");
		return;
	}

	//WAIT FOR RESPONSE	
	MsgRcvedFromServer();
	printf("SetMonitorVal-Received Msg: %s\n", serverResponse);

	string temp;
	stringstream ss;
	ss << serverResponse;
	ss >> temp;
	if (atoi(temp.c_str()) < 0)
		printf("Monitor does not exist on Server.\n");
}

#endif //END #IFDEF NETWORK

int Rand_Syscall(int mod) {
	srand(time(NULL));
	rnd = rand() % mod;
	return rnd;
}

void IntPrint_Syscall(int i)
{
	printf("%d", i);
}

void WriteToSwap(int epn)
{
	iptLock->Acquire("");
	currentThread->space->PTLock->Acquire("");
	for(int j = 0; j < mIPT->ipTable[epn].owner->numPages; j++)
	{
		
		if(currentThread->space->PageTable[j].physicalPage == epn)
		{
			if(currentThread->space->PageTable[j].diskLocation > -1)
			{
				int swapPage;
				if(mIPT->ipTable[epn].owner->PageTable[j].diskLocation != 1)
				{
					swapPage = swapMap->Find();
					if(swapPage == -1)
					{
						printf("Error: swapfile full\n");
						return;
					}
				
					mIPT->ipTable[epn].owner->PageTable[j].diskLocation = 1;
					swapPage = swapPage*PageSize;
					mIPT->ipTable[epn].owner->PageTable[j].byteOffset = swapPage;

				}
				else
				{
					swapPage = mIPT->ipTable[epn].owner->PageTable[j].byteOffset;
				}
			
				memoryLock->Acquire("");
				swapFile->WriteAt(&machine->mainMemory[epn*PageSize], PageSize, swapPage);
				memoryLock->Release("");	

				printf("added physical page %d from virtual page %d to swapfile page %d\n", epn, j, swapPage/PageSize);
			}
			break;
		}
	}	
	iptLock->Release("");
	currentThread->space->PTLock->Release("");
}

int handleMemoryFull(int neededVPN)
{
/*#ifdef NETWORK
	isFIFO = true;
#else
	isFIFO = false;
#endif*/
	int evictedPageNum = -1;
	//isFIFO = true;
	//If isFIFO is true, use FIFO to evict page. Otherwise, use random page selection.
	if(isFIFO)
	{
		//evict the page added the earliest, which is stored in currentIPT
		evictedPageNum = (currentIPT + 1) % NumPhysPages;
		currentIPT = evictedPageNum;
		//printf("currentIPT: %d\n", currentIPT);
	}
	else
	{
		//randomly determine an IPT page to evict
		do 
		{
			srand(time(NULL));
			evictedPageNum = rand() % NumPhysPages;
		}while(evictedPageNum == currentIPT);
		currentIPT = evictedPageNum;
	}

	mIPT->ipTable[evictedPageNum].valid = false;

	iptLock->Acquire("");
	if(mIPT->ipTable[evictedPageNum].dirty == true)
	{
		iptLock->Release("");
		WriteToSwap(evictedPageNum);
	}
	else
	{
		iptLock->Release("");
		
		//Check if evicted page already exists in TLB
		//If it is in TLB, check if the page is dirty
		//If the page is dirty, find the physical page in the current process pagetable
		//If the page exists in the pagetable, write the dirty page to an available spot in the swapfile, using the physical page number
		
		//Need to disable interrupts for TLB access				  
		IntStatus oldLevel = interrupt->SetLevel(IntOff); 
		for(int i = 0; i < TLBSize; i++)
		{			
			if(machine->tlb[i].physicalPage == evictedPageNum)
			{
				if(machine->tlb[i].dirty == true)
				{
					WriteToSwap(evictedPageNum);

					break;
				}
			}
		}	
		(void) interrupt->SetLevel(oldLevel);
		
	}

	//need to give IPT page to replace
	return evictedPageNum;
}

int handleIPTMiss(int neededVPN)
{	
	int ppn = mmBitMap->Find();
	if(ppn == -1)
	{
		ppn = handleMemoryFull(neededVPN);
	}	

	//Get location of virtual page from current space pagetable
	//Get data from virtual page from swap file or executable
	currentThread->space->PTLock->Acquire("");
	for(int j = 0; j < currentThread->space->numPages; j++)
	{
		if(currentThread->space->PageTable[j].virtualPage == neededVPN)
		{
			if(currentThread->space->PageTable[j].diskLocation == 1)
			{
				memoryLock->Acquire("");
				
				int pg = currentThread->space->PageTable[j].byteOffset/PageSize;
				printf("Reading from swapfile page %d to main memory...\n", pg);
				swapFile->ReadAt(&machine->mainMemory[ppn*PageSize], 
					PageSize, currentThread->space->PageTable[j].byteOffset);

				//clear the page in swapfile bitmap
				swapMap->Clear(currentThread->space->PageTable[j].byteOffset/PageSize);

				//came from swap file, so it's dirty
				mIPT->ipTable[ppn].dirty = true;

				//no longer in swapfile or executable
				//currentThread->space->PageTable[j].diskLocation = 2;
				memoryLock->Release("");
			}
			else if(currentThread->space->PageTable[j].diskLocation <= 0)
			{
				memoryLock->Acquire("");
				currentThread->space->spaceExec->ReadAt(&machine->mainMemory[ppn*PageSize], 
					PageSize, currentThread->space->PageTable[j].byteOffset);
				printf("Reading from executable virtual page %d to main memory...\n", currentThread->space->PageTable[j].virtualPage);
				/*if(currentThread->space->PageTable[j].diskLocation == 0)
				{
					currentThread->space->PageTable[j].diskLocation = 2;
				}*/
				
				mIPT->ipTable[ppn].dirty = false;
				memoryLock->Release("");
			}

			currentThread->space->PageTable[j].physicalPage = ppn;
			
			iptLock->Acquire("");
			mIPT->ipTable[ppn].owner = currentThread->space;
			mIPT->ipTable[ppn].virtualPage = neededVPN;
			mIPT->ipTable[ppn].valid = true;
			
			iptLock->Release("");
			break;
		}
	}	
	currentThread->space->PTLock->Release("");	

	return ppn;
}

void handlePageFault()
{	
	int neededVA = machine->ReadRegister(BadVAddrReg);
	int neededVPN = neededVA / PageSize;

	printf("Needed VPN: %d\n", neededVPN);

	int ppn = -1;
		
	iptLock->Acquire("");
	for(int i = 0; i < NumPhysPages; i++)
	{
		if(mIPT->ipTable[i].virtualPage == neededVPN)
		{
			if(mIPT->ipTable[i].owner == currentThread->space)
			{
				if(mIPT->ipTable[i].valid == true)
				{
					//IPT hit
					//wanted physical page is IPT index number
					printf("IPT hit for virtual page %d\n", neededVPN);
					ppn = i;
					break;
				}
			}
		}
	}
	iptLock->Release("");

	//IPT miss
	if(ppn == -1)
	{		
		ppn = handleIPTMiss(neededVPN);
	}
	
	
	//Last step: update TLB
	//Need to disable interrupts	
	IntStatus oldLevel = interrupt->SetLevel(IntOff); 
	if(ppn != -1)
	{
		//transfer dirty bits from replaced tlb page if still valid
		for(int i = 0; i < NumPhysPages; i++)
		{
			if(machine->tlb[currentTLB].valid == true /*&& machine->tlb[currentTLB].dirty == true*/ && mIPT->ipTable[i].virtualPage == machine->tlb[currentTLB].virtualPage)
			{
				printf("Transferred dirty bit from tlb page %d to IPT page %d (VPN: %d)\n", currentTLB, i, machine->tlb[currentTLB].virtualPage); 
				mIPT->ipTable[i].dirty = machine->tlb[currentTLB].dirty;
			}
		}
	
		machine->tlb[currentTLB].physicalPage = ppn;
		machine->tlb[currentTLB].virtualPage = neededVPN;
		machine->tlb[currentTLB].valid = true;
		machine->tlb[currentTLB].dirty = mIPT->ipTable[ppn].dirty;	
		
	}
	else
	{
		printf("Error: could not provide physical page to TLB");
	}

	currentTLB = (currentTLB + 1) % TLBSize; //Increment TLB page number for FIFO
	(void) interrupt->SetLevel(oldLevel);
}

void ExceptionHandler(ExceptionType which) {
    
    int type = machine->ReadRegister(2); // Which syscall?
    if ( which == SyscallException ) 
	{
		
		int rv=0; 	// the return value from a syscall

		switch (type) 
		{
			default:
			DEBUG('a', "Unknown syscall - shutting down.\n");
			case SC_Halt:
			DEBUG('a', "Shutdown, initiated by user program.\n");
			interrupt->Halt();
			break;
			case SC_Create:
			DEBUG('a', "Create syscall.\n");
			Create_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_Open:
			DEBUG('a', "Open syscall.\n");
			rv = Open_Syscall(machine->ReadRegister(4), machine->ReadRegister(5));
			break;
			case SC_Write:
			DEBUG('a', "Write syscall.\n");
			Write_Syscall(machine->ReadRegister(4),
					  machine->ReadRegister(5),
					  machine->ReadRegister(6));
			break;
			case SC_Read:
			DEBUG('a', "Read syscall.\n");
			rv = Read_Syscall(machine->ReadRegister(4),
					  machine->ReadRegister(5),
					  machine->ReadRegister(6));
			break;
			case SC_Close:
			DEBUG('a', "Close syscall.\n");
			Close_Syscall(machine->ReadRegister(4));
			break;
			case SC_Yield:
			DEBUG('a', "Yield syscall.\n");
			currentThread->Yield();
			break;
			case SC_Exit:
			DEBUG('a', "Exit syscall.\n");
			Exit_Syscall(machine->ReadRegister(4));
			break;
			case SC_Exec:
			DEBUG('a', "Exec syscall.\n");
			char* data = new char[machine->ReadRegister(5)];
			int x = copyin(machine->ReadRegister(4), machine->ReadRegister(5), data);
			rv = Exec_Syscall(data);
			break;
			case SC_Fork:
			DEBUG('a', "Fork syscall.\n");
			Fork_Syscall(machine->ReadRegister(4));
			break;
#ifdef NETWORK
			case SC_CreateLock:
			DEBUG('a', "CreateLock syscall.\n");
			rv = CreateLock_Syscall();
			break;
			case SC_DestroyLock:
			DEBUG('a', "DestroyLock syscall.\n");
			DestroyLock_Syscall(machine->ReadRegister(4));
			break;
			case SC_Acquire:
			DEBUG('a', "Acquire syscall.\n");
			Acquire_Syscall(machine->ReadRegister(4));
			break;
			case SC_Release:
			DEBUG('a', "Release syscall.\n");
			Release_Syscall(machine->ReadRegister(4));
			break;
			case SC_CreateCondition:
			DEBUG('a', "CreateCondition syscall.\n");
			rv = CreateCondition_Syscall();
			break;
			case SC_DestroyCondition:
			DEBUG('a', "DestroyCondition syscall.\n");
			DestroyCondition_Syscall(machine->ReadRegister(4));
			break;
			case SC_Signal:
			DEBUG('a', "Signal syscall.\n");
			Signal_Syscall(machine->ReadRegister(4), 
					machine->ReadRegister(5));
			break;
			case SC_Wait:
			DEBUG('a', "Wait syscall.\n");
			Wait_Syscall(machine->ReadRegister(4), 
					machine->ReadRegister(5));
			break;
			case SC_Broadcast:
			DEBUG('a', "Broadcast syscall.\n");
			Broadcast_Syscall(machine->ReadRegister(4),
					machine->ReadRegister(5));
			break;
			case SC_CreateMonitor:
			DEBUG('a', "CreateMonitor syscall.\n");
			rv = CreateMonitor_Syscall();
			break;
			case SC_DestroyMonitor:
			DEBUG('a', "DestroyMonitor syscall.\n");
			DestroyMonitor_Syscall(machine->ReadRegister(4));
			break;
			case SC_GetMonitorVal:
			DEBUG('a', "GetMonitorVal syscall.\n");
			rv = GetMonitorVal_Syscall(machine->ReadRegister(4));
			break;
			case SC_SetMonitorVal:
			DEBUG('a', "SetMonitorVal syscall.\n");
			SetMonitorVal_Syscall(machine->ReadRegister(4),
				machine->ReadRegister(5));
			break;
#endif //NETWORK
			case SC_Rand:
			DEBUG('a', "Rand syscall.\n");
			rv = Rand_Syscall(machine->ReadRegister(4));
			break;
			case SC_IntPrint:
			DEBUG('a', "IntPrint syscall.\n");
			IntPrint_Syscall(machine->ReadRegister(4));
			break;
		}

		// Put in the return value and increment the PC
		machine->WriteRegister(2,rv);
		machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
		machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
		machine->WriteRegister(NextPCReg, machine->ReadRegister(PCReg)+4);
		return;
    } 
	else if(which == PageFaultException) 
	{
		handlePageFault();
		return;
	} 
	else 
	{
      cout<<"Unexpected user mode exception - which:"<<which<<"  type:"<< type<<endl;
      interrupt->Halt();
    }
}
