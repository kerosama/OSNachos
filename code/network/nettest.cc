// nettest.cc 
//	Test out message delivery between two "Nachos" machines,
//	using the Post Office to coordinate delivery.
//
//	Two caveats:
//	  1. Two copies of Nachos must be running, with machine ID's 0 and 1:
//		./nachos -m 0 -o 1 &
//		./nachos -m 1 -o 0 &
//
//	  2. You need an implementation of condition variables,
//	     which is *not* provided as part of the baseline threads 
//	     implementation.  The Post Office won't work without
//	     a correct implementation of condition variables.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include <sstream>
#include "copyright.h"

#include "system.h"
#include "network.h"
#include "post.h"
#include "interrupt.h"

// Test out message delivery, by doing the following:
//	1. send a message to the machine with ID "farAddr", at mail box #0
//	2. wait for the other machine's message to arrive (in our mailbox #0)
//	3. send an acknowledgment for the other machine's message
//	4. wait for an acknowledgement from the other machine to our 
//	    original message

#define LOCKS_MAX_COUNT 50
#define CV_MAX_COUNT 50
#define MV_MAX_COUNT 50

PacketHeader outPktHdr, inPktHdr;       
MailHeader outMailHdr, inMailHdr;

char *data;
char ack[25];                 
char buffer[MaxMailSize];

int farAddr = 0;

int *toAddress;
char *msg;
bool success;

struct MsgContents{
	//PacketHeader msgPktHdr;
	//MailHeader msgMailHdr;
	int machineID;
};

//DATA STRUCT FOR LOCK, CV, MV
struct ServerLock{
    int ownerOfLock;
    int inUse;
    char* nameOfLock;
    bool toBeDeleted;

    List* destMachineIDQueue; //DESTINATION
    List* msgQueue;
    LockStatus lockStatus;
};
int lockServerIDAdder = 0;
ServerLock lockServerList[LOCKS_MAX_COUNT];

struct ServerCV{
    int waitLock;
    int inUse;
    int waitQueueCount;
    char *nameOfCV;
    bool toBeDeleted;

    List *msgQueue;
    List *destMachineIDQueue; //DESTINATION
};
int cvServerIDAdder = 0;
ServerCV cvServerList[CV_MAX_COUNT];

struct ServerMV{
    int size;
    //int* mv;
	int mv;
    int inUse;

    char* nameOfMV;
};
int mvServerIDAdder = 0;
ServerMV mvServerList[MV_MAX_COUNT];

//SERVER SYSCALL FOR CREATING SERVER
void createServerLock(const char *lockName, int idOfMachine){
    int lenOfName;
    lenOfName = strlen(lockName);
	    
	/*char *temp = lockName;

    while(*temp != '\0'){
		temp++;
    }*/

    for(int i = 0; i < lockServerIDAdder; i++){
        if(lockServerList[i].nameOfLock != NULL){
            //IF BOTH LOCK NAMES ARE THE SAME
            if(strcmp(lockServerList[i].nameOfLock, lockName) == 0){
                printf("RETURN LOCK ID FOR MACHINE. LOCK ALREADY EXISTS \n");
                sprintf(ack,"%d",i);
				lockServerList[i].inUse++;
                return;
            }
        }
    }

    //CHECK TO SEE IF IT MEETS MAXIMUM SPACES AVAILABLE
    if(lockServerIDAdder >= LOCKS_MAX_COUNT){
        printf("MAXIMUM NUMBER OF LOCKS HAVE BEEN REACHED\n");
        strcpy(ack,"-1");
        return;
    }

    //SETTING UP DATA FOR LOCK
    lockServerList[lockServerIDAdder].ownerOfLock = 0;
    lockServerList[lockServerIDAdder].inUse = 1;
    lockServerList[lockServerIDAdder].toBeDeleted = false;

    lockServerList[lockServerIDAdder].destMachineIDQueue = new List;
    lockServerList[lockServerIDAdder].msgQueue = new List;
    lockServerList[lockServerIDAdder].lockStatus = FREE;

    lockServerList[lockServerIDAdder].nameOfLock = new char[40];
    strcpy(lockServerList[lockServerIDAdder].nameOfLock,lockName);

	printf("LOCK NAME: %s\n", lockName);

    printf("MACHINE ID %d CREATED LOCK %d\n", idOfMachine, lockServerIDAdder);
    sprintf(ack,"%d",lockServerIDAdder++);
    //lockServerIDAdder++;

    return;
}

