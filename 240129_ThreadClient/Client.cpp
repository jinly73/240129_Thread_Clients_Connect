#define _WINSOCK_DEPRECATED_NO_WARNINGS

#include <winsock2.h>
#include <windows.h>
#include <process.h>
#include <iostream>
#include <map>
#include <conio.h>

#include "Packet.h"

#pragma comment(lib, "ws2_32")

using namespace std;

SOCKET ServerSocket;

map<SOCKET, Player> SessionList; // SOCKET를 쓴 이유는 Clien 번호가 겹치지 않게 하기 위함

int MyPlayerID = 0;
bool bIsRunnging = true;


unsigned WINAPI SendThread(void* Arg)
{
	srand((unsigned int)time(nullptr));

	char Message[1024] = { 0, };
	//나 로그인 한다. Server, Req
	PacketManager::PlayerData.ID = 0;
	PacketManager::PlayerData.X = 0;
	PacketManager::PlayerData.Y = 0;
	PacketManager::Type = EPacketType::C2S_Login;
	PacketManager::Size = 12; //Data + Header

	PacketManager::MakePacket(Message);

	int SendLength = send(ServerSocket, Message, PacketManager::Size + 4, 0);


	while (true)
	{
		int KeyCode;
		KeyCode = _getch();
		//std::cin.getline(Message, sizeof(Message));

		if (KeyCode == 'q')
		{
			//C2S_Logut send
			closesocket(ServerSocket);
			bIsRunnging = false;
			return 0;
		}

		//Move
		char SendData[1024] = { 0, };
		PacketManager::PlayerData = SessionList[MyPlayerID];
		PacketManager::PlayerData.X = KeyCode;
		PacketManager::Type = EPacketType::C2S_Move;
		PacketManager::Size = 12;
		PacketManager::MakePacket(SendData);

		int SendLength = send(ServerSocket, SendData, PacketManager::Size + 4, 0);

	}


	return 0;
}

unsigned WINAPI RecvThread(void* Arg)
{
	while (true)
	{
		char Header[4] = { 0, };
		int RecvLength = recv(ServerSocket, Header, 4, MSG_WAITALL);
		if (RecvLength < 0)
		{

		}

		unsigned short DataSize = 0;
		EPacketType PacketType = EPacketType::Max;

		//disassemble header
		memcpy(&DataSize, &Header[0], 2);
		memcpy(&PacketType, &Header[2], 2);

		DataSize = ntohs(DataSize);
		PacketType = (EPacketType)(ntohs((u_short)PacketType));

		char Data[1024] = { 0, };

		//Data
		RecvLength = recv(ServerSocket, Data, DataSize, MSG_WAITALL);
		if (RecvLength > 0)
		{
			//Packet Type
			switch (PacketType)
			{
			case EPacketType::S2C_Login:
			{
				Player NewPlayer;
				memcpy(&NewPlayer.ID, &Data[0], 4);
				memcpy(&NewPlayer.X, &Data[4], 4);
				memcpy(&NewPlayer.Y, &Data[8], 4);

				NewPlayer.ID = ntohl(NewPlayer.ID);
				NewPlayer.X = ntohl(NewPlayer.X);
				NewPlayer.Y = ntohl(NewPlayer.Y);

				SessionList[NewPlayer.ID] = NewPlayer;

				MyPlayerID = NewPlayer.ID;

				cout << "Connect Client : " << SessionList.size() << endl;
			}
			break;

			case EPacketType::S2C_Spawn:
			{
				Player NewPlayer;
				memcpy(&NewPlayer.ID, &Data[0], 4);
				memcpy(&NewPlayer.X, &Data[4], 4);
				memcpy(&NewPlayer.Y, &Data[8], 4);

				NewPlayer.ID = ntohl(NewPlayer.ID);
				NewPlayer.X = ntohl(NewPlayer.X);
				NewPlayer.Y = ntohl(NewPlayer.Y);

				if (SessionList.find(NewPlayer.ID) == SessionList.end())
				{
					SessionList[NewPlayer.ID] = NewPlayer;
				}

				cout << "Spawn Client" << endl;
			}
			break;

			case EPacketType::S2C_Logout:
			{
				Player DisconnectPlayer;
				memcpy(&DisconnectPlayer.ID, &Data[0], 4);
				memcpy(&DisconnectPlayer.X, &Data[4], 4);
				memcpy(&DisconnectPlayer.Y, &Data[8], 4);

				DisconnectPlayer.ID = ntohl(DisconnectPlayer.ID);
				DisconnectPlayer.X = ntohl(DisconnectPlayer.X);
				DisconnectPlayer.Y = ntohl(DisconnectPlayer.Y);

				SessionList.erase(DisconnectPlayer.ID);

				cout << "disconnect Client" << endl;
			}
			case EPacketType::S2C_Move:
			{
				Player MovePlayer;
				memcpy(&MovePlayer.ID, &Data[0], 4);
				memcpy(&MovePlayer.X, &Data[4], 4);
				memcpy(&MovePlayer.Y, &Data[8], 4);

				MovePlayer.ID = ntohl(MovePlayer.ID);
				MovePlayer.X = ntohl(MovePlayer.X);
				MovePlayer.Y = ntohl(MovePlayer.Y);

				SessionList[MovePlayer.ID] = MovePlayer;
			}
			break;
			}

			system("cls");
			for (const auto& Data : SessionList)
			{
				COORD Cur;
				Cur.X = Data.second.X;
				Cur.Y = Data.second.Y;
				SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), Cur);

				cout << Data.second.ID << endl;
			}
		}
	}

	return 0;
}

int main()
{
	WSAData wsaData;
	WSAStartup(MAKEWORD(2, 2), &wsaData);

	ServerSocket = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP);

	SOCKADDR_IN ServerSockAddr = { 0 , };
	ServerSockAddr.sin_family = AF_INET;
	ServerSockAddr.sin_addr.s_addr = inet_addr("127.0.0.1");
	ServerSockAddr.sin_port = htons(22222);

	connect(ServerSocket, (SOCKADDR*)&ServerSockAddr, sizeof(ServerSockAddr));

	HANDLE ThreadHandles[2];

	ThreadHandles[0] = (HANDLE)_beginthreadex(0, 0, SendThread, 0, 0, 0);
	ThreadHandles[1] = (HANDLE)_beginthreadex(0, 0, RecvThread, 0, 0, 0);

	while (bIsRunnging)
	{
		Sleep(1);
	}

	closesocket(ServerSocket);

	WSACleanup();

	return 0;
}