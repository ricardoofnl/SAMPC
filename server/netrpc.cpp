/*

	SA:MP Multiplayer Modification
	Copyright 2004-2005 SA:MP Team

    Version: $Id: netrpc.cpp,v 1.52 2006/06/02 13:24:21 mike Exp $

*/

#include "main.h"
//#include "vehmods.h"
//#include "anticheat.h"

#include <ttmath/ttmath.h>

RakServerInterface		*pRak=0;

// Removed for RakNet upgrade
//#define REGISTER_STATIC_RPC REGISTER_AS_REMOTE_PROCEDURE_CALL
//#define UNREGISTER_STATIC_RPC UNREGISTER_AS_REMOTE_PROCEDURE_CALL

//----------------------------------------------------
// Sent by a client who's wishing to join us in our
// multiplayer-like activities.

void ClientJoin(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams);
	RakNet::BitStream bsReject;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	CHAR szPlayerName[MAX_PLAYER_NAME];
	WORD wPlayerID;
	int  iVersion=0;
	BYTE byteMod=0;
	BYTE byteNickLen=0;
	BYTE byteRejectReason;
	BYTE byteVersionLen=0;
	CHAR szVersion[MAX_VERSION_NAME]="unknown";
	unsigned int uiChallengeResponse=0;
	PlayerID otherPlayerId = UNASSIGNED_PLAYER_ID;
	int iCount=0;
	BYTE byteSerialLen=0;
	CHAR szSerial[100];
	std::string serial;
	in_addr in;

	wPlayerID = pRak->GetIndexFromPlayerID(sender);
	PlayerID MyPlayerID = pRak->GetPlayerIDFromIndex(wPlayerID);

	memset(szPlayerName, 0, sizeof(szPlayerName));
	memset(szSerial, 0, sizeof(szSerial));

	if (UNASSIGNED_PLAYER_ID == MyPlayerID)
	{
		in.s_addr = sender.binaryAddress;
		logprintf("(0) Invalid client connecting from %s", inet_ntoa(in));
		pRak->Kick(MyPlayerID);
		return;
	}

	if (!pRak->IsActivePlayerID(sender) || wPlayerID > MAX_PLAYERS)
	{
		byteRejectReason = REJECT_REASON_BAD_PLAYERID;
		bsReject.Write(byteRejectReason);
		pRak->RPC(RPC_ConnectionRejected,&bsReject,HIGH_PRIORITY,RELIABLE,0,sender,false,false);
		pRak->Kick(sender);
		return;
	}

	if (pPlayerPool->GetSlotState(wPlayerID))
	{
		logprintf("Player slot (%u) is taken. Kicking %s.", wPlayerID, sender.ToString());
		pNetGame->KickPlayer(wPlayerID);
		return;
	}

	for (int i = 0; i < MAX_PLAYERS; i++) {
		if (pPlayerPool->GetSlotState(i)) {
			otherPlayerId = pRak->GetPlayerIDFromIndex(i);
			if (otherPlayerId != UNASSIGNED_PLAYER_ID &&
				otherPlayerId.binaryAddress == MyPlayerID.binaryAddress)
				iCount++;
		}
	}

	if (pConsole->GetIntVariable("maxplayerperip") <= iCount)
	{
		byteRejectReason = REJECT_REASON_IP_LIMIT_REACHED;
		bsReject.Write(byteRejectReason);
		pRak->RPC(RPC_ConnectionRejected, &bsReject, HIGH_PRIORITY, RELIABLE, 0, sender, false, false);
		pRak->Kick(sender);
		return;
	}

	if (!bsData.Read(iVersion) || iVersion != NETGAME_VERSION)
	{
		byteRejectReason = REJECT_REASON_BAD_VERSION;
		bsReject.Write(byteRejectReason);
		pRak->RPC(RPC_ConnectionRejected, &bsReject, HIGH_PRIORITY, RELIABLE, 0, sender, false, false);
		pRak->Kick(sender);
		return;
	}

	if (!bsData.Read(byteMod) || byteMod != pNetGame->m_byteMod)
	{
		byteRejectReason = REJECT_REASON_BAD_MOD;
		bsReject.Write(byteRejectReason);
		pRak->RPC(RPC_ConnectionRejected,&bsReject,HIGH_PRIORITY,RELIABLE,0,sender,false,false);
		pRak->Kick(sender);
		return;
	}

	if (!bsData.Read(byteNickLen) || (byteNickLen < 3 || byteNickLen > 20) ||
		!bsData.Read(szPlayerName, byteNickLen), szPlayerName[byteNickLen] = '\0' ||
		ContainsInvalidNickChars(szPlayerName) || pPlayerPool->IsNickInUse(szPlayerName))
	{
		byteRejectReason = REJECT_REASON_BAD_NICKNAME;
		bsReject.Write(byteRejectReason);
		pRak->RPC(RPC_ConnectionRejected,&bsReject,HIGH_PRIORITY,RELIABLE,0,sender,false,false);
		pRak->Kick(sender);
		return;
	}
	
	if (!bsData.Read(uiChallengeResponse) || _uiRndSrvChallenge != (uiChallengeResponse ^ NETGAME_VERSION))
	{
		byteRejectReason = REJECT_REASON_BAD_VERSION;
		bsReject.Write(byteRejectReason);
		pRak->RPC(RPC_ConnectionRejected,&bsReject,HIGH_PRIORITY,RELIABLE,0,sender,false,false);
		pRak->Kick(sender);
		return;
	}
	
	if (!bsData.Read(byteSerialLen) || byteSerialLen == 0 || byteSerialLen >= 50)
	{
		in.s_addr = sender.binaryAddress;
		logprintf("(0) Invalid client connecting from %s", inet_ntoa(in));
		pRak->Kick(sender);
		return;
	}

	bool bSerialCheckFailed = true;
	if (bsData.Read(szSerial, byteSerialLen) && szSerial[0] != '\0')
	{
		szSerial[byteSerialLen] = 0;

		ttmath::UInt<100> m;
		ttmath::uint remainder = 0;

		m.FromString(szSerial, 16);
		m.DivInt(1001, remainder);

		if (!remainder)
		{
			serial = m.ToString(16);
			if (!serial.empty() && serial.length() < 50)
			{
				bSerialCheckFailed = false;
			}
		}
	}

	if (bSerialCheckFailed)
	{
		in.s_addr = sender.binaryAddress;
		logprintf("Invalid client connecting from %s", inet_ntoa(in));
		pRak->Kick(MyPlayerID);
		return;
	}

	if (!bsData.Read(byteVersionLen) || byteVersionLen >= MAX_VERSION_NAME)
	{
		in.s_addr = sender.binaryAddress;
		logprintf("(1) Invalid client connecting from %s", inet_ntoa(in));
		pRak->Kick(MyPlayerID);
		return;
	}

	if (bsData.Read(szVersion, byteVersionLen))
	{
		szVersion[byteVersionLen] = '\0';
	}

	// Add this client to the player pool.
	if(!pPlayerPool->New(wPlayerID, szPlayerName, (char*)serial.c_str(), szVersion)) {
		pRak->Kick(sender);
		return;
	}

	pNetGame->ProcessClientJoin(wPlayerID);
}