int acquireServerLock(int idOfLock, int idOfMachine){
	if (idOfLock < 0 || idOfLock > lockServerIDAdder)
	{
		printf("AcquireLock: Incorrect lock id.");
		strcpy(ack, "-1");
		return 1;
	}
	//CHECK TO SEE IF THE LOCK IS AVAILABLE
	if (lockServerList[idOfLock].lockStatus == FREE){
		lockServerList[idOfLock].ownerOfLock = idOfMachine;
		lockServerList[idOfLock].lockStatus = BUSY;

		printf("LOCK IS AVAILABLE FOR MACHINE ID %d\n", idOfMachine);
	}

	else{
		msg = new char[70];
		strcpy(msg, "clientmsg");

		toAddress = new int;
		*toAddress = idOfMachine;

		//ADD MESSAGE TO THE WAITING QUEUE IF LOCK IS NOT AVAILABLE
		lockServerList[idOfLock].destMachineIDQueue->Append((void *)toAddress);
		//lockServerList[idOfLock].destMachineIDQueue->Append((void *)idOfMachine);
		//lockServerList[idOfLock].msgQueue->Append((void *)msg);

		printf("MACHINE ID %d IS PUT TO WAIT QUEUE SINCE LOCK IS UNAVAIALBLE\n", idOfMachine);

		return 0;
	}

	strcpy(ack, "1");
	return 1;
}



int releaseServerLock(int idOfLock, int idOfMachine){
	if (idOfLock < 0 || idOfLock > lockServerIDAdder)
	{
		printf("ReleaseLock: Incorrect lock id.");
		strcpy(ack, "-1");
		return 1;
	}
	//LOCKS STILL IN LOCK QUEUE
	if (!(lockServerList[idOfLock].msgQueue->IsEmpty())){
		char *pointerMsg;
		int *pointerMachineID;

		//REMOVE MESSAGE FROM MESSAGE QUEUE  
		pointerMsg = (char *)lockServerList[idOfLock].msgQueue->Remove();
		//REMOVE MACHINE ID FROM MACHINE ID QUEUE
		pointerMachineID = (int *)lockServerList[idOfLock].destMachineIDQueue->Remove();
		//int num = (int)lockServerList[idOfLock].destMachineIDQueue->Remove();
		//lockServerList[idOfLock].ownerOfLock = *pointerMachineID;
		//sendMessage(*pointerMachineID, pointerMsg);

		strcpy(ack, "1");
		outPktHdr.to = *pointerMachineID; //location
		//MAILBOX IS ALWAYS 0                                      
		outMailHdr.to = *pointerMachineID;
		outMailHdr.from = 0;
		outMailHdr.length = strlen(ack) + 1;

		postOffice->Send(outPktHdr, outMailHdr, ack);
	}
	else{
		lockServerList[idOfLock].ownerOfLock = 0;
		lockServerList[idOfLock].lockStatus = FREE;
	}

	strcpy(ack, "1");
	return 1;
}

void destroyServerLock(int idOfLock, int idOfMachine){
	if (idOfLock < 0 || idOfLock > lockServerIDAdder)
	{
		printf("DestroyLock: Incorrect lock id.");
		strcpy(ack, "-1");
		return;
	}
	if (!(lockServerList[idOfLock].msgQueue->IsEmpty())){
		lockServerList[idOfLock].toBeDeleted = true;
		strcpy(ack, "1");
		return;
	}
	lockServerList[idOfLock].inUse--; //DECREMENT THE NUMBER OF MACHINE ID IN USE

	if (lockServerList[idOfLock].inUse == 0){
		lockServerList[idOfLock].ownerOfLock = 0;
		lockServerList[idOfLock].nameOfLock = NULL;

		delete lockServerList[idOfLock].msgQueue;
		delete lockServerList[idOfLock].destMachineIDQueue;
	}

	strcpy(ack, "1");
	return;
}

