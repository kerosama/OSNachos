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
#define SERVER_MAX_COUNT 5

class Server{
	PacketHeader outPktHdr, inPktHdr;
	MailHeader outMailHdr, inMailHdr;

	char *data;
	char *ack;
	char *servAck;
	bool hasClientData, shouldMsgClient;
	char buffer[MaxMailSize];

	int farAddr;

	int *toAddress;
	char *msg;
	bool success;
	int serverNum, numOfServers;

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
		int lockID;

		List* destMachineIDQueue; //DESTINATION
		List* msgQueue;
		LockStatus lockStatus;
	};
	int lockServerIDAdder;
	ServerLock lockServerList[LOCKS_MAX_COUNT];

	struct ServerCV{
		int waitLock;
		int inUse;
		int waitQueueCount;
		char *nameOfCV;
		bool toBeDeleted;
		int cvID;

		List *msgQueue;
		List *destMachineIDQueue; //DESTINATION
	};
	int cvServerIDAdder;
	ServerCV cvServerList[CV_MAX_COUNT];

	struct ServerMV{
		int size;
		//int* mv;
		int mv;
		int inUse;

		char* nameOfMV;
	};
	int mvServerIDAdder;
	ServerMV mvServerList[MV_MAX_COUNT];

public:
	Server::Server(int serverNumber, int numOfServers);
	void Server::doServer();