//----------------------------------------------------
// Sent by client with global chat text

void Chat(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	unsigned char szText[256];
	memset(szText,0,256);

	size_t uiTextLen;

	CPlayerPool *pPool = pNetGame->GetPlayerPool();

	RakNet::BitStream bsData(rpcParams);
	bsData.Read(uiTextLen);

	if(uiTextLen > MAX_CMD_INPUT) return;

	bsData.Read((char *)szText, uiTextLen);
	szText[uiTextLen] = '\0';

	if (!pPool->GetSlotState(pRak->GetIndexFromPlayerID(sender))) return;	

	ReplaceBadChars((char *)szText);

	BYTE bytePlayerID = pRak->GetIndexFromPlayerID(sender);
	CPlayer* pPlayer = pNetGame->GetPlayerPool()->GetAt(bytePlayerID);

	if(pConsole->GetIntVariable("chatlogging"))
		logprintf("[chat] [%s]: %s",
			pPlayer->GetName(),
			szText);

/*#ifdef RAKRCON
	RakNet::BitStream bsSend;

	bsSend.Write( bytePlayerID );
	bsSend.Write( byteTextLen );
	bsSend.Write( szText, byteTextLen );

	pRcon->GetRakServer()->RPC( RPC_Chat, &bsSend, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true, false );
#endif*/

	CGameMode *pGameMode = pNetGame->GetGameMode();
	
	if (pPlayer)
	{	
		// Send OnPlayerText callback to the GameMode script.
		if (pNetGame->GetFilterScripts()->OnPlayerText((cell)bytePlayerID, szText)) {
			if (pGameMode)
			{
				// Comment by spookie:
				//   The CPlayer::Say() call has moved to CGameMode::OnPlayerText(),
				//   when a gamemode is available. This is due to filter scripts.
				pGameMode->OnPlayerText((cell)bytePlayerID, szText);
			} else {
				// No pGameMode
				pPlayer->Say(szText, uiTextLen);
			}
		}
	}
}

//----------------------------------------------------
// Sent by client who wishes to request a class from
// the gamelogic handler.

void RequestClass(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams);

	if(pNetGame->GetGameState() != GAMESTATE_RUNNING) return;
	if(!pNetGame->GetPlayerPool()->GetSlotState(pRak->GetIndexFromPlayerID(sender))) return;
	
	int iRequestedClass = 1;
	BYTE byteRequestOutcome = 0;
	BYTE bytePlayerID = pRak->GetIndexFromPlayerID(sender);
	bsData.Read(iRequestedClass);

	if(iRequestedClass >= MAX_SPAWNS) return;
	if(iRequestedClass >= pNetGame->m_iSpawnsAvailable) return;
    
	CPlayer *pPlayer = pNetGame->GetPlayerPool()->GetAt(bytePlayerID);
	if(pPlayer && (iRequestedClass <= (pNetGame->m_iSpawnsAvailable - 1)))
	{
		pPlayer->SetSpawnInfo(&pNetGame->m_AvailableSpawns[iRequestedClass]);
		//logprintf("SetSpawnInfo - iSkin = %d", pNetGame->m_AvailableSpawns[iRequestedClass].iSkin);
	}

	pNetGame->GetFilterScripts()->OnPlayerRequestClass((cell)bytePlayerID, (cell)iRequestedClass);
	byteRequestOutcome = 1;
	if (pNetGame->GetGameMode()) {
		byteRequestOutcome = pNetGame->GetGameMode()->OnPlayerRequestClass((cell)bytePlayerID, (cell)iRequestedClass);
	}
	
	RakNet::BitStream bsSpawnRequestReply;
	PLAYER_SPAWN_INFO *pSpawnInfo = pPlayer->GetSpawnInfo();

	bsSpawnRequestReply.Write(byteRequestOutcome);
	bsSpawnRequestReply.Write((PCHAR)pSpawnInfo,sizeof(PLAYER_SPAWN_INFO));
	pRak->RPC(RPC_RequestClass,&bsSpawnRequestReply,HIGH_PRIORITY,RELIABLE,0,sender,false,false);
}

//----------------------------------------------------
// Client wants to spawn

void RequestSpawn(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	BYTE bytePlayerID = pRak->GetIndexFromPlayerID(sender);
	BYTE byteRequestOutcome = 1;

	if(pNetGame->GetGameState() != GAMESTATE_RUNNING) return;
	if(!pNetGame->GetPlayerPool()->GetSlotState(pRak->GetIndexFromPlayerID(sender))) return;

	if (!pNetGame->GetFilterScripts()->OnPlayerRequestSpawn((cell)bytePlayerID)) byteRequestOutcome = 0;
	if (pNetGame->GetGameMode() && byteRequestOutcome) {
		if (!pNetGame->GetGameMode()->OnPlayerRequestSpawn((cell)bytePlayerID)) byteRequestOutcome = 0;
	}
	
	RakNet::BitStream bsSpawnRequestReply;
	bsSpawnRequestReply.Write(byteRequestOutcome);
	pRak->RPC(RPC_RequestSpawn,&bsSpawnRequestReply,HIGH_PRIORITY,RELIABLE,0,sender,false,false);
}


