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
bool successful;

//DATA STRUCT FOR LOCK, CV, MV
struct lockServer{
    int ownerOfLock;
    int inUse;
    char* nameOfLock;
    bool toBeDeleted;

    List* destMachineIDQueue; //DESTINATION
    List* msgQueue;
    LockStatus statusOfLock;
};
int lockServerIDAdder = 1;
lockServer lockServerList[LOCKS_MAX_COUNT];

struct cvServer{
    int waitLock;
    int inUse;
    int waitQueueCount;
    char *nameOfCV;
    bool toBeDeleted;

    List *msgWaitQueue;
    List *destMachineIDQueue; //DESTINATION
};
int cvServerIDAdder = 1;
ServerCV cvServerList[CV_MAX_COUNT];

struct mvServer{
    int size;
    int* mv;
    int inUse;

    char* nameOfMV;
};
int mvServerIDAdder = 1;
ServerMV mvServerList[MV_MAX_COUNT];

//SERVER SYSCALL FOR CREATING SERVER
void createServerLock(char *lockName, int idOfMachine){
    int lenOfName;
    lenOfName = strlen(nameOfLock);

    char *temp = nameOfLock;

    while(*temp != '\0'){
        temp++
    }

    for(int i = 1; i < lockServerIDAdder; i++){
        if(lockServerList[i].nameOfLock != NULL){
            //IF BOTH LOCK NAMES ARE THE SAME
            if(strcmp(lockServerList[i].nameOfLock, lockName) == 0){
                printf("RETURN LOCK ID FOR MACHINE %d. LOCK ALREADY EXISTS \n", idOfMachine);
                sprintf(ack,"%d",i);
                serverLockTable[i].inUse++;
                return;
            }
        }
    }

    //CHECK TO SEE IF IT MEETS MAXIMUM SPACES AVAILABLE
    if(lockServerIDAdder >= LOCKS_MAX_COUNT){
        printf("MAXIMUM NUMBER OF LOCKS HAVE BEEN REACHED\n");
        strcpy(ack,"-1LOCK IS FULL\0");
        return;
    }

    //SETTING UP DATA FOR LOCK
    lockServerList[lockServerIDAdder].ownerOfLock = 0;
    lockServerList[lockServerIDAdder].inUse = 1;
    lockServerList[lockServerIDAdder].toBeDeleted = false;

    lockServerList[lockServerIDAdder].destMachineIDQueue = new List;
    lockServerList[lockServerIDAdder].msgQueue = new List;
    lockServerList[lockServerIDAdder].statusOfLock = FREE;

    lockServerList[lockServerIDAdder].nameOfLock = new char[40];
    strcpy(lockServerList[lockServerIDAdder].nameOfLock,LockName);

    printf("MACHINE ID %d CREATED LOCK %d\n", idOfMachine, lockServerIDAdder);
    sprintf(ack,"%d",lockServerIDAdder);
    lockServerIDAdder++;

    return;
}

int parsingRequest(char* message, int idOfMachine){

    int statusResult;
    char desiredRequest = *message;

    message++;

    switch(desiredRequest){

        //RESPONDING TO REQUEST FOR CREATING LOCK
        case 'first': 
            CreateServerLock(message, idOfMachine);
            return 1;
            break;
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
        statusResult = parsingRequest(inPktHdr.from, buffer)

        if(statusResult == 1){
            //MESSAGE SENT TO CLIENT
            //LOCATION TO BE SENT TO
            outPktHdr.to = inPktHdr.from; //location
            //MAILBOX IS ALWAYS 0                                      
            outMailHdr.to = 0; 
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