private:
	void CreateMessage(char* request, char* clName, char* varName)
	{
		char buff[50];

		//CREATE MESSAGE IN PARTICULAR FORMAT TO BE SENT OVER TO SERVER 
		sprintf(buff, "%s %s %s", request, clName, varName);
		int length = strlen(buff);
		ack = new char[length];
		strcpy(ack, buff);
	}

	void CreateMessage(char* request, char* clName, char* varName, char* varName2)
	{
		char buff[50];

		//CREATE MESSAGE IN PARTICULAR FORMAT TO BE SENT OVER TO SERVER 
		sprintf(buff, "%s %s %s %s", request, clName, varName, varName2);
		int length = strlen(buff);
		ack = new char[length];
		strcpy(ack, buff);
	}

	//SERVER SYSCALL FOR CREATING SERVER
	int createServerLock(const char *lockName, int idOfMachine, bool isClient){
		printf("Starting createServerLock\n");
		hasClientData = false;
		int lenOfName;
		lenOfName = strlen(lockName);

		bool haveLock = false;
		int lockIndex;
		for (int a = 0; a < lockServerIDAdder; a++) {
			if (lockServerList[a].lockID == atoi(lockName))
			{
				haveLock = true; hasClientData = true; lockIndex = a; break;
			}
		}

		if (isClient && !hasClientData)
		{
			char buff[50];
			char clName[2];
			int idOfLock = atoi(lockName);
			char lock_Name[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(lock_Name, "%d", idOfLock);
			CreateMessage("servCreateLock", clName, lock_Name);
			printf("ACK: %s\n", ack);
			int exists = 0;
			
			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				printf("SERVER NUMBER: %d\n", serverNum);
				printf("X: %d\n", x);

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);
				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) == 1)
				{
					exists = 1;
					printf("Lock Handled!\n");
					break;
				}
			}

			if (exists == 0)
			{
				printf("Lock does not exist!\n");
			}
			else
			{
				printf("Lock found on another server!\n");
				strcpy(ack, "1");
				return 0;
			}
		}

		if (!isClient && !hasClientData)
		{
			printf("Do not have the lock for the other server!\n");
			sprintf(ack, "-1");
			return 1;
		}
		else if (!isClient)
		{
			printf("Have the lock the other client is looking for.\n");
			sprintf(ack, "%d", lockServerList[lockIndex].lockID);
			return 1;
		}

		for (int i = 0; i < lockServerIDAdder; i++){
			if (lockServerList[i].nameOfLock != NULL){
				//IF BOTH LOCK NAMES ARE THE SAME
				if (strcmp(lockServerList[i].nameOfLock, lockName) == 0){
					printf("RETURN LOCK ID FOR MACHINE. LOCK ALREADY EXISTS \n");
					sprintf(ack, "%d", i);
					lockServerList[i].inUse++;
					return 1;
				}
			}
		}

		//CHECK TO SEE IF IT MEETS MAXIMUM SPACES AVAILABLE
		if (lockServerIDAdder >= LOCKS_MAX_COUNT){
			printf("MAXIMUM NUMBER OF LOCKS HAVE BEEN REACHED\n");
			strcpy(ack, "-1");
			return 1;
		}

		//SETTING UP DATA FOR LOCK
		lockServerList[lockServerIDAdder].ownerOfLock = 0;
		lockServerList[lockServerIDAdder].inUse = 1;
		lockServerList[lockServerIDAdder].toBeDeleted = false;

		lockServerList[lockServerIDAdder].destMachineIDQueue = new List;
		lockServerList[lockServerIDAdder].msgQueue = new List;
		lockServerList[lockServerIDAdder].lockStatus = FREE;
		lockServerList[lockServerIDAdder].lockID = atoi(lockName);

		lockServerList[lockServerIDAdder].nameOfLock = new char[40];
		strcpy(lockServerList[lockServerIDAdder].nameOfLock, lockName);

		printf("LOCK NAME: %s\n", lockName);

		printf("MACHINE ID %d CREATED LOCK %d\n", idOfMachine, lockServerIDAdder);
		sprintf(ack, "%d", lockServerIDAdder++);

		return 1;
	}

	int acquireServerLock(int idOfLock, int idOfMachine, bool isClient){
		hasClientData = false;
		shouldMsgClient = false;
		int lockIndex = -1;
		if (idOfLock < 0)
		{
			printf("AcquireLock: Incorrect lock id.");
			strcpy(ack, "-1");
			return 1;
		}
		bool haveLock = false;
		for (int a = 0; a < lockServerIDAdder; a++)
			if (lockServerList[a].lockID == idOfLock)
			{
				haveLock = true; hasClientData = true; lockIndex = a; break;
			}
		
		if (!haveLock && !isClient)
		{
			//Server asking for lock I don't have
			printf("AcquireLock: Do not have lock for server.");
			strcpy(ack, "-1");
			return 1;
		}

		if (isClient && !haveLock)
		{
			char buff[50];
			char clName[2];
			char lockName[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(lockName, "%d", idOfLock);
			CreateMessage("servAcquireLock", clName, lockName);
			int acquired = 0;
			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				/*outPktHdr.to = x; //location
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = x;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;*/

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);

				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) > -1)
				{
					acquired = 1;
					printf("Lock Handled!\n");
					break;
				}
			}

			if (acquired == 0)
			{
				printf("Lock does not exist!\n");
				strcpy(ack, "-1");
				return 1;
			}
			else
			{
				printf("Lock found and acquired on another server!\n");
				return 0;
			}
		}
		//shouldMsgClient = true;
		//CHECK TO SEE IF THE LOCK IS AVAILABLE
		if (lockServerList[lockIndex].lockStatus == FREE){
			lockServerList[lockIndex].ownerOfLock = idOfMachine;
			lockServerList[lockIndex].lockStatus = BUSY;
			printf("LOCK IS AVAILABLE FOR MACHINE ID %d\n", idOfMachine);
			shouldMsgClient = true;
		}
		else
		{
			msg = new char[70];
			strcpy(msg, "clientmsg");
			strcpy(ack, "1");
			toAddress = new int;
			*toAddress = idOfMachine;
			shouldMsgClient = false;
			//ADD MESSAGE TO THE WAITING QUEUE IF LOCK IS NOT AVAILABLE
			lockServerList[lockIndex].destMachineIDQueue->Append((void *)toAddress);

			printf("MACHINE ID %d IS PUT TO WAIT QUEUE SINCE LOCK IS UNAVAIALBLE\n", idOfMachine);

			return 0;
		}

		strcpy(ack, "1");
		return 1;
	}

	int releaseServerLock(int idOfLock, int idOfMachine, bool isClient){
		if (idOfLock < 0)
		{
			printf("ReleaseLock: Incorrect lock id.");
			strcpy(ack, "-1");
			return 1;
		}
		
		int lockIndex;
		bool haveLock = false;
		for (int a = 0; a < lockServerIDAdder; a++)
		if (lockServerList[a].lockID == idOfLock)
		{
			haveLock = true; hasClientData = true; lockIndex = a; break;
		}

		if (isClient && !haveLock)
		{
			char buff[50];
			char clName[2];
			char lockName[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(lockName, "%d", idOfLock);
			CreateMessage("servReleaseLock", clName, lockName);
			int acquired = 0;
			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				/*outPktHdr.to = x; //location
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = x;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;*/

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);

				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) == 1)
				{
					acquired = 1;
					printf("Lock Handled!\n");
					break;
				}
			}

			if (acquired == 0)
			{
				printf("Lock does not exist!\n");
				strcpy(ack, "-1");
				return 1;
			}
			else
			{
				printf("Lock found and released on another server!\n");
				strcpy(ack, "1");
				return 0;
			}
		}

		if (!isClient && !hasClientData)
		{
			printf("Do not have the lock for the other server!\n");
			sprintf(ack, "-1");
			return 1;
		}
		//LOCKS STILL IN LOCK QUEUE
		if (!lockServerList[lockIndex].destMachineIDQueue->IsEmpty()){
			char *pointerMsg;
			int *pointerMachineID;

			//REMOVE MACHINE ID FROM MACHINE ID QUEUE
			pointerMachineID = (int *)lockServerList[lockIndex].destMachineIDQueue->Remove();

			strcpy(ack, "1");

			outPktHdr.to = *pointerMachineID; //location
			//outPktHdr.from = 0;
			//MAILBOX IS ALWAYS 0                                      
			outMailHdr.to = *pointerMachineID;
			outMailHdr.from = serverNum;
			outMailHdr.length = strlen(ack) + 1;

			postOffice->Send(outPktHdr, outMailHdr, ack);
		}
		else{
			lockServerList[lockIndex].ownerOfLock = 0;
			lockServerList[lockIndex].lockStatus = FREE;
		}
		strcpy(ack, "1");
		return 1;
	}

	int destroyServerLock(int idOfLock, int idOfMachine, bool isClient){
		hasClientData = false;
		if (idOfLock < 0)
		{
			printf("DestroyLock: Incorrect lock id.");
			strcpy(ack, "-1");
			return 1;
		}

		int lockIndex;
		bool haveLock = false;
		for (int a = 0; a < lockServerIDAdder; a++)
		if (lockServerList[a].lockID == idOfLock)
		{
			haveLock = true; hasClientData = true; lockIndex = a; break;
		}

		if (isClient && !haveLock)
		{
			char buff[50];
			char clName[2];
			char lockName[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(lockName, "%d", idOfLock);
			CreateMessage("servDestroyLock", clName, lockName);
			int acquired = 0;
			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				/*outPktHdr.to = x; //location
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = x;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;*/

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);

				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) == 1)
				{
					acquired = 1;
					printf("Lock Handled!\n");
					break;
				}
			}

			if (acquired == 0)
			{
				printf("Lock does not exist!\n");
				strcpy(ack, "-1");
				return 1;
			}
			else
			{
				printf("Lock found and released on another server!\n");
				strcpy(ack, "1");
				return 0;
			}
		}
		
		if (!isClient && !hasClientData)
		{
			printf("Do not have the lock for the other server!\n");
			sprintf(ack, "-1");
			return 1;
		}

		if (!(lockServerList[lockIndex].msgQueue->IsEmpty())){
			lockServerList[lockIndex].toBeDeleted = true;
			strcpy(ack, "1");
			return 1;
		}
		lockServerList[lockIndex].inUse--; //DECREMENT THE NUMBER OF MACHINE ID IN USE

		if (lockServerList[lockIndex].inUse == 0){
			lockServerList[lockIndex].ownerOfLock = 0;
			lockServerList[lockIndex].nameOfLock = NULL;

			delete lockServerList[lockIndex].msgQueue;
			delete lockServerList[lockIndex].destMachineIDQueue;
		}
		
		strcpy(ack, "1");
		return 1;
	}

	int CreateCV(const char *nameOfCondition, int idOfMachine, bool isClient){
		hasClientData = false;
		int lenOfName;
		lenOfName = strlen(nameOfCondition);

		int cvIndex;
		bool haveCV = false;
		for (int a = 0; a < cvServerIDAdder; a++)
		if (cvServerList[a].cvID == atoi(nameOfCondition))
		{
			haveCV = true; hasClientData = true; cvIndex = a; break;
		}

		if (isClient && !hasClientData)
		{
			char buff[50];
			char clName[2];
			int idOfCV = atoi(nameOfCondition);
			char cv_Name[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(cv_Name, "%d", idOfCV);
			CreateMessage("servCreateCV", clName, cv_Name);
			printf("ACK: %s\n", ack);
			int exists = 0;

			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				printf("SERVER NUMBER: %d\n", serverNum);
				printf("X: %d\n", x);

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);
				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) == 1)
				{
					exists = 1;
					printf("CV Handled!\n");
					break;
				}
			}

			if (exists == 0)
			{
				printf("CV does not exist!\n");
			}
			else
			{
				printf("CV found on another server!\n");
				strcpy(ack, "1");
				return 0;
			}
		}

		if (!isClient && !hasClientData)
		{
			printf("Do not have the cv for the other server!\n");
			sprintf(ack, "-1");
			return 1;
		}
		else if (!isClient)
		{
			printf("Have the cv the other client is looking for.\n");
			sprintf(ack, "%d", cvServerList[cvIndex].cvID);
			return 1;
		}

		for (int i = 0; i < cvServerIDAdder; i++){
			if (cvServerList[i].nameOfCV != NULL){
				//IF BOTH CV NAMES ARE THE SAME
				if (strcmp(cvServerList[i].nameOfCV, nameOfCondition) == 0){
					printf("RETURN CV ID FOR MACHINE %d. CV ALREADY EXISTS \n", idOfMachine);
					sprintf(ack, "%d", i);
					cvServerList[i].inUse++;
					return 1;
				}
			}
		}

		//CHECK TO SEE IF IT MEETS MAXIMUM SPACES AVAILABLE
		if (cvServerIDAdder >= CV_MAX_COUNT){
			printf("MAXIMUM NUMBER OF CVS HAVE BEEN REACHED\n");
			strcpy(ack, "-1");
			return 1;
		}

		//SETTING UP DATA FOR LOCK
		cvServerList[cvServerIDAdder].inUse = 1;
		cvServerList[cvServerIDAdder].toBeDeleted = false;
		cvServerList[cvServerIDAdder].waitQueueCount = 0;
		cvServerList[cvServerIDAdder].waitQueueCount = 0;

		cvServerList[cvServerIDAdder].destMachineIDQueue = new List;
		cvServerList[cvServerIDAdder].msgQueue = new List;
		cvServerList[cvServerIDAdder].waitLock = -1;
		cvServerList[cvServerIDAdder].cvID = atoi(nameOfCondition);

		cvServerList[cvServerIDAdder].nameOfCV = new char[50];

		strcpy(cvServerList[cvServerIDAdder].nameOfCV, nameOfCondition);

		printf("MACHINE ID %d CREATED CV %d\n", idOfMachine, cvServerIDAdder);
		sprintf(ack, "%d", cvServerIDAdder);
		cvServerIDAdder++;

		return 1;
	}

	int DestroyCV(int idOfCondition, int idOfMachine, bool isClient){
		if (idOfCondition < 0 || idOfCondition > CV_MAX_COUNT)
		{
			printf("DestroyCV: Incorrect lock id.");
			strcpy(ack, "-1");
			return 1;
		}

		int cvIndex;
		bool haveCV = false;
		for (int a = 0; a < cvServerIDAdder; a++)
		if (cvServerList[a].cvID == idOfCondition)
		{
			haveCV = true; hasClientData = true; cvIndex = a; break;
		}

		if (isClient && !haveCV)
		{
			char buff[50];
			char clName[2];
			char cvName[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(cvName, "%d", idOfCondition);
			CreateMessage("servDestroyCV", clName, cvName);
			int exists = 0;
			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);

				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) == 1)
				{
					exists = 1;
					printf("CV Handled!\n");
					break;
				}
			}

			if (exists == 0)
			{
				printf("CV does not exist!\n");
				strcpy(ack, "-1");
				return 1;
			}
			else
			{
				printf("CV found and destroyed on another server!\n");
				strcpy(ack, "1");
				return 0;
			}
		}

		if (!isClient && !hasClientData)
		{
			printf("Do not have the CV for the other server!\n");
			sprintf(ack, "-1");
			return 1;
		}

		if (!(cvServerList[cvIndex].msgQueue->IsEmpty())){
			cvServerList[cvIndex].toBeDeleted = true;
			strcpy(ack, "1");
			return 1;
		}
		cvServerList[cvIndex].inUse--; //DECREMENT THE NUMBER OF CV ID IN USE

		if (cvServerList[cvIndex].inUse == 0){
			cvServerList[cvIndex].waitQueueCount = -1;
			cvServerList[cvIndex].waitLock = -1;
			cvServerList[cvIndex].nameOfCV = NULL;

			delete cvServerList[cvIndex].msgQueue;
		}

		strcpy(ack, "1");
		return 1;
	}

	int WaitCV(int conditionID, int lockID, int idOfMachine, bool isClient){
		shouldMsgClient = false;
		if (conditionID < 0 || conditionID > CV_MAX_COUNT ||
			lockID < 0 || lockID > LOCKS_MAX_COUNT)
		{
			printf("WaitCV: Incorrect condition or lock id.");
			strcpy(ack, "-1");
			return 1;
		}

		int cvIndex;
		bool haveCV = false;
		for (int a = 0; a < cvServerIDAdder; a++)
		if (cvServerList[a].cvID == conditionID)
		{
			haveCV = true; hasClientData = true; cvIndex = a; break;
		}

		if (isClient && !haveCV)
		{
			char buff[50];
			char clName[2];
			char lockName[2];
			char cvName[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(cvName, "%d", conditionID);
			sprintf(lockName, "%d", lockID);
			CreateMessage("servWaitCV", cvName, lockName, clName);
			int exists = 0;
			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);

				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) == 1)
				{
					exists = 1;
					printf("CV Handled!\n");
					break;
				}
			}

			if (exists == 0)
			{
				shouldMsgClient = false;
				printf("CV does not exist!\n");
				strcpy(ack, "-1");
				return 1;
			}
			else
			{
				shouldMsgClient = false;
				printf("CV found and waited for on another server!\n");
				strcpy(ack, "1");
				return 0;
			}
		}

		if (!isClient && !hasClientData)
		{
			printf("Do not have the CV for the other server!\n");
			sprintf(ack, "-1");
			return 1;
		}

		//WAIT IS CALLED BY THREAD WITH CV LOCK
		if (cvServerList[cvIndex].waitLock == -1)
			cvServerList[cvIndex].waitLock = lockID;
		else
		{
			printf("WaitCV: Lock being waited on.");
			strcpy(ack, "-1");
			shouldMsgClient = true;
			return 1;
		}

		//exiting the monitor before going to sleep
		releaseServerLock(lockID, idOfMachine, true);

		//ADD CURRENT THREAD TO THE WAITING QUEUE OF CV 
		msg = new char[70];
		strcpy(msg, "clientcvmsg");
		toAddress = new int;
		*toAddress = idOfMachine;

		cvServerList[cvIndex].msgQueue->Append((void *)msg);                // Add the message to the wait queue
		cvServerList[cvIndex].destMachineIDQueue->Append((void *)toAddress);               // Add the machine id to the wait queue
		printf("MACHINE ID %d WITH LOCK %d IS PUT TO CV WAIT QUEUE %d \n", idOfMachine, lockID, conditionID);

		return 0;
	}

	int SignalCV(int conditionID, int lockID, int idOfMachine, bool isClient){
		shouldMsgClient = false;
		if (conditionID < 0 || conditionID > CV_MAX_COUNT ||
			lockID < 0 || lockID > LOCKS_MAX_COUNT)
		{
			printf("SignalCV: Incorrect condition or lock id.");
			strcpy(ack, "-1");
			return 1;
		}
		int cvIndex;
		bool haveCV = false;
		for (int a = 0; a < cvServerIDAdder; a++)
		if (cvServerList[a].cvID == conditionID)
		{
			haveCV = true; hasClientData = true; cvIndex = a; break;
		}
		if (isClient && !haveCV)
		{
			char buff[50];
			char clName[2];
			char lockName[2];
			char cvName[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(cvName, "%d", conditionID);
			sprintf(lockName, "%d", lockID);
			CreateMessage("servSignalCV", cvName, lockName, clName);
			int exists = 0;
			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);

				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) == 1)
				{
					exists = 1;
					printf("CV Handled!\n");
					break;
				}
			}

			if (exists == 0)
			{
				printf("CV does not exist!\n");
				strcpy(ack, "-1");
				return 1;
			}
			else
			{
				printf("CV found and signaled on another server!\n");
				strcpy(ack, "1");
				return 0;
			}
		}

		if (!isClient && !hasClientData)
		{
			printf("Do not have the CV for the other server!\n");
			sprintf(ack, "-1");
			return 1;
		}

		if (lockID != cvServerList[cvIndex].waitLock)
		{
			printf("SignalCV: LockID does not equal the waiting Lock.\n");
			printf("LOCKID: %d", lockID);
			printf("WAITING LOCK: %d", cvServerList[cvIndex].waitLock);
			strcpy(ack, "-1");
			return 1;
		}

		int index = 0;
		cvServerList[cvIndex].msgQueue->Remove();
		if (cvServerList[cvIndex].msgQueue->IsEmpty()){
			cvServerList[cvIndex].waitLock = -1;
		}
		//
		//ACQUIRE LOCK FOR SIGNALLING 
		int signalMachineID = *((int *)cvServerList[cvIndex].destMachineIDQueue->Remove());
		//acquireServerLock(signalMachineID, lockID, true);
		printf("MACHINE ID %d WITH LOCK %d IS COMING OFF CV WAIT QUEUE %d \n", idOfMachine, lockID, conditionID);

		outPktHdr.to = signalMachineID; //location                                    
		outMailHdr.to = signalMachineID;
		outMailHdr.from = serverNum;
		outMailHdr.length = strlen(ack) + 1;
		postOffice->Send(outPktHdr, outMailHdr, ack);
		shouldMsgClient = true;
		strcpy(ack, "1");

		return 1;
	}

	int BroadcastCV(int conditionID, int lockID, int idOfMachine, bool isClient){
		shouldMsgClient = false;
		if (conditionID < 0 || conditionID > CV_MAX_COUNT ||
			lockID < 0 || lockID > LOCKS_MAX_COUNT)
		{
			printf("BroadcastCV: Incorrect condition or lock id.");
			strcpy(ack, "-1");
			return 1;
		}

		int cvIndex;
		bool haveCV = false;
		for (int a = 0; a < cvServerIDAdder; a++)
		if (cvServerList[a].cvID == conditionID)
		{
			haveCV = true; hasClientData = true; cvIndex = a; break;
		}

		if (isClient && !haveCV)
		{
			char buff[50];
			char clName[2];
			char lockName[2];
			char cvName[2]; //LOCK ID
			sprintf(clName, "%d", idOfMachine);
			sprintf(cvName, "%d", conditionID);
				sprintf(lockName, "%d", lockID);
			CreateMessage("servBroadcastCV", cvName, lockName, clName);
			int exists = 0;
			for (int x = 0; x < numOfServers; x++)
			{
				if (x == serverNum)
					continue;

				/*outPktHdr.to = x; //location
				//MAILBOX IS ALWAYS 0
				outMailHdr.to = x;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;*/

				outPktHdr.to = x; //location
				outPktHdr.from = serverNum;
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = 0;
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(ack) + 1;

				postOffice->Send(outPktHdr, outMailHdr, ack);

				postOffice->Receive(serverNum, &inPktHdr, &inMailHdr, buff);
				printf("Got \"%s\" from %d, box %d\n", buff, inPktHdr.from, inMailHdr.from);
				fflush(stdout);

				string temp;
				stringstream ss;
				ss << buff;
				ss >> temp;

				if (atoi(temp.c_str()) == 1)
				{
					exists = 1;
					printf("CV Handled!\n");
					break;
				}
			}

			if (exists == 0)
			{
				printf("CV does not exist!\n");
				strcpy(ack, "-1");
				return 1;
			}
			else
			{
				printf("CV found and waited for on another server!\n");
				strcpy(ack, "1");
				return 0;
			}
		}

		if (!isClient && !hasClientData)
		{
			printf("Do not have the CV for the other server!\n");
			sprintf(ack, "-1");
			return 1;
		}

		while (!(cvServerList[cvIndex].msgQueue->IsEmpty())){
			SignalCV(conditionID, lockID, idOfMachine, true);
		}
		shouldMsgClient = true;
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
		string clName;
		
		std::stringstream ss;
		ss << message;
		string strRequest;
		const char* request; 
		ss >> strRequest;
		//message++;
		request = strRequest.c_str();
		if (!strcmp(request, "CreateLock"))
		{
			string name;
			ss >> name;
			printf("Request: CreateLock\n");
			int returnVal = createServerLock(name.c_str(), idOfMachine, true);
			return returnVal;
		}
		else if (!strcmp(request, "servCreateLock"))
		{
			//string clName;
			string lockName;
			ss >> clName;
			ss >> lockName;
			printf("Request: servCreateLock\n");
			
			createServerLock(lockName.c_str(), atoi(clName.c_str()), false);

			if (hasClientData)
			{
				printf("CREATE LOCK ACK: %s", ack);
				//Make a message for the client and send it
				strcpy(servAck, ack);
				printf("CREATE LOCK ACK: %s", servAck);
				outPktHdr.to = atoi(clName.c_str()); //location
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = atoi(clName.c_str());
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(servAck) + 1;

				postOffice->Send(outPktHdr, outMailHdr, servAck);
				//Send 1 to the other server
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				//Send -1 (No) to other server
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "DestroyLock"))
		{
			string name;
			ss >> name;
			printf("Request: DestroyLock\n");
			int returnVal = destroyServerLock(atoi(name.c_str()), idOfMachine, true);
			return returnVal;
		}
		else if (!strcmp(request, "servDestroyLock"))
		{
			string lockName;
			ss >> clName;
			ss >> lockName;
			printf("Request: servDestroyLock\n");

			destroyServerLock(atoi(lockName.c_str()), atoi(clName.c_str()), false);

			if (hasClientData)
			{
				//Make a message for the other server and send it
				strcpy(servAck, ack);
				strcpy(ack, "1");

				outPktHdr.to = atoi(clName.c_str()); //location
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = atoi(clName.c_str());
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(servAck) + 1;

				postOffice->Send(outPktHdr, outMailHdr, servAck);
				//return the correct return value with the to set to the client
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "AcquireLock"))
		{
			string name;
			ss >> name;
			printf("Request: AcquireLock\n");
			int returnVal = acquireServerLock(atoi(name.c_str()), idOfMachine, true);
			if (returnVal == 0)
				return 0;
			return returnVal;
		}
		else if (!strcmp(request, "servAcquireLock"))
		{
			string lockName;
			ss >> clName;
			ss >> lockName;
			printf("Request: servAcquireLock\n");

			int returnVal = acquireServerLock(atoi(lockName.c_str()), atoi(clName.c_str()), false);

			if (hasClientData)
			{
				if (shouldMsgClient)
				{
					//Make a message for the other server and send it
					outPktHdr.to = atoi(clName.c_str()); //location
					//MAILBOX IS ALWAYS 0                                      
					outMailHdr.to = atoi(clName.c_str());
					outMailHdr.from = serverNum;
					outMailHdr.length = strlen(ack) + 1;

					postOffice->Send(outPktHdr, outMailHdr, servAck);
					//return the correct return value with the to set to the client
				}
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "ReleaseLock"))
		{
			string name;
			ss >> name;
			printf("Request: ReleaseLock\n");
			int returnVal = releaseServerLock(atoi(name.c_str()), idOfMachine, true);
			return returnVal;
		}
		else if (!strcmp(request, "servReleaseLock"))
		{
			string lockName;
			ss >> clName;
			ss >> lockName;
			printf("Request: servReleaseLock\n");

			int returnVal = releaseServerLock(atoi(lockName.c_str()), atoi(clName.c_str()), false);

			printf("RELEASER SERVER\n");
			if (hasClientData)
			{
				//Make a message for the other server and send it
				strcpy(servAck, ack);
				strcpy(ack, "1");

				outPktHdr.to = atoi(clName.c_str()); //location
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = atoi(clName.c_str());
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(servAck) + 1;

				postOffice->Send(outPktHdr, outMailHdr, servAck);
				printf("Blahx2\n");
				//return the correct return value with the to set to the client
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "CreateCV"))
		{
			string name;
			ss >> name;
			printf("Request: CreateCV\n");
			int returnVal = CreateCV(name.c_str(), idOfMachine, true);
			return returnVal;
		}
		else if (!strcmp(request, "servCreateCV"))
		{
			//string clName;
			string cvName;
			ss >> clName;
			ss >> cvName;
			printf("Request: servCreateCV\n");

			CreateCV(cvName.c_str(), atoi(clName.c_str()), false);

			if (hasClientData)
			{
				printf("CREATE CV ACK: %s", ack);
				//Make a message for the client and send it
				strcpy(servAck, ack);
				printf("CREATE CV ACK: %s", servAck);
				outPktHdr.to = atoi(clName.c_str()); //location
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = atoi(clName.c_str());
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(servAck) + 1;

				postOffice->Send(outPktHdr, outMailHdr, servAck);
				//Send 1 to the other server
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				//Send -1 (No) to other server
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "DestroyCV"))
		{
			string name;
			ss >> name;
			printf("Request: DestroyCV\n");
			DestroyCV(atoi(name.c_str()), idOfMachine, true);
			return 1;
		}
		else if (!strcmp(request, "servDestroyCV"))
		{
			string cvName;
			ss >> clName;
			ss >> cvName;
			printf("Request: servDestroyCV\n");

			DestroyCV(atoi(cvName.c_str()), atoi(clName.c_str()), false);

			if (hasClientData)
			{
				//Make a message for the other server and send it
				strcpy(servAck, ack);
				strcpy(ack, "1");

				outPktHdr.to = atoi(clName.c_str()); //location
				//MAILBOX IS ALWAYS 0                                      
				outMailHdr.to = atoi(clName.c_str());
				outMailHdr.from = serverNum;
				outMailHdr.length = strlen(servAck) + 1;

				postOffice->Send(outPktHdr, outMailHdr, servAck);
				//return the correct return value with the to set to the client
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "WaitCV"))
		{
			string name, lock_name;
			ss >> name;
			ss >> lock_name;
			printf("Request: WaitCV\n");
			int returnVal = WaitCV(atoi(name.c_str()), atoi(lock_name.c_str()), idOfMachine, true);
			return returnVal;
		}
		else if (!strcmp(request, "servWaitCV"))
		{
			string cvName, lock_name;
			ss >> cvName;
			ss >> lock_name;
			ss >> clName;
			printf("Request: servWaitCV\n");

			int returnVal = WaitCV(atoi(cvName.c_str()), atoi(lock_name.c_str()), atoi(clName.c_str()), false);

			if (hasClientData)
			{
				if (shouldMsgClient)
				{
					//Make a message for the other server and send it
					outPktHdr.to = atoi(clName.c_str()); //location
					//MAILBOX IS ALWAYS 0                                      
					outMailHdr.to = atoi(clName.c_str());
					outMailHdr.from = serverNum;
					outMailHdr.length = strlen(ack) + 1;

					postOffice->Send(outPktHdr, outMailHdr, servAck);
					//return the correct return value with the to set to the client
				}
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "BroadcastCV"))
		{
			string name, lock_name;
			ss >> name;
			ss >> lock_name;
			printf("Request: BroadcastCV\n");
			BroadcastCV(atoi(name.c_str()), atoi(lock_name.c_str()), idOfMachine, true);
			return 1;
		}
		else if (!strcmp(request, "servBroadcastCV"))
		{
			string cvName, lock_name;
			ss >> cvName;
			ss >> lock_name;
			ss >> clName;
			printf("Request: servBroadcastCV\n");

			int returnVal = BroadcastCV(atoi(cvName.c_str()), atoi(lock_name.c_str()), atoi(clName.c_str()), false);

			if (hasClientData)
			{
				if (shouldMsgClient)
				{
					//Make a message for the other server and send it
					outPktHdr.to = atoi(clName.c_str()); //location
					//MAILBOX IS ALWAYS 0                                      
					outMailHdr.to = atoi(clName.c_str());
					outMailHdr.from = serverNum;
					outMailHdr.length = strlen(ack) + 1;

					postOffice->Send(outPktHdr, outMailHdr, servAck);
					//return the correct return value with the to set to the client
				}
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "SignalCV"))
		{
			string name, lock_name;
			ss >> name;
			ss >> lock_name;
			printf("Request: SignalCV\n");
			int retVal = SignalCV(atoi(name.c_str()), atoi(lock_name.c_str()), idOfMachine, true);
			return retVal;
		}
		else if (!strcmp(request, "servSignalCV"))
		{
			string cvName, lock_name;
			ss >> cvName;
			ss >> lock_name;
			ss >> clName;
			printf("Request: servSignalCV\n");

			int returnVal = SignalCV(atoi(cvName.c_str()), atoi(lock_name.c_str()), atoi(clName.c_str()), false);

			if (hasClientData)
			{
				if (shouldMsgClient)
				{
					//Make a message for the other server and send it
					outPktHdr.to = atoi(clName.c_str()); //location
					//MAILBOX IS ALWAYS 0                                      
					outMailHdr.to = atoi(clName.c_str());
					outMailHdr.from = serverNum;
					outMailHdr.length = strlen(ack) + 1;

					postOffice->Send(outPktHdr, outMailHdr, servAck);
					//return the correct return value with the to set to the client
				}
				strcpy(ack, "1");
				return 1;
			}
			else
			{
				strcpy(ack, "-1");
				return 1;
			}
		}
		else if (!strcmp(request, "CreateMonitor"))
		{
			string name;
			ss >> name;
			printf("Request: CreateMonitor\n");
			//CreateMV(name.c_str(), idOfMachine);
			return 1;
		}
		else if (!strcmp(request, "DestroyMonitor"))
		{
			string name;
			ss >> name;
			printf("Request: DestroyMonitor\n");
			//DestroyMV(name.c_str(), idOfMachine);
			return 1;
		}
		else if (!strcmp(request, "GetMonitorVal"))
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
	    switch(desiredRequest){

	   //RESPONDING TO REQUEST FOR CREATING LOCK
	   case "first":
	   createServerLock(message, idOfMachine);
	   return 1;
	   break;
	   }*/
		return -1;
	}

	
};