//----------------------------------------------------
// Sent by client when they're spawning/respawning.

void Spawn(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	if(pNetGame->GetGameState() != GAMESTATE_RUNNING) return;
	
	BYTE bytePlayerID = pRak->GetIndexFromPlayerID(sender);
	if(!pNetGame->GetPlayerPool()->GetSlotState(bytePlayerID)) return;
	CPlayer	*pPlayer = pNetGame->GetPlayerPool()->GetAt(bytePlayerID);

	// More sanity checks for crashers.
	if(!pPlayer->m_bHasSpawnInfo) return;
	int iSpawnClass = pPlayer->m_SpawnInfo.iSkin;
	if(iSpawnClass < 0 || iSpawnClass > 300) return;

	pPlayer->Spawn();
}

//----------------------------------------------------
// Sent by client when they die.

void Death(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams);

	if(pNetGame->GetGameState() != GAMESTATE_RUNNING) return;

	BYTE bytePlayerID = pRak->GetIndexFromPlayerID(sender);
	if(!pNetGame->GetPlayerPool()->GetSlotState(bytePlayerID)) return;
	CPlayer	*pPlayer = pNetGame->GetPlayerPool()->GetAt(bytePlayerID);

	BYTE byteDeathReason;
	BYTE byteWhoWasResponsible;

	bsData.Read(byteDeathReason);
	bsData.Read(byteWhoWasResponsible);

	if(pPlayer) {
		pPlayer->HandleDeath(byteDeathReason,byteWhoWasResponsible);
	}
}

//----------------------------------------------------
// Sent by client when they want to enter a
// vehicle gracefully.
// This RPC only gets triggered when player vehicle entering task is started,
// not when player already in a vehicle, so they could stop their character entering anytime

void EnterVehicle(RPCParameters* rpcParams)
{
	bool bValidEntering = false;

	// Expecting 2 bytes and 1 bit of data
	if (rpcParams->numberOfBitsOfData == 17) {
		// Cheking if player and vehicle pool is initialized
		if (pNetGame->GetPlayerPool() && pNetGame->GetVehiclePool()) {
			// Checking if player is valid and get object of it
			CPlayer* pPlayer = pNetGame->GetPlayerPool()->GetAt(rpcParams->senderId);
			if (pPlayer != nullptr) {
				RakNet::BitStream bsData(rpcParams);
				VEHICLEID VehicleID = INVALID_VEHICLE;
				bool bPassenger = false;

				bsData.Read(VehicleID);
				bsData.Read(bPassenger);

				// Checking if the vehicle ID is exists
				if (pNetGame->GetVehiclePool()->GetSlotState(VehicleID)) {
					// Notify server and scripts
					pPlayer->EnterVehicle(VehicleID, bPassenger);
					bValidEntering = true;
				}
			}
		}
	}

	// If some of the check is not right, kicking the player from the server
	if (!bValidEntering) {
		logprintf("[SERVER] Kicking player %d for entering an invalid vehicle.", rpcParams->senderId);
		pNetGame->KickPlayer(rpcParams->senderId);
	}
}

//----------------------------------------------------
// Sent by client when they want to exit a
// vehicle gracefully.

void ExitVehicle(RPCParameters *rpcParams)
{
	bool bValidExiting = false;

	// Expecting only 2 bytes of data
	if (rpcParams->numberOfBitsOfData == 16) {
		// Checking if player and vehicle pool is initialized
		if (pNetGame->GetPlayerPool() && pNetGame->GetVehiclePool()) {
			// Checking if the player is valid, and use that object info
			CPlayer* pPlayer = pNetGame->GetPlayerPool()->GetAt(rpcParams->senderId);
			// Also checking if player was actually in a vehicle as driver or a passenger
			if (pPlayer != nullptr && (pPlayer->GetState() == PLAYER_STATE_DRIVER || pPlayer->GetState() == PLAYER_STATE_PASSENGER)) {
				RakNet::BitStream bsData(rpcParams);
				// Expecting only 2 bytes of data
				//if (bsData.GetNumberOfUnreadBits() == 16) {
					VEHICLEID VehicleID;
					bsData.Read(VehicleID);
					// Checking if the actual vehicle is valid
					if (pNetGame->GetVehiclePool()->GetSlotState(VehicleID)) {
						// Notify vehicle exit event
						pPlayer->ExitVehicle(VehicleID);
						bValidExiting = true;
					}
				//}
			}
		}
	}
	// If it was an invalid vehicle exiting, player about to get desyncronized, kicking player out from the server
	if (!bValidExiting) {
		logprintf("[SERVER] Kicking player %d for exiting an invalid vehicle.", rpcParams->senderId);
		pNetGame->KickPlayer(rpcParams->senderId);
	}
}

//----------------------------------------------------

void ServerCommand(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;
	int iStrLen=0;
	unsigned char* szCommand=NULL;

	RakNet::BitStream bsData(rpcParams);

	if(pNetGame->GetGameState() != GAMESTATE_RUNNING) return;

	BYTE bytePlayerID = pRak->GetIndexFromPlayerID(sender);
		
	bsData.Read(iStrLen);

	if(iStrLen < 1) return;
	if(iStrLen > MAX_CMD_INPUT) return;

	szCommand = (unsigned char*)calloc(iStrLen+1,1);
	bsData.Read((char*)szCommand, iStrLen);
	szCommand[iStrLen] = '\0';

	ReplaceBadChars((char *)szCommand);

	if (!pNetGame->GetFilterScripts()->OnPlayerCommandText(bytePlayerID, szCommand))
	{
		if (pNetGame->GetGameMode())
		{
			if (!pNetGame->GetGameMode()->OnPlayerCommandText(bytePlayerID, szCommand))
			{
				pNetGame->SendClientMessage(sender, 0xFFFFFFFF, "SERVER: Unknown command.");
			}
		}
	}

	free(szCommand);
}