void CreateCV(const char *nameOfCondition, int idOfMachine){
	int lenOfName;
	lenOfName = strlen(nameOfCondition);

	for (int i = 1; i < cvServerIDAdder; i++){
		if (cvServerList[i].nameOfCV != NULL){
			//IF BOTH LOCK NAMES ARE THE SAME
			if (strcmp(cvServerList[i].nameOfCV, nameOfCondition) == 0){
				printf("RETURN CV ID FOR MACHINE %d. CV ALREADY EXISTS \n", idOfMachine);
				sprintf(ack, "%d", i);
				cvServerList[i].inUse++;
				return;
			}
		}
	}

	//CHECK TO SEE IF IT MEETS MAXIMUM SPACES AVAILABLE
	if (cvServerIDAdder >= CV_MAX_COUNT){
		printf("MAXIMUM NUMBER OF LOCKS HAVE BEEN REACHED\n");
		strcpy(ack, "-1");
		return;
	}

	//SETTING UP DATA FOR LOCK
	cvServerList[cvServerIDAdder].inUse = 1;
	cvServerList[cvServerIDAdder].toBeDeleted = false;
	cvServerList[cvServerIDAdder].waitQueueCount = 0;

	cvServerList[cvServerIDAdder].destMachineIDQueue = new List;
	cvServerList[cvServerIDAdder].msgQueue = new List;
	cvServerList[cvServerIDAdder].waitLock = -1;

	cvServerList[cvServerIDAdder].nameOfCV = new char[50];

	strcpy(cvServerList[cvServerIDAdder].nameOfCV, nameOfCondition);

	printf("MACHINE ID %d CREATED LOCK %d\n", idOfMachine, cvServerIDAdder);
	sprintf(ack, "%d", cvServerIDAdder);
	cvServerIDAdder++;

	return;
}

int DestroyCV(int idOfCondition, int idOfMachine){
	if (idOfCondition < 0 || idOfCondition > cvServerIDAdder)
	{
		printf("DestroyCV: Incorrect lock id.");
		strcpy(ack, "-1");
		return 1;
	}
	if (!(cvServerList[idOfCondition].msgQueue->IsEmpty())){
		cvServerList[idOfCondition].toBeDeleted = true;
		strcpy(ack, "1");
		return 1;
	}
	cvServerList[idOfCondition].inUse--; //DECREMENT THE NUMBER OF CV ID IN USE

	if (cvServerList[idOfCondition].inUse == 0){
		cvServerList[idOfCondition].waitQueueCount = -1;
		cvServerList[idOfCondition].waitLock = -1;
		cvServerList[idOfCondition].nameOfCV = NULL;

		delete cvServerList[idOfCondition].msgQueue;
	}

	strcpy(ack, "1");
	return 1;
}

int WaitCV(int conditionID, int lockID, int idOfMachine){
	if (conditionID < 0 || conditionID > cvServerIDAdder ||
		lockID < 0 || lockID > lockServerIDAdder)
	{
		printf("WaitCV: Incorrect condition or lock id.");
		strcpy(ack, "-1");
		return 1;
	}

	int index = 0;
	//int conditionID, lockID;
	//char conditionVs[30], locks[30];

	//ACQUIRE CONDITION VARIABLE ID FROM PARAMETERS PASSED IN
	//*(conditionVs + index) = *message;
	//conditionID = atoi(conditionVs);

	//index = 0;

	//ACQUIRE LOCK ID FROM PARAMETERS PASSED IN
	//*(locks + index) = *message;
	//lockID = atoi(locks);

	//WAIT IS CALLED BY THREAD WITH CV LOCK
	if (cvServerList[conditionID].waitLock == -1)
		cvServerList[conditionID].waitLock = lockID;
	else
	{
		printf("WaitCV: Lock being waited on.");
		strcpy(ack, "-1");
		return 1;
	}

	//exiting the monitor before going to sleep
	releaseServerLock(lockID, idOfMachine);

	//ADD CURRENT THREAD TO THE WAITING QUEUE OF CV 
	msg = new char[70];
	strcpy(msg, "clientcvmsg");
	toAddress = new int;
	*toAddress = idOfMachine;

	cvServerList[conditionID].msgQueue->Append((void *)msg);                // Add the message to the wait queue
	cvServerList[conditionID].destMachineIDQueue->Append((void *)toAddress);               // Add the machine id to the wait queue
	printf("MACHINE ID %d WITH LOCK %d IS PUT TO CV WAIT QUEUE %d \n", idOfMachine, lockID, conditionID);

	return 0;
}