Server::Server(int serverNumber, int totalNumberOfServers)
{
	serverNum = serverNumber;
	numOfServers = totalNumberOfServers;
	farAddr = 0;
	serverNum = serverNumber;
	hasClientData = false;
	shouldMsgClient = false;
	ack = new char[50];
	servAck = new char[50];

	lockServerIDAdder = 0;
	cvServerIDAdder = 0;
	mvServerIDAdder = 0;
}

void Server::doServer() {
	printf("Beginning Server\n\n");
	//inMailHdr.to = serverNum;
	//inPktHdr.to = serverNum;

	int pktFrom, mailFrom;

	printf("Server inPktHdr.to: %d\n", inPktHdr.to);
	printf("Server inPktHdr.from: %d\n", inPktHdr.from);
	printf("Server inMailHdr.to: %d\n", inMailHdr.to);
	printf("Server inMailHdr.from: %d\n", inMailHdr.from);
	while (true)
	{
		printf("Server waiting for a message\n");

		//RECEIVING MESSAGE FROM CLIENT
		postOffice->Receive(0, &inPktHdr, &inMailHdr, buffer);
		printf("Got \"%s\" from %d, box %d\n", buffer, inPktHdr.from, inMailHdr.from);
		fflush(stdout);

		pktFrom = inPktHdr.from;
		mailFrom = inMailHdr.from;

		//CHECK TO SEE WHICH REQUEST TO RESPOND TO AND RESPOND
		int statusResult = parsingRequest(buffer, inPktHdr.from);
		
		if (statusResult == 1){
			printf("------Sending a message!\n");
			//MESSAGE SENT TO CLIENT
			//LOCATION TO BE SENT TO
			outPktHdr.to = pktFrom; //location
			//MAILBOX IS ALWAYS SERVERNUM                                
			outMailHdr.to = mailFrom;
			outMailHdr.from = serverNum;
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

Server *servers[SERVER_MAX_COUNT];
//int numOfServers = 0;
int numTimes = 0;
void doServer(int totalServerNum)
{
	//numberOfServers++;
	printf("NET NAME: %d\n", net_name);
	if (totalServerNum < SERVER_MAX_COUNT)
	{
		servers[0] = new Server(net_name, totalServerNum);
		servers[0]->doServer();
	}
	else
	{
		printf("Max number of Servers reached!\n");
		//numberOfServers--;
		interrupt->Halt();
	}
}

void MailTest(){
    printf("\n Server began now! \n");

    //CONTINUE CHECKING FOR REQUESTS
   /* while(true)
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
    }*/
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