//----------------------------------------------------

static void UpdateScoresPingsIPs(RPCParameters* rpcParams)
{
	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	if (!pPlayerPool) return;
	
	CPlayer* pPlayer = pPlayerPool->GetAt(rpcParams->senderId);

	if (!pPlayer || (RakNet::GetTime() - pPlayer->m_nLastPingUpdate) < 2000)
		return;
	
	RakNet::BitStream bsParams;

	int iLastID = pPlayerPool->GetLastPlayerId();
	WORD i = 0;

	if (iLastID != -1)
	{
		while (i <= (WORD)iLastID)
		{
			pPlayer = pPlayerPool->GetAt(i);
			if (!pPlayer) continue;
			
			bsParams.Write(i);
			bsParams.Write(pPlayer->m_iScore);
			// GetLastPing could return -1 when fails, but its only overflow back to 65535
			bsParams.Write(pRak->GetLastPing(pRak->GetPlayerIDFromIndex(i)));
			i++;
		}
	}
	
	// Written, but unused last 2 bytes in client?
	//bsParams.Write((WORD)pPlayerPool->GetPlayerCount());

	pNetGame->SendToPlayer(rpcParams->senderId, RPC_UpdateScoresPingsIPs, &bsParams);

	pPlayer->m_nLastPingUpdate = RakNet::GetTime();
}

//----------------------------------------------------

/*void SvrStats(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams);

	RakNet::BitStream bsParams;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	BYTE bytePlayerId = pRak->GetIndexFromPlayerID(sender);

	if(!pPlayerPool->GetSlotState(bytePlayerId)) return;
	if(!pPlayerPool->GetAt(bytePlayerId)->m_bIsAdmin) return;

	bsParams.Write((const char *)pRak->GetStatistics(UNASSIGNED_PLAYER_ID),sizeof(RakNetStatisticsStruct));
	pRak->RPC(RPC_SvrStats, &bsParams, HIGH_PRIORITY, RELIABLE, 0, sender, false, false);
}*/

//----------------------------------------------------

void SetInteriorId(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams);

	BYTE byteInteriorId;
	bsData.Read(byteInteriorId);

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	BYTE bytePlayerId = pRak->GetIndexFromPlayerID(sender);

	if (pPlayerPool->GetSlotState(bytePlayerId))
	{
		CGameMode *pGameMode = pNetGame->GetGameMode();
		CFilterScripts *pFilters = pNetGame->GetFilterScripts();

		CPlayer *pPlayer = pPlayerPool->GetAt(bytePlayerId);
		int iOldInteriorId=pPlayer->m_iInteriorId;
		pPlayer->m_iInteriorId = (int)byteInteriorId;

		if(pGameMode) pGameMode->OnPlayerInteriorChange(
			bytePlayerId,pPlayer->m_iInteriorId,iOldInteriorId);

		if(pFilters) pFilters->OnPlayerInteriorChange(
			bytePlayerId,pPlayer->m_iInteriorId,iOldInteriorId);
	}
}

//----------------------------------------------------

void ScmEvent(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams);
	RakNet::BitStream bsSend;
	BYTE bytePlayerID;
	int iEvent;
	DWORD dwParams1;
	DWORD dwParams2;
	DWORD dwParams3;
	
	bytePlayerID = pNetGame->GetRakServer()->GetIndexFromPlayerID(sender);
	bsData.Read(iEvent);
	bsData.Read(dwParams1);
	bsData.Read(dwParams2);
	bsData.Read(dwParams3);
	
	bool bSend = true;

	//printf("ScmEvent: %u %u %u %u\n",iEvent,dwParams1,dwParams2,dwParams3);
    
	if(pNetGame->GetGameState() != GAMESTATE_RUNNING) return;

	if (iEvent == EVENT_TYPE_CARCOMPONENT)
	{
		CVehicle* pVehicle = pNetGame->GetVehiclePool()->GetAt((VEHICLEID)dwParams1);
		if (!pVehicle) return;

		if (!pNetGame->GetGameMode()->OnVehicleMod(bytePlayerID, dwParams1, dwParams2) ||
			!pNetGame->GetFilterScripts()->OnVehicleMod(bytePlayerID, dwParams1, dwParams2))
		{
			bSend = false;
		}

		if (bSend == true)
		{
			int iComponentId = Utils::GetTypeByComponentId(dwParams2);
			if (iComponentId == -1)
			{
				bSend = false;
			}
			else
			{
				pVehicle->m_CarModInfo.ucCarMod[iComponentId] = (unsigned char)(dwParams2 - 1000);
			}
		}

		if (bSend)
		{
			bsSend.Write(bytePlayerID);
			bsSend.Write(iEvent);
			bsSend.Write(dwParams1);
			bsSend.Write(dwParams2);
			bsSend.Write(dwParams3);
			pRak->RPC(RPC_ScmEvent, &bsSend, HIGH_PRIORITY, RELIABLE, 0, sender, true, false);
		}
		else
		{
			bsSend.Write((VEHICLEID)dwParams1);
			bsSend.Write(dwParams2);
			pRak->RPC(RPC_ScrRemoveComponent, &bsSend, HIGH_PRIORITY, RELIABLE, 0, sender, false, false);
		}
	}
	else if (iEvent == EVENT_TYPE_PAINTJOB)
	{
		CVehicle*	pVehicle	=	pNetGame->GetVehiclePool()->GetAt((VEHICLEID)dwParams1);
		if (!pVehicle) return;

		if (!pNetGame->GetGameMode()->OnVehiclePaintjob(bytePlayerID, dwParams1, dwParams2) ||
		!pNetGame->GetFilterScripts()->OnVehiclePaintjob(bytePlayerID, dwParams1, dwParams2)) bSend = false;
		if (bSend)
		{
			pVehicle->m_CarModInfo.bytePaintJob = (BYTE)dwParams2;

			bsSend.Write(bytePlayerID);
			bsSend.Write(iEvent);
			bsSend.Write(dwParams1);
			bsSend.Write(dwParams2);
			bsSend.Write(dwParams3);
			pRak->RPC(RPC_ScmEvent, &bsSend, HIGH_PRIORITY, RELIABLE, 0, sender, true, false);
		}
	}
	else if (iEvent == EVENT_TYPE_CARCOLOR)
	{
		CVehicle*	pVehicle	=	pNetGame->GetVehiclePool()->GetAt((VEHICLEID)dwParams1);
		if (!pVehicle)
			return;

		if (!pNetGame->GetGameMode()->OnVehicleRespray(bytePlayerID, dwParams1, dwParams2, dwParams3) ||
		!pNetGame->GetFilterScripts()->OnVehicleRespray(bytePlayerID, dwParams1, dwParams2, dwParams3)) bSend = false;
		if (bSend)
		{
			pVehicle->m_CarModInfo.iColor0 = (int)dwParams2;
			pVehicle->m_CarModInfo.iColor1 = (int)dwParams3;

			bsSend.Write(bytePlayerID);
			bsSend.Write(iEvent);
			bsSend.Write(dwParams1);
			bsSend.Write(dwParams2);
			bsSend.Write(dwParams3);
			pRak->RPC(RPC_ScmEvent, &bsSend, HIGH_PRIORITY, RELIABLE, 0, sender, true, false);
		}
	}
	else if (iEvent == EVENT_ENTEREXIT_MODSHOP)
	{
		if (!pNetGame->GetPlayerPool()->GetSlotState(dwParams1))
			return;

		if (pNetGame->GetFilterScripts())
			pNetGame->GetFilterScripts()->OnEnterExitModShop(bytePlayerID, dwParams2, dwParams3);
		if (pNetGame->GetGameMode())
			pNetGame->GetGameMode()->OnEnterExitModShop(bytePlayerID, dwParams2, dwParams3);

		bsSend.Write(bytePlayerID);
		bsSend.Write(iEvent);
		bsSend.Write(dwParams1);
		bsSend.Write(dwParams2);
		bsSend.Write(dwParams3);
		pRak->RPC(RPC_ScmEvent, &bsSend, HIGH_PRIORITY, RELIABLE, 0, sender, true, false);
	}
	else if (iEvent == EVENT_TYPE_STUNT_JUMP)
	{
		//if (!pNetGame->GetVehiclePool()->GetSlotState(dwParams1))
			//return;

		if (pNetGame->GetFilterScripts())
			pNetGame->GetFilterScripts()->OnPlayerStunt(bytePlayerID, dwParams1);
		if (pNetGame->GetGameMode())
			pNetGame->GetGameMode()->OnPlayerStunt(bytePlayerID, dwParams1);
	}
	/*else 
	{
		bsSend.Write(bytePlayerID);
		bsSend.Write(iEvent);
		bsSend.Write(dwParams1);
		bsSend.Write(dwParams2);
		bsSend.Write(dwParams3);
		pRak->RPC(RPC_ScmEvent, &bsSend, HIGH_PRIORITY, RELIABLE, 0, sender, true, false);
	}*/
}

