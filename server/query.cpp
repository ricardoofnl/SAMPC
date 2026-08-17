/*

	SA:MP Multiplayer Modification
	Copyright 2004-2005 SA:MP Team

    Version: $Id: query.cpp,v 1.16 2006/05/08 13:28:46 kyeman Exp $

*/

#include "main.h"

bool bRconSocketReply = false;

SOCKET	cur_sock			= INVALID_SOCKET;
char*	cur_data			= NULL;
int		cur_datalen			= 0;
sockaddr_in to;

void RconSocketReply(char* szMessage)
{
	// IMPORTANT!
	// Don't use logprintf from here... You'll cause an infinite loop.
	if (bRconSocketReply)
	{
		int message_len = strlen(szMessage);
		char* newdata = (char*)malloc(message_len + cur_datalen + sizeof(WORD));
		if (newdata == NULL) return;
		char* keep_ptr = newdata;
		memcpy(newdata, cur_data, cur_datalen);
		newdata += cur_datalen;
		*(WORD*)newdata = (WORD)message_len;
		newdata += sizeof(WORD);
		memcpy(newdata, szMessage, message_len);
		newdata += message_len;
		sendto(cur_sock, keep_ptr, (newdata - keep_ptr), 0, (sockaddr*)&to, sizeof(to));
		free(keep_ptr);
	}
}

// the browser counts what it can see, so npcs and empty slots are skipped
static WORD GetQueryPlayerCount()
{
	CPlayerPool* pPlayerPool = pNetGame ? pNetGame->GetPlayerPool() : NULL;
	if (!pPlayerPool) return 0;

	WORD wCount = 0;
	for (int i = 0; i < MAX_PLAYERS; i++)
		if (pPlayerPool->GetSlotState(i)) wCount++;

	return wCount;
}

// a length prefixed string, the browser reads the count first and the bytes are
// not terminated
static char* WriteQueryString(char* pWrite, const char* szText, bool bDwordLength)
{
	size_t uiLen = szText ? strlen(szText) : 0;
	if (uiLen > 255 && !bDwordLength) uiLen = 255;

	if (bDwordLength)
	{
		DWORD dwLen = (DWORD)uiLen;
		memcpy(pWrite, &dwLen, sizeof(DWORD));
		pWrite += sizeof(DWORD);
	}
	else
	{
		BYTE byteLen = (BYTE)uiLen;
		memcpy(pWrite, &byteLen, sizeof(BYTE));
		pWrite += sizeof(BYTE);
	}

	if (uiLen)
	{
		memcpy(pWrite, szText, uiLen);
		pWrite += uiLen;
	}
	return pWrite;
}

//----------------------------------------------------------------------------------
// 'i', what the browser puts in the server list row and the info panel

static void SendServerInfo(SOCKET s, char* data, const sockaddr_in* to, int tolen)
{
	char* szHostName = pConsole->GetStringVariable("hostname");
	char* szGameMode = pConsole->GetStringVariable("gamemodetext");
	char* szMapName = pConsole->GetStringVariable("mapname");
	char* szPassword = pConsole->GetStringVariable("password");

	if (!szHostName) szHostName = (char*)"";
	if (!szGameMode) szGameMode = (char*)"";
	if (!szMapName) szMapName = (char*)"";

	size_t uiSize = 11 + 1 + 2 + 2 +
		(4 + strlen(szHostName)) + (4 + strlen(szGameMode)) + (4 + strlen(szMapName));

	char* pReply = (char*)malloc(uiSize);
	if (!pReply) return;

	char* pWrite = pReply;

	// the browser matches the reply to a row by the header it sent
	memcpy(pWrite, data, 11);
	pWrite += 11;

	BYTE bytePassworded = (szPassword && szPassword[0] != '\0') ? 1 : 0;
	memcpy(pWrite, &bytePassworded, sizeof(BYTE));
	pWrite += sizeof(BYTE);

	WORD wPlayers = GetQueryPlayerCount();
	memcpy(pWrite, &wPlayers, sizeof(WORD));
	pWrite += sizeof(WORD);

	WORD wMaxPlayers = (WORD)pConsole->GetIntVariable("maxplayers");
	memcpy(pWrite, &wMaxPlayers, sizeof(WORD));
	pWrite += sizeof(WORD);

	pWrite = WriteQueryString(pWrite, szHostName, true);
	pWrite = WriteQueryString(pWrite, szGameMode, true);
	pWrite = WriteQueryString(pWrite, szMapName, true);

	sendto(s, pReply, (int)(pWrite - pReply), 0, (sockaddr*)to, tolen);
	free(pReply);
}

//----------------------------------------------------------------------------------
// 'c', the name and score list behind the players tab

static void SendClientList(SOCKET s, char* data, const sockaddr_in* to, int tolen)
{
	CPlayerPool* pPlayerPool = pNetGame ? pNetGame->GetPlayerPool() : NULL;
	if (!pPlayerPool) return;

	WORD wPlayers = GetQueryPlayerCount();

	// one datagram only, the original stops answering rather than fragment
	if (wPlayers > 100) return;

	size_t uiSize = 11 + 2 + ((size_t)wPlayers * (1 + MAX_PLAYER_NAME + 4));

	char* pReply = (char*)malloc(uiSize);
	if (!pReply) return;

	char* pWrite = pReply;

	memcpy(pWrite, data, 11);
	pWrite += 11;

	memcpy(pWrite, &wPlayers, sizeof(WORD));
	pWrite += sizeof(WORD);

	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (!pPlayerPool->GetSlotState(i)) continue;

		CPlayer* pPlayer = pPlayerPool->GetAt(i);
		if (!pPlayer) continue;

		pWrite = WriteQueryString(pWrite, pPlayer->GetName(), false);

		DWORD dwScore = (DWORD)pPlayer->m_iScore;
		memcpy(pWrite, &dwScore, sizeof(DWORD));
		pWrite += sizeof(DWORD);
	}

	sendto(s, pReply, (int)(pWrite - pReply), 0, (sockaddr*)to, tolen);
	free(pReply);
}

//----------------------------------------------------------------------------------

int ProcessQueryPacket(unsigned int binaryAddress, unsigned short port, char* data, int length, SOCKET s)
{
	// Expecting atleast 10 bytes long data, starting first 4 bytes with "SAMP"
	if (length >= 11 && *(unsigned int*)data == 0x504D4153) {

		// Tell the user someone sent a request, if "logqueries" enabled
		if (bQueryLogging) {
			in_addr in;
			in.s_addr = binaryAddress;
			logprintf("[query:%c] from %s:%d", data[10], inet_ntoa(in), port);
		}

		if (pConsole)
		{
			memset(&to, 0, sizeof(to));
			to.sin_family = AF_INET;
			to.sin_addr.s_addr = binaryAddress;
			to.sin_port = htons(port);

			// RconSocketReply builds on these
			cur_sock = s;
			cur_data = data;
			cur_datalen = 11;

			switch (data[10])
			{
			case 'i':
				SendServerInfo(s, data, &to, sizeof(to));
				break;
			case 'c':
				SendClientList(s, data, &to, sizeof(to));
				break;
			case 'r':
				pConsole->SendRules(s, data, &to, sizeof(to));
				break;
			case 'p':
				// the browser timed the round trip itself, hand its tick back
				if (length >= 15)
					sendto(s, data, 15, 0, (sockaddr*)&to, sizeof(to));
				break;
			}
		}

		// Data was in fact query request
		return 1;
	}
	return 0;
}