int SignalCV(int conditionID, int lockID, int idOfMachine){

	if (conditionID < 0 || conditionID > cvServerIDAdder ||
		lockID < 0 || lockID > lockServerIDAdder)
	{
		printf("SignalCV: Incorrect condition or lock id.");
		strcpy(ack, "-1");
		return 1;
	}

	if (lockID != cvServerList[conditionID].waitLock)
	{
		printf("SignalCV: LockID does not equal the waiting Lock.");
		strcpy(ack, "-1");
		return 1;
	}

	int index = 0;
	//int conditionID, lockID;
	//char conditionVs[30], locks[30];
	//char *msg;
	//msg = message;

	//ACQUIRE CONDITION VARIABLE ID FROM PARAMETERS PASSED IN
	//*(conditionVs + index) = *message;
	//conditionID = atoi(conditionVs);

	//message++;
	//index = 0;

	//ACQUIRE LOCK ID FROM PARAMETERS PASSED IN
	//*(locks + index) = *message;
	//lockID = atoi(locks);

	cvServerList[conditionID].msgQueue->Remove();

	if(cvServerList[conditionID].msgQueue->IsEmpty()){
		cvServerList[conditionID].waitLock = -1;
	}
//
	//ACQUIRE LOCK FOR SIGNALLING 
	int signalMachineID = *((int *)cvServerList[conditionID].destMachineIDQueue->Remove());
	acquireServerLock(signalMachineID, lockID);
	printf("MACHINE ID %d WITH LOCK %d IS PUT TO CV WAIT QUEUE %d \n", idOfMachine, lockID, conditionID);
	
	strcpy(ack, "1");
	outPktHdr.to = signalMachineID; //location                                    
	outMailHdr.to = signalMachineID;
	outMailHdr.from = 0;
	outMailHdr.length = strlen(ack) + 1;
	postOffice->Send(outPktHdr, outMailHdr, ack);

	strcpy(ack, "1");

	return 1;
}

int BroadcastCV(int conditionID, int lockID, int idOfMachine){

	//int index = 0;
	//int conditionID, lockID;
	//char conditionVs[30], locks[30];
	//char *msg;
	//msg = message;

	//ACQUIRE CONDITION VARIABLE ID FROM PARAMETERS PASSED IN
	//*(conditionVs + index) = *message;
	//conditionID = atoi(conditionVs);

	//message++;
	//index = 0;

	//ACQUIRE LOCK ID FROM PARAMETERS PASSED IN
	//*(locks + index) = *message;
	//lockID = atoi(locks);

	while (!(cvServerList[conditionID].msgQueue->IsEmpty())){
		SignalCV(conditionID, lockID, idOfMachine);
	}
	return 1;
}

void CreateMV(const char *name, int idOfMachine)
{
	for (int i = 1; i < mvServerIDAdder; i++){
		if (mvServerList[i].nameOfMV != NULL){
			//IF BOTH LOCK NAMES ARE THE SAME
			if (strcmp(mvServerList[i].nameOfMV, name) == 0){
				printf("RETURN MV ID FOR MACHINE %d. MV ALREADY EXISTS \n", idOfMachine);
				sprintf(ack, "%d", i);
				mvServerList[i].inUse++;
				return;
			}
		}
	}

	if (mvServerIDAdder >= MV_MAX_COUNT){
		printf("MAXIMUM NUMBER OF MVs HAVE BEEN REACHED\n");
		strcpy(ack, "-1");
		return;
	}

	//mvServerList[mvServerIDAdder].size = atoi(string2);
	mvServerList[mvServerIDAdder].inUse = 1;

	//mvServerList[mvServerIDAdder].monitorVar = new int[atoi(string2)];

	mvServerList[mvServerIDAdder].nameOfMV = new char[40];
	strcpy(mvServerList[mvServerIDAdder].nameOfMV, name);

	printf("MACHINE ID %d CREATED MV %d\n", idOfMachine, mvServerIDAdder);
	sprintf(ack, "%d", mvServerIDAdder);
	mvServerIDAdder++;
	return;
}

void DestroyMV(int idOfMV, int idOfMachine)
{
	if (idOfMV < 0 || idOfMV > mvServerIDAdder)
	{
		printf("DestroyMV: Incorrect mv id.");
		strcpy(ack, "-1");
		return;
	}

	mvServerList[idOfMV].inUse--;
	if (mvServerList[idOfMV].inUse == 0){
		mvServerList[idOfMV].size = -1;
		mvServerList[idOfMV].mv = NULL;
		mvServerList[idOfMV].nameOfMV = NULL;
	}
	strcpy(ack, "1");
	
	return;
}