void AdminMapTeleport(RPCParameters *rpcParams)
{
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams);
	
	VECTOR vecPos;
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);

	BYTE bytePlayerId = pRak->GetIndexFromPlayerID(sender);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	if (!pNetGame) return;
	if (pNetGame->GetGameState() != GAMESTATE_RUNNING) return;

	if (pPlayerPool->GetSlotState(bytePlayerId)) {
		CPlayer* pPlayer = pPlayerPool->GetAt(bytePlayerId);
		if (!pPlayer)
			return;

		if (!pNetGame->GetGameMode()->OnPlayerClickMap(
			(cell)bytePlayerId, vecPos.X, vecPos.Y, vecPos.Z))
		{
			pNetGame->GetFilterScripts()->OnPlayerClickMap((cell)bytePlayerId,
				vecPos.X, vecPos.Y, vecPos.Z);
		}

		if (pPlayer->m_bCanTeleport || pNetGame->m_bAdminTeleport && pPlayer->m_bIsAdmin)
		{
			RakNet::BitStream bsParams;
			bsParams.Write(vecPos.X);	// X
			bsParams.Write(vecPos.Y);	// Y
			bsParams.Write(vecPos.Z);	// Z

			RakServerInterface* pRak = pNetGame->GetRakServer();
			pRak->RPC(RPC_ScrSetPlayerPos, &bsParams, HIGH_PRIORITY, RELIABLE, 0,
				sender, false, false);
		}
	}
}

// VehicleDestroyed triggers, when a vehicle (exluding boats) fully sunk in water, or vehicle
// health reported as 0.0f, and incoming from that player who used it as a driver last time.
void VehicleDestroyed(RPCParameters *rpcParams)
{
	// TODO: Add checks for last driver maybe, that may could fix a backdoor,
	// so player cannot make all vehicles dead in a single time.
	PlayerID sender = rpcParams->sender;

	RakNet::BitStream bsData(rpcParams);
	VEHICLEID VehicleID;
	bsData.Read(VehicleID);

	if(!pNetGame) return;
	if(pNetGame->GetGameState() != GAMESTATE_RUNNING) return;

	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	if(!pPlayerPool || !pVehiclePool) return;

	BYTE bytePlayerId = pRak->GetIndexFromPlayerID(sender);
	if(!pPlayerPool->GetSlotState(bytePlayerId)) return;

	if(pVehiclePool->GetSlotState(VehicleID))
	{
		CVehicle* pVehicle = pVehiclePool->GetAt(VehicleID);
		if (pVehicle) {
			pVehicle->SetDead();
			pVehicle->m_ucKillerID = bytePlayerId;
		}
	}
}

void PickedUpWeapon(RPCParameters *rpcParams)
{
	// Tells all other clients to destroy this pickup as it's been got already
	RakNet::BitStream bsData(rpcParams);

	BYTE bytePlayerID;
	bsData.Read(bytePlayerID);

	RakNet::BitStream bsSend;
	bsSend.Write(bytePlayerID);
	
	pRak->RPC(RPC_DestroyWeaponPickup, &bsSend, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true, false);
}

