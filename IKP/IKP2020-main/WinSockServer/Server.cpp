#define _WINSOCK_DEPRECATED_NO_WARNINGS

#define WIN32_LEAN_AND_MEAN
#define HAVE_STRUCT_TIMESPEC

#include <windows.h>
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdlib.h>
#include <stdio.h>
#include "conio.h"
#include <time.h>
#include <string.h>
#pragma comment (lib, "Ws2_32.lib")
#pragma comment (lib, "Mswsock.lib")
#pragma comment (lib, "AdvApi32.lib")

#define SERVER_PORT 27016
#define BUFFER_SIZE 256
#define MAX_CLIENTS 8
#define MAX 11.00
#define MIN 7.00

DWORD WINAPI handle_connection(LPVOID lpParam);
DWORD WINAPI handle_introduction(LPVOID lpParam);
DWORD WINAPI handle_results(LPVOID lpParam);
DWORD WINAPI handle_comp(LPVOID lpParam);
DWORD handle_ID[MAX_CLIENTS];
DWORD comp_ID;
DWORD res_ID;
HANDLE hHandle[MAX_CLIENTS];
HANDLE hHandle_intr[MAX_CLIENTS];
HANDLE hHanle_res;
HANDLE intr_semaphore;
HANDLE res_semaphore;
HANDLE comp;

double calc_points(double min, double max);

typedef struct Contestant {
	int id;
	char name[15];
	SOCKET clientSocket;
	double points;
	bool disq = FALSE;
	int placement = 0;
}CONTESTANT;

CONTESTANT contestants[MAX_CLIENTS];