void GetMV(int idOfMV, int idOfMachine)
{
	if (idOfMV < 0 || idOfMV > mvServerIDAdder)
	{
		printf("GetMV: Incorrect mv id.");
		strcpy(ack, "-1");
		return;
	}

	int value = mvServerList[idOfMV].mv;
	printf("VALUE OF %d FOR MV %d GOT RETURNED TO MACHINE ID %d\n", value, idOfMV, idOfMachine);
	sprintf(ack, "%d", value);
	return;
}

void SetMV(int idOfMV, int value, int machineID)
{
	if (idOfMV < 0 || idOfMV > mvServerIDAdder)
	{
		printf("SetMV: Incorrect mv id.");
		strcpy(ack, "-1");
		return;
	}

	mvServerList[idOfMV].mv = value;
	printf("MACHINE ID %d SET VALUE %d FROM MV %d\n", machineID, value, idOfMV);
	//printf("\n INDEX %d IS SET VALUE WITH %d\n", idOfMV, *(pointer + anotherIndex));
	strcpy(ack, "1");
	return;
}

int parsingRequest(char* message, int idOfMachine){

    int statusResult;
    //char desiredRequest = *message;
	std::stringstream ss;
	ss << message;
	char* request;
	ss >> request;
    //message++;

	if (!strcmp(request, "CreateLock"))
	{
		string name;
		ss >> name;
		printf("Request: CreateLock\n");
		createServerLock(name.c_str(), idOfMachine);
		return 1;
	} else if (!strcmp(request, "DestroyLock"))
	{
		string name;
		ss >> name;
		printf("Request: DestroyLock\n");
		destroyServerLock(atoi(name.c_str()), idOfMachine);
		return 1;
	} else if (!strcmp(request, "AcquireLock"))
	{
		string name;
		ss >> name;
		printf("Request: AcquireLock\n");
		int returnVal = acquireServerLock(atoi(name.c_str()), idOfMachine);
		return returnVal;
	} else if (!strcmp(request, "ReleaseLock"))
	{
		string name;
		ss >> name;
		printf("Request: ReleaseLock\n");
		int returnVal = releaseServerLock(atoi(name.c_str()), idOfMachine);
		return returnVal;
	} else if (!strcmp(request, "CreateCV"))
	{
		string name;
		ss >> name;
		printf("Request: CreateCV\n");
		CreateCV(name.c_str(), idOfMachine);
		return 1;
	} else if (!strcmp(request, "DestroyCV"))
	{
		string name;
		ss >> name;
		printf("Request: DestroyCV\n");
		DestroyCV(atoi(name.c_str()), idOfMachine);
		return 1;
	} else if (!strcmp(request, "WaitCV"))
	{
		string name, lock_name;
		ss >> name;
		ss >> lock_name;
		printf("Request: WaitCV\n");
		int returnVal = WaitCV(atoi(name.c_str()), atoi(lock_name.c_str()), idOfMachine);
		return returnVal;
	} else if (!strcmp(request, "BroadcastCV"))
	{
		string name, lock_name;
		ss >> name;
		ss >> lock_name;
		printf("Request: BroadcastCV\n");
		BroadcastCV(atoi(name.c_str()), atoi(lock_name.c_str()), idOfMachine);
		return 1;
	} else if (!strcmp(request, "SignalCV"))
	{
		string name, lock_name;
		ss >> name;
		ss >> lock_name;
		printf("Request: SignalCV\n");
		SignalCV(atoi(name.c_str()), atoi(lock_name.c_str()), idOfMachine);
		return 1;
	}
	else if (!strcmp(request, "CreateMonitor"))
	{
		string name;
		ss >> name;
		printf("Request: CreateMonitor\n");
		//CreateMV(name.c_str(), idOfMachine);
		return 1;
	} else if (!strcmp(request, "DestroyMonitor"))
	{
		string name;
		ss >> name;
		printf("Request: DestroyMonitor\n");
		//DestroyMV(name.c_str(), idOfMachine);
		return 1;
	} else if (!strcmp(request, "GetMonitorVal"))
	{
		string name;
		ss >> name;
		printf("Request: GetMonitorVal\n");
		//GetMV(name.c_str(), idOfMachine);
		return 1;
	}
	else if (!strcmp(request, "SetMonitorVal"))
	{
		string name, lock_name;
		ss >> name;
		ss >> lock_name;
		printf("Request: SetMonitorVal\n");
		//SetMV(name.c_str(), atoi(lock_name), idOfMachine);
		return 1;
	}
	/* swi
   /* switch(desiredRequest){

        //RESPONDING TO REQUEST FOR CREATING LOCK
        case "first": 
            createServerLock(message, idOfMachine);
            return 1;
            break;
    }*/
	return -1;
}