void PickedUpPickup(RPCParameters *rpcParams)
{
	BYTE bytePlayerID = pRak->GetIndexFromPlayerID(rpcParams->sender);

	RakNet::BitStream bsData(rpcParams);

	int iPickup;
	bsData.Read(iPickup);

	CGameMode *pGameMode = pNetGame->GetGameMode();
	CFilterScripts *pFilters = pNetGame->GetFilterScripts();
	
	if(pGameMode) pGameMode->OnPlayerPickedUpPickup(bytePlayerID, iPickup);
	if(pFilters) pFilters->OnPlayerPickedUpPickup(bytePlayerID, iPickup);
}

void MenuSelect(RPCParameters *rpcParams)
{
	// Checking if the menu pool is initialized
	CMenuPool* pMenuPool = pNetGame->GetMenuPool();
	if (pMenuPool) {
		// Do we have shown menu to player?
		unsigned char ucMenuId = pMenuPool->GetPlayerMenu(rpcParams->senderId);
		if (ucMenuId != INVALID_MENU_ID) {
			// Checking if the actual menu is valid, and get the data of it for validating row
			CMenu* pMenu = pMenuPool->GetAt(ucMenuId);
			if (pMenu != nullptr) {
				RakNet::BitStream bsData(rpcParams);
				// Expecting a 1 byte packet size
				if (bsData.GetNumberOfUnreadBits() == 8) {
					unsigned char ucRow = MAX_MENU_ITEMS;
					bsData.Read(ucRow);
					// Validate menu row, if valid then notify scripts
					if (pMenu->ValidRow(ucRow)) {
						if (pNetGame->GetGameMode())
							pNetGame->GetGameMode()->OnPlayerSelectedMenuRow(rpcParams->senderId, ucRow);
						if (pNetGame->GetFilterScripts())
							pNetGame->GetFilterScripts()->OnPlayerSelectedMenuRow(rpcParams->senderId, ucRow);
					}
				}
			}
		}
		// Reset anyway, even it fails
		pMenuPool->ResetForPlayer(rpcParams->senderId);
	}
}

void MenuQuit(RPCParameters *rpcParams)
{
	// Checking if the menu pool is initialized
	CMenuPool* pMenuPool = pNetGame->GetMenuPool();
	if (pMenuPool) {
		// Do we have shown menu to player?
		unsigned char ucMenuId = pMenuPool->GetPlayerMenu(rpcParams->senderId);
		if (ucMenuId != INVALID_MENU_ID) {
			// Probably already valid at this point, but still making sure..
			//if (pMenuPool->GetSlotState(ucMenuId)) {
				// Notify the scripts
				if (pNetGame->GetGameMode())
					pNetGame->GetGameMode()->OnPlayerExitedMenu(rpcParams->senderId);
				if (pNetGame->GetFilterScripts())
					pNetGame->GetFilterScripts()->OnPlayerExitedMenu(rpcParams->senderId);
			//}
		}
		// Reset menu for player, even if it fails
		pMenuPool->ResetForPlayer(rpcParams->senderId);
	}
}

void TypingEvent(RPCParameters* rpcParams)
{
	if (pNetGame->GetPlayerPool()) {
		CPlayer* pPlayer = pNetGame->GetPlayerPool()->GetAt(rpcParams->senderId);
		if (pPlayer != NULL) {
			RakNet::BitStream in(rpcParams);
			if (in.GetNumberOfUnreadBits() == 1) {
				CFilterScripts* pFS = pNetGame->GetFilterScripts();
				CGameMode* pGM = pNetGame->GetGameMode();
				if (pFS == NULL || pGM == NULL)
					return;

				if (in.ReadBit()) {
					pPlayer->m_bTyping = true;
					pFS->OnPlayerBeginTyping(rpcParams->senderId);
					pGM->OnPlayerBeginTyping(rpcParams->senderId);
				} else {
					pPlayer->m_bTyping = false;
					pFS->OnPlayerEndTyping(rpcParams->senderId);
					pGM->OnPlayerEndTyping(rpcParams->senderId);
				}
			}
		}
	}
}

static void ClientCheck(RPCParameters* rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	if (bsData.GetNumberOfUnreadBits() == 48) {
		unsigned char ucType, ucCheckSum;
		unsigned long ulMemAddress;

		bsData.Read(ucType);
		bsData.Read(ulMemAddress);
		bsData.Read(ucCheckSum);

		if (pNetGame->GetFilterScripts())
			pNetGame->GetFilterScripts()->OnClientCheckResponse(rpcParams->senderId, ucType, ulMemAddress, ucCheckSum);
		if (pNetGame->GetGameMode())
			pNetGame->GetGameMode()->OnClientCheckResponse(rpcParams->senderId, ucType, ulMemAddress, ucCheckSum);
	}
}

static void VehicleDamage(RPCParameters* rpcParams)
{
	CVehiclePool* pVehiclePool;
	CVehicle* pVehicle;
	VEHICLEID VehicleID;
	int iPanels, iDoors;
	unsigned char ucLights, ucWheels;
	
	pVehiclePool = pNetGame->GetVehiclePool();

	if (rpcParams->numberOfBitsOfData == 96 && pVehiclePool)
	{
		RakNet::BitStream bsData(rpcParams);

		bsData.Read(VehicleID);

		pVehicle = pVehiclePool->GetAt(VehicleID);
		if (pVehicle != NULL && pVehicle->m_byteDriverID == (BYTE)rpcParams->senderId)
		{
			bsData.Read(iPanels);
			bsData.Read(iDoors);
			bsData.Read(ucLights);
			bsData.Read(ucWheels);

			pVehicle->UpdateDamage((PLAYERID)rpcParams->senderId, iPanels, iDoors, ucLights, ucWheels);
		}
	}
}

//----------------------------------------------------