int round = 0;
int limit;
int active_players = MAX_CLIENTS;
// TCP server that use blocking sockets
int main()
{

	// Socket used for listening for new clients 
	SOCKET listenSocket = INVALID_SOCKET;

	// Socket used for communication with client
	SOCKET acceptedSocket[MAX_CLIENTS];


	// Variable used to store function return value
	int iResult;
	int counter = 0;

	

	// Buffer used for storing incoming data
//	char dataBuffer[BUFFER_SIZE];

	// WSADATA data structure that is to receive details of the Windows Sockets implementation
	WSADATA wsaData;
	
	// Initialize windows sockets library for this process
	if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
	{
		printf("WSAStartup failed with error: %d\n", WSAGetLastError());
		return 1;
	}


	// Initialize serverAddress structure used by bind
	sockaddr_in serverAddress;
	memset((char*)&serverAddress, 0, sizeof(serverAddress));
	serverAddress.sin_family = AF_INET;				// IPv4 address family
	serverAddress.sin_addr.s_addr = INADDR_ANY;		// Use all available addresses
	serverAddress.sin_port = htons(SERVER_PORT);	// Use specific port


	// Create a SOCKET for connecting to server
	listenSocket = socket(AF_INET,      // IPv4 address family
		SOCK_STREAM,  // Stream socket
		IPPROTO_TCP); // TCP protocol

// Check if socket is successfully created
	if (listenSocket == INVALID_SOCKET)
	{
		printf("socket failed with error: %ld\n", WSAGetLastError());
		WSACleanup();
		return 1;
	}

	// Setup the TCP listening socket - bind port number and local address to socket
	iResult = bind(listenSocket, (struct sockaddr*)&serverAddress, sizeof(serverAddress));

	// Check if socket is successfully binded to address and port from sockaddr_in structure
	if (iResult == SOCKET_ERROR)
	{
		printf("bind failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	// Set listenSocket in listening mode
	iResult = listen(listenSocket, SOMAXCONN);
	if (iResult == SOCKET_ERROR)
	{
		printf("listen failed with error: %d\n", WSAGetLastError());
		closesocket(listenSocket);
		WSACleanup();
		return 1;
	}

	printf("Server socket is set to listening mode. Waiting for new connection requests.\n");
	do
	{

			
		// Struct for information about connected client
		sockaddr_in clientAddr;

		int clientAddrSize = sizeof(struct sockaddr_in);

		// Accept new connections from clients 
		for (int i = 0; i < MAX_CLIENTS; i++) {
			acceptedSocket[i] = accept(listenSocket, (struct sockaddr*)&clientAddr, &clientAddrSize);

			// Check if accepted socket is valid 
			if (acceptedSocket[i] == INVALID_SOCKET)
			{
				printf("accept failed with error: %d\n", WSAGetLastError());
				closesocket(listenSocket);
				WSACleanup();
				return 1;
			}
			else {
				counter++;
				contestants[i].id = i;
				strcpy(contestants[i].name, "");
				contestants[i].clientSocket = acceptedSocket[i];
				contestants[i].points = 0;

			}

			printf("\nNew client request accepted. Client address: %s : %d\n", inet_ntoa(clientAddr.sin_addr), ntohs(clientAddr.sin_port));
		}
		for (int i = 0; i < MAX_CLIENTS; i++) {
			hHandle_intr[i] = CreateThread(NULL, 0, &handle_introduction, &contestants[i], 0, &handle_ID[i]);

		}
		
		intr_semaphore = CreateSemaphore(0, 0, 1, NULL);
		res_semaphore = CreateSemaphore(0, 0, 1, NULL);
		comp = CreateThread(NULL, 0, &handle_comp, NULL, 0, &comp_ID);
		hHanle_res = CreateThread(NULL, 0, &handle_results, NULL, 0, &res_ID);
		while (true) {
			WaitForMultipleObjects(MAX_CLIENTS, hHandle_intr, TRUE, INFINITE);
			/*if (p == MAX_CLIENTS) {*/
			ReleaseSemaphore(intr_semaphore, 1, NULL);
			
			
			Sleep(100);
		}
		
		CloseHandle(intr_semaphore);

		for (int i = 0; i < MAX_CLIENTS; i++) {
			CloseHandle(hHandle_intr[i]);

		}
		
		CloseHandle(handle_results);
		// Here is where server shutdown loguc could be placed
	} while (true);

	// Shutdown the connection since we're done
	for (int i = 0; i < counter; i++) {
		iResult = shutdown(acceptedSocket[i], SD_BOTH);

		// Check if connection is succesfully shut down.
		if (iResult == SOCKET_ERROR)
		{
			printf("shutdown failed with error: %d\n", WSAGetLastError());
			closesocket(acceptedSocket[i]);
			WSACleanup();
			return 1;
		}
	}
	//Close listen and accepted sockets
	closesocket(listenSocket);
	for (int i = 0; i < counter; i++) {
		closesocket(acceptedSocket[i]);
	}
	// Deinitialize WSA library
	WSACleanup();

	return 0;
	
	
}

DWORD WINAPI handle_introduction(LPVOID lpParam) {

	CONTESTANT contestant = *(CONTESTANT*)lpParam;
	char dataBuffer[BUFFER_SIZE];
	char* message = "Predstavi se";
	int Result = send(contestant.clientSocket, message, (int)strlen(message), 0);

	// Check result of send function
	if (Result == SOCKET_ERROR)
	{
		printf("send failed with error: %d\n", WSAGetLastError());
		closesocket(contestant.clientSocket);
		WSACleanup();
		return 1;
	}

	do
	{
		int iResult = recv(contestant.clientSocket, dataBuffer, BUFFER_SIZE, 0);

		if (iResult > 0)	// Check if message is successfully received
		{
			dataBuffer[iResult] = '\0';

			printf("Client sent: %s.\n", dataBuffer);

			strcpy(contestants[contestant.id].name, dataBuffer);
			//printf("The name is %s", contestant.name);
			break;

		}
		else if (iResult == 0)	// Check if shutdown command is received
		{
			// Connection was closed successfully
			printf("Connection with client closed.\n");
			closesocket(contestant.clientSocket);
		}
		else	// There was an error during recv
		{

			printf("recv failed with error: %d\n", WSAGetLastError());
			closesocket(contestant.clientSocket);
		}

	} while (true);

	/*x++;*/

}

DWORD WINAPI handle_connection(LPVOID lpParam) {
	CONTESTANT contestant = *(CONTESTANT*)lpParam;
	//contestants[contestant.id].points = 0;
	int shot = 0;
	char res[256];
	res[0] = 0;
	//double xf = 0.0;
	srand(time(NULL));
	if (!contestant.disq) {
		char dataBuffer[BUFFER_SIZE];
		
		char* message = "pucaj!";
		int Result = send(contestant.clientSocket, message, (int)strlen(message), 0);

		// Check result of send function
		if (Result == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(contestant.clientSocket);
			WSACleanup();
			return 1;
		}


		while (shot < limit)
		{
			int iResult = recv(contestant.clientSocket, dataBuffer, BUFFER_SIZE, 0);

			if (iResult > 0)	// Check if message is successfully received
			{

				char sh[30];
				strncpy(sh, dataBuffer, 9);
				sh[9] = 0;
				dataBuffer[iResult] = '\0';
				if (strcmp(sh, "pucao sam") == 0) {
					/*	printf("%d", strlen(dataBuffer));*/
					printf("%s sent %s\n", contestant.name, sh);

					double r = calc_points(MIN, MAX);


					contestants[contestant.id].points += r;
					sprintf(res, "%lf", contestants[contestant.id].points);
					strcat(res, " Points");
					int Result = send(contestant.clientSocket, res, (int)strlen(res), 0);
					if (Result == SOCKET_ERROR)
					{
						printf("send failed with error: %d\n", WSAGetLastError());
						closesocket(contestant.clientSocket);
						WSACleanup();
						return 1;
					}
					Sleep(1000);
					shot++;

				}
				else {
					printf("%s sent: %s.\n", contestant.name, dataBuffer);

				}

			}
			else if (iResult == 0)	// Check if shutdown command is received
			{
				// Connection was closed successfully
				printf("Connection with client closed.\n");
				closesocket(contestant.clientSocket);
			}
			else	// There was an error during recv
			{

				printf("recv failed with error: %d\n", WSAGetLastError());
				closesocket(contestant.clientSocket);
			}

			//contestants[contestant.id].points += xf;

		}



	}
}

DWORD WINAPI handle_comp(LPVOID lpParam) {
	int winner = 0;
	int placement = MAX_CLIENTS + 1;
	do {
		WaitForSingleObject(intr_semaphore, INFINITE);

		for (round; round < MAX_CLIENTS - 1; round++) {
			/*round = i;*/
			placement--;
			switch (round) {
			case 0:
				limit = 6;
				break;
			case 1:
				limit = 2;
				break;
			case 2:
				limit = 2;
				break;
			case 3:
				limit = 2;
				break;
			case 4:
				limit = 2;
				break;
			case 5:
				limit = 3;
				break;
			case 6:
				limit = 3;
				break;
			default:
				limit = 2;
				break;
			}

			for(int i = 0; i < MAX_CLIENTS; i++){
				if (!contestants[i].disq) {
					
					hHandle[i] = CreateThread(NULL, 0, &handle_connection, &contestants[i], 0, &handle_ID[i]);
					WaitForSingleObject(hHandle[i], INFINITE);
					CloseHandle(hHandle[i]);

					Sleep(3000);

				}
			}
			
			Sleep(3000);
			for (int i = 0; i < MAX_CLIENTS; i++) {
				printf("%s ima ovoliko poena %lf\n", contestants[i].name, contestants[i].points);
			}
			double min = 0;
			int minId = 0;
			
			while (minId < MAX_CLIENTS) {
				if (!contestants[minId].disq) {
					min = contestants[minId].points;
					break;
				}
				else {
					minId++;
				}
			}

			for (int x = 0; x < MAX_CLIENTS; x++) {
				if (!contestants[x].disq && contestants[x].points < min) {
					min = contestants[x].points;
					minId = x;
				}
			}
		
			int winner = 0;
		
			

			char* exit = "\nYou have been eliminated!";
			
			
			int Result = send(contestants[minId].clientSocket, exit, (int)strlen(exit), 0);
			if (Result == SOCKET_ERROR)
			{
				printf("send failed with error: %d\n", WSAGetLastError());
				closesocket(contestants[minId].clientSocket);
				WSACleanup();
				return 1;
			}
			contestants[minId].disq = TRUE;
			contestants[minId].placement = placement;
			printf("%s placed: %d.\n", contestants[minId].name, contestants[minId].placement);
			active_players -= 1;
			if (active_players <= 1) {
				ReleaseSemaphore(res_semaphore, 1, NULL);
				break;
			}
			Sleep(5000);

		}
		
		CloseHandle(intr_semaphore);

	} while (active_players > 1);

	return true;
}

DWORD WINAPI handle_results(LPVOID lpParam) {
	int winner = 0;

	while (true) {
		WaitForSingleObject(res_semaphore, INFINITE);
		for (int i = 0; i < MAX_CLIENTS; i++) {
			
			if (!contestants[i].disq) {
				winner = i;
			}

		}
		printf("The winner is %s\n", contestants[winner].name);
		char* win = "\nYou won!";
		int Result = send(contestants[winner].clientSocket, win, (int)strlen(win), 0);
		if (Result == SOCKET_ERROR)
		{
			printf("send failed with error: %d\n", WSAGetLastError());
			closesocket(contestants[winner].clientSocket);
			WSACleanup();
			return 1;
		}
		Sleep(2000);
		char* gameOver = "Competition is over. Thanks for participating";
		for (int j = 0; j < MAX_CLIENTS; j++) {
			int Result = send(contestants[j].clientSocket, gameOver, (int)strlen(gameOver), 0);

		}
		
	}
}

double calc_points(double min, double max) {

	return (max - min) * ((double)rand() / (double)RAND_MAX) + min;
}