void doServer()
{
	printf("Beginning Server\n\n");

	while (true)
	{
		printf("Server waiting for a message\n");

		//RECEIVING MESSAGE FROM CLIENT
		postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
		printf("Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from, inMailHdr.from);
		fflush(stdout);

		//CHECK TO SEE WHICH REQUEST TO RESPOND TO AND RESPOND
		int statusResult = parsingRequest(buffer, inPktHdr.from);

		if (statusResult == 1){
			//MESSAGE SENT TO CLIENT
			//ack = "Success";
			//sprintf(ack, "Success");
			//LOCATION TO BE SENT TO
			outPktHdr.to = inPktHdr.from; //location
			//MAILBOX IS ALWAYS 0                                      
			outMailHdr.to = inMailHdr.from;
			outMailHdr.from = 0;
			outMailHdr.length = strlen(ack) + 1;

			printf("Sent \"%s\" to outPktHdr.to: %d,outMailHdr.to: %d \n", ack, outPktHdr.to, outMailHdr.to);
			success = postOffice->Send(outPktHdr, outMailHdr, ack);

			if (!success) {
				printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
				interrupt->Halt();
			}
		}
	}
}

void MailTest(){
    printf("\n Server began now! \n");

    //CONTINUE CHECKING FOR REQUESTS
    while(true)
    {

        printf("Server waiting for a message\n");

        int statusResult;

        //RECEIVING MESSAGE FORM CLIENT
        postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);                         
        printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
        fflush(stdout);

        //CHECK TO SEE WHICH REQUEST TO RESPOND TO AND RESPOND
		statusResult = parsingRequest(buffer, inPktHdr.from);

        if(statusResult == 1){
            //MESSAGE SENT TO CLIENT
            //LOCATION TO BE SENT TO
            outPktHdr.to = inPktHdr.from; //location
            //MAILBOX IS ALWAYS 0                                      
            outMailHdr.to = inMailHdr.from; 
            outMailHdr.from = 0;
            outMailHdr.length = strlen(ack) + 1;

            printf("Sent \"%s\" to outPktHdr.to: %d,outMailHdr.to: %d \n",ack,outPktHdr.to,outMailHdr.to);
            success = postOffice->Send(outPktHdr, outMailHdr, ack);

            if ( !success ) {
                printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
                interrupt->Halt();
            }
        }
    }
    // Then we're done!
    interrupt->Halt();

    /*PacketHeader outPktHdr, inPktHdr;
    MailHeader outMailHdr, inMailHdr;
    char *data = "Hello there!";
    char *ack = "Got it!";
    char buffer[MaxMailSize];

    // construct packet, mail header for original message
    // To: destination machine, mailbox 0
    // From: our machine, reply to: mailbox 1
    outPktHdr.to = farAddr;		
    outMailHdr.to = 0;

    outMailHdr.from = 1; 

    outMailHdr.length = strlen(data) + 1; //length of data + 1

    // Send the first message
    bool success = postOffice->Send(outPktHdr, outMailHdr, data); 
    //Set whatever returned to success to the boolean

    //If not sucessful, say for example false
    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the first message from the other machine
    postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Send acknowledgement to the other machine (using "reply to" mailbox
    // in the message that just arrived
    outPktHdr.to = inPktHdr.from; //Packet header that just arrived
    outMailHdr.to = inMailHdr.from; //Mail header that just arrived
    outMailHdr.length = strlen(ack) + 1; 

    //
    success = postOffice->Send(outPktHdr, outMailHdr, ack); 

    if ( !success ) {
      printf("The postOffice Send failed. You must not have the other Nachos running. Terminating Nachos.\n");
      interrupt->Halt();
    }

    // Wait for the ack from the other machine to the first message we sent.
    postOffice->Receive(1, &inPktHdr, &inMailHdr, buffer);
    printf("Got \"%s\" from %d, box %d\n",buffer,inPktHdr.from,inMailHdr.from);
    fflush(stdout);

    // Then we're done!
    interrupt->Halt();*/
}