static void ActorDamage(RPCParameters* rpcParams)
{
	//bool bUnk;
	unsigned short usPlayerID, usActorID;
	float fDamage;
	int iWeaponID, iBodyPart;

	usPlayerID = (unsigned short)rpcParams->senderId;

	if (pNetGame->GetActorPool() &&
		pNetGame->GetPlayerPool() &&
		rpcParams->numberOfBitsOfData == 112)
	{
		RakNet::BitStream bsData(rpcParams);
	
		if (pNetGame->GetPlayerPool()->GetSlotState(usPlayerID))
		{
			//bsData.Read(bUnk); // Possibly the same as what OnPlayer(Take/Give)Damage would've been
			bsData.Read(usActorID);
			if (pNetGame->GetActorPool()->GetSlotState(usActorID))
			{
				bsData.Read(fDamage);
				bsData.Read(iWeaponID);
				bsData.Read(iBodyPart);

				if (pNetGame->GetFilterScripts())
					pNetGame->GetFilterScripts()->OnPlayerGiveDamageActor(usPlayerID, usActorID, fDamage, iWeaponID, iBodyPart);
				if (pNetGame->GetGameMode())
					pNetGame->GetGameMode()->OnPlayerGiveDamageActor(usPlayerID, usActorID, fDamage, iWeaponID, iBodyPart);
			}
		}
	}
}

//----------------------------------------------------

static void Click(RPCParameters* rpcParams)
{
	WORD wClickedPlayerID = INVALID_PLAYER_ID;
	unsigned char ucSource = 0;

	if (rpcParams->numberOfBitsOfData == 24 &&
		pNetGame->GetPlayerPool() &&
		pNetGame->GetPlayerPool()->GetSlotState(rpcParams->senderId))
	{
		RakNet::BitStream bsData(rpcParams);

		if (bsData.Read(wClickedPlayerID) &&
			pNetGame->GetPlayerPool()->GetSlotState(wClickedPlayerID))
		{
			bsData.Read(ucSource);

			if (pNetGame->GetFilterScripts())
				pNetGame->GetFilterScripts()->OnPlayerClickPlayer(rpcParams->senderId, wClickedPlayerID, ucSource);

			if (pNetGame->GetGameMode())
				pNetGame->GetGameMode()->OnPlayerClickPlayer(rpcParams->senderId, wClickedPlayerID, ucSource);
		}
	}
}

//----------------------------------------------------

#define GLOBAL_TEXT_DRAW 1
#define PLAYER_TEXT_DRAW 2

static void ScrClickTextDraw(RPCParameters* rpcParams)
{
	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	CTextDrawPool* pTextDrawPool = pNetGame->GetTextDrawPool();
	CFilterScripts* pFilterScripts = pNetGame->GetFilterScripts();
	CGameMode* pGameMode = pNetGame->GetGameMode();

	if (pNetGame->GetGameState() != GAMESTATE_RUNNING) return;
	if (!pPlayerPool || !pTextDrawPool) return;

	WORD wPlayerID = (WORD)rpcParams->senderId;
	
	// Kindly add a patch for prevent receiving fake event, and triggering callbacks anytime
	CPlayer* pPlayer = pPlayerPool->GetAt(wPlayerID);
	if (pPlayer && pPlayer->m_bSelectingText)
	{
		RakNet::BitStream bsData(rpcParams);
		WORD wText = INVALID_TEXT_DRAW;
		WORD wPlayerText = INVALID_TEXT_DRAW;
		BYTE byteType = 0;

		// Check if the stream contains a 2 byte text id
		if (bsData.Read(wText))
		{
			// Here it comes the complex-city.
			// Basically determining the received ID is a global or a player text draw and checking it was visible and
			// selectable for the player and call their respective callbacks. On paper, global and player text draw are
			// different, but calling SelectTextDraw() function makes them act like, they are the same, so if there's
			// a visible clickable global and player text draw, and clicking on a global text will trigger only the 
			// OnPlayerClickTextDraw() callback, while OnPlayerClickPlayerTextDraw() gets no information what so ever.
			// And also when player presses ESC button, only OnPlayerClickTextDraw() gets triggered, when maybe be only
			// player text draws visible for the player, so this mess will isolates them both
			if (wText < MAX_TEXT_DRAWS)
			{
				if (pTextDrawPool->IsSelectable(wText))
				{
					byteType |= GLOBAL_TEXT_DRAW;

					if (pPlayer->m_pTextDraw && pPlayer->m_pTextDraw->HasSelectableText())
					{
						byteType |= PLAYER_TEXT_DRAW;
					}
				}
			}
			else if (wText >= MAX_TEXT_DRAWS && wText < (MAX_TEXT_DRAWS + MAX_PLAYER_TEXT_DRAWS))
			{
				wPlayerText = wText - MAX_TEXT_DRAWS;

				if (pPlayer->m_pTextDraw && pPlayer->m_pTextDraw->IsSelectable(wPlayerText))
				{
					byteType |= PLAYER_TEXT_DRAW;

					if (pTextDrawPool->IsPlayerHasSelectableText(wPlayerID))
					{
						byteType |= GLOBAL_TEXT_DRAW;
						wText = INVALID_TEXT_DRAW;
					}
				}
			}
			else if (wText == INVALID_TEXT_DRAW)
			{
				if (pPlayer->m_pTextDraw && pPlayer->m_pTextDraw->HasSelectableText())
					byteType |= PLAYER_TEXT_DRAW;

				if(pTextDrawPool->IsPlayerHasSelectableText(wPlayerID))
					byteType |= GLOBAL_TEXT_DRAW;
			}

			if (byteType & PLAYER_TEXT_DRAW)
			{
				if (!pFilterScripts || !pFilterScripts->OnPlayerClickPlayerTextDraw(wPlayerID, wPlayerText))
				{
					if (pGameMode)
					{
						pGameMode->OnPlayerClickPlayerTextDraw(wPlayerID, wPlayerText);
					}
				}
			}

			if (byteType & GLOBAL_TEXT_DRAW)
			{
				if (!pFilterScripts || !pFilterScripts->OnPlayerClickTextDraw(wPlayerID, wText))
				{
					if (pGameMode)
					{
						pGameMode->OnPlayerClickTextDraw(wPlayerID, wText);
					}
				}
			}

			// Done here, player no longer on text selection.
			// This also fixes a bug when calling CancelSelectTextDraw() function in one of click text draw
			// callbacks, which would caused being stuck in an infinite loop
			pPlayer->m_bSelectingText = false;
		}
	}
}

//----------------------------------------------------

static void ScrDialogResponse(RPCParameters* rpcParams)
{
	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();

	if (pNetGame->GetGameState() != GAMESTATE_RUNNING) return;
	if (!pPlayerPool) return;

	WORD wPlayerID = (WORD)rpcParams->senderId;

	CPlayer* pPlayer = pPlayerPool->GetAt(wPlayerID);
	if (!pPlayer) return;

	RakNet::BitStream bsData(rpcParams);

	WORD wDialogID = INVALID_DIALOG_ID;
	BYTE byteResponse = 0;
	WORD wListItem = 0xFFFF;
	BYTE byteTextLen = 0;
	char szInputText[MAX_DIALOG_RESPONSE_TEXT + 1];

	if (!bsData.Read(wDialogID)) return;
	if (!bsData.Read(byteResponse)) return;
	if (!bsData.Read(wListItem)) return;
	if (!bsData.Read(byteTextLen)) return;

	if (byteTextLen > MAX_DIALOG_RESPONSE_TEXT) return;

	if (byteTextLen && !bsData.Read(szInputText, byteTextLen)) return;
	szInputText[byteTextLen] = '\0';

	// a response only counts for the dialog this player was actually shown, so a
	// crafted packet cannot fire the callback whenever it likes
	if (pPlayer->m_wDialogID == INVALID_DIALOG_ID || pPlayer->m_wDialogID != wDialogID)
		return;

	pPlayer->m_wDialogID = INVALID_DIALOG_ID;

	CFilterScripts* pFilterScripts = pNetGame->GetFilterScripts();
	CGameMode* pGameMode = pNetGame->GetGameMode();

	if (!pFilterScripts ||
		!pFilterScripts->OnDialogResponse(wPlayerID, wDialogID, byteResponse, (cell)(short)wListItem, szInputText))
	{
		if (pGameMode)
		{
			pGameMode->OnDialogResponse(wPlayerID, wDialogID, byteResponse, (cell)(short)wListItem, szInputText);
		}
	}
}

//----------------------------------------------------

void RegisterRPCs(RakServerInterface * pRakServer)
{
	pRak = pRakServer;

	REGISTER_STATIC_RPC(pRakServer, ClientJoin);
	REGISTER_STATIC_RPC(pRakServer, Chat);
	REGISTER_STATIC_RPC(pRakServer, RequestClass);
	REGISTER_STATIC_RPC(pRakServer, RequestSpawn);
	REGISTER_STATIC_RPC(pRakServer, Spawn);
	REGISTER_STATIC_RPC(pRakServer, Death);
	REGISTER_STATIC_RPC(pRakServer, EnterVehicle);
	REGISTER_STATIC_RPC(pRakServer, ExitVehicle);
	REGISTER_STATIC_RPC(pRakServer, ServerCommand);
	REGISTER_STATIC_RPC(pRakServer, UpdateScoresPingsIPs);
	//REGISTER_STATIC_RPC(pRakServer, SvrStats);
	REGISTER_STATIC_RPC(pRakServer, SetInteriorId);
	REGISTER_STATIC_RPC(pRakServer, ScmEvent);
	REGISTER_STATIC_RPC(pRakServer, AdminMapTeleport);
	REGISTER_STATIC_RPC(pRakServer, VehicleDestroyed);
	REGISTER_STATIC_RPC(pRakServer, PickedUpWeapon);
	REGISTER_STATIC_RPC(pRakServer, PickedUpPickup);
	REGISTER_STATIC_RPC(pRakServer, MenuSelect);
	REGISTER_STATIC_RPC(pRakServer, MenuQuit);
	REGISTER_STATIC_RPC(pRakServer, TypingEvent);
	REGISTER_STATIC_RPC(pRakServer, ClientCheck);
	REGISTER_STATIC_RPC(pRakServer, VehicleDamage);
	REGISTER_STATIC_RPC(pRakServer, ActorDamage);
	REGISTER_STATIC_RPC(pRakServer, Click);
	REGISTER_STATIC_RPC(pRakServer, ScrClickTextDraw);
	REGISTER_STATIC_RPC(pRakServer, ScrDialogResponse);
}

//----------------------------------------------------

/*void UnRegisterRPCs(RakServerInterface * pRakServer)
{
	pRak = 0;

	UNREGISTER_STATIC_RPC(pRakServer, ClientJoin);
	UNREGISTER_STATIC_RPC(pRakServer, Chat);
	UNREGISTER_STATIC_RPC(pRakServer, RequestClass);
	UNREGISTER_STATIC_RPC(pRakServer, RequestSpawn);
	UNREGISTER_STATIC_RPC(pRakServer, Spawn);
	UNREGISTER_STATIC_RPC(pRakServer, Death);
	UNREGISTER_STATIC_RPC(pRakServer, EnterVehicle);
	UNREGISTER_STATIC_RPC(pRakServer, ExitVehicle);
	UNREGISTER_STATIC_RPC(pRakServer, ServerCommand);
	UNREGISTER_STATIC_RPC(pRakServer, UpdateScoresPingsIPs);
	UNREGISTER_STATIC_RPC(pRakServer, SvrStats);
	UNREGISTER_STATIC_RPC(pRakServer, SetInteriorId);
	UNREGISTER_STATIC_RPC(pRakServer, ScmEvent);
	UNREGISTER_STATIC_RPC(pRakServer, AdminMapTeleport);
	UNREGISTER_STATIC_RPC(pRakServer, VehicleDestroyed);
	UNREGISTER_STATIC_RPC(pRakServer, PickedUpWeapon);
	UNREGISTER_STATIC_RPC(pRakServer, PickedUpPickup);
	UNREGISTER_STATIC_RPC(pRakServer, MenuSelect);
	UNREGISTER_STATIC_RPC(pRakServer, MenuQuit);
	UNREGISTER_STATIC_RPC(pRakServer, TypingEvent);

}*/

//----------------------------------------------------
