//----------------------------------------------------------
//
// SA:MP Multiplayer Modification For GTA:SA
// Copyright 2004-2005 SA:MP team
//
// Version: $Id: netrpc.cpp,v 1.40 2006/06/02 13:24:20 mike Exp $
//
//----------------------------------------------------------

#include "../main.h"

using namespace RakNet;

// No longer needed in new RakNet
//#define REGISTER_STATIC_RPC REGISTER_AS_REMOTE_PROCEDURE_CALL 
//#define UNREGISTER_STATIC_RPC UNREGISTER_AS_REMOTE_PROCEDURE_CALL

void ProcessIncommingEvent(BYTE bytePlayerID, int iEventType, DWORD dwParam1, DWORD dwParam2, DWORD dwParam3);
void ApplyManualVehicleLightsPatch();

//----------------------------------------------------
// Sent when a client joins the server we're
// currently connected to.

void ServerJoin(RPCParameters *rpcParams)
{
	CHAR szPlayerName[MAX_PLAYER_NAME+1];
	BYTE bytePlayerID;
	BYTE byteNameLen=0;
	int iPlayerScore;
	bool bIsNPC = false;

	RakNet::BitStream bsData(rpcParams);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	bsData.Read(bytePlayerID);
	bsData.Read(byteNameLen);
	if (byteNameLen > MAX_PLAYER_NAME) return;
	bsData.Read(szPlayerName,byteNameLen);
	szPlayerName[byteNameLen] = '\0';
	bsData.Read(iPlayerScore);
	bsData.Read(bIsNPC);

	// Add this client to the player pool.
	pPlayerPool->New(bytePlayerID, szPlayerName, bIsNPC);
	pPlayerPool->UpdateScore(bytePlayerID, iPlayerScore);

	/*
	switch (bytePlayerState) {
		case PLAYER_STATE_SPECTATING:
			pPlayerPool->GetAt(bytePlayerID)->SetState(PLAYER_STATE_SPECTATING);
	}*/
}

//----------------------------------------------------
// Sent when a client joins the server we're
// currently connected to.

void ServerQuit(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	BYTE bytePlayerID;
	BYTE byteReason;

	bsData.Read(bytePlayerID);
	bsData.Read(byteReason);

	// Delete this client from the player pool.
	pPlayerPool->Delete(bytePlayerID,byteReason);
}

//----------------------------------------------------
// Server is giving us basic init information.

void InitGame(RPCParameters *rpcParams)
{
	RakNet::BitStream bsInitGame(rpcParams);
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();
	BYTE byteMyPlayerID;
	bool bLanMode, bStuntBonus;
	short sOnFootRate = 40;
	short sInCarRate = 40;

	bsInitGame.Read(pNetGame->m_iSpawnsAvailable);
	bsInitGame.Read(byteMyPlayerID);
	bsInitGame.Read(pNetGame->m_bShowPlayerTags);
	bsInitGame.Read(pNetGame->m_bShowPlayerMarkers);
	bsInitGame.Read(pNetGame->m_bTirePopping);
	bsInitGame.Read(pNetGame->m_byteWorldTime);
	bsInitGame.Read(pNetGame->m_byteWeather);
	bsInitGame.Read(pNetGame->m_fGravity);
	bsInitGame.Read(bLanMode);
	bsInitGame.Read((int)pNetGame->m_iDeathDropMoney);
	bsInitGame.Read(pNetGame->m_bInstagib);
	bsInitGame.Read(pNetGame->m_bZoneNames);
	bsInitGame.Read(pNetGame->m_bUseCJWalk);	
	bsInitGame.Read(pNetGame->m_bAllowWeapons);
	bsInitGame.Read(pNetGame->m_bLimitGlobalMarkerRadius);
	bsInitGame.Read(pNetGame->m_fGlobalMarkerRadius);
	bsInitGame.Read(bStuntBonus);
	bsInitGame.Read(pNetGame->m_fNameTagDrawDistance);
	bsInitGame.Read(pNetGame->m_bManualEngineAndLights);
	bsInitGame.Read(pNetGame->m_bDisableEnterExits);
	bsInitGame.Read(pNetGame->m_bDisableVehMapIcons);
	bsInitGame.Read(pNetGame->m_bNameTagLOS);
	bsInitGame.Read(sOnFootRate);
	bsInitGame.Read(sInCarRate);

	size_t uiHostLen;
	bsInitGame.Read(uiHostLen);
	if(0 < uiHostLen) {
		bsInitGame.Read(pNetGame->m_szHostName, uiHostLen);
	}
	pNetGame->m_szHostName[uiHostLen] = '\0';

	pPlayerPool->SetLocalPlayerID(byteMyPlayerID);
	pGame->EnableStuntBonus(bStuntBonus);
	if (bLanMode) pNetGame->SetLanMode(true);

	//pNetGame->InitGameLogic();
	
	if (pNetGame->m_bManualEngineAndLights)
		ApplyManualVehicleLightsPatch();

	// Set the gravity now
	pGame->SetGravity(pNetGame->m_fGravity);

	// Disable the enter/exits if needed.
	pGame->DisableEnterExits(pNetGame->m_bDisableEnterExits);
	
	CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();
	pLocalPlayer->m_sNormalOnfootRate = sOnFootRate;
	pLocalPlayer->m_sNormalIncarRate = sInCarRate;
	pLocalPlayer->GetPlayerPed()->TogglePlayerControllable(1);

	pNetGame->SetGameState(GAMESTATE_CONNECTED);
	pLocalPlayer->HandleClassSelection();

	pChatWindow->AddDebugMessage("Connected to {B9C9BF}%.64s",pNetGame->m_szHostName);

	UpdateDiscordPresence("Playing", pNetGame->m_szHostName);
}

//----------------------------------------------------
// Remote player has sent a chat message.

void Chat(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE bytePlayerID;
	size_t uiTextLen;

	if(pNetGame->GetGameState() != GAMESTATE_CONNECTED)	return;

	unsigned char szText[256];
	memset(szText,0,256);

	bsData.Read(bytePlayerID);
	bsData.Read(uiTextLen);

	if(uiTextLen > MAX_CMD_INPUT) return;

	bsData.Read((char*)szText, uiTextLen);

	szText[uiTextLen] = '\0';

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	if (bytePlayerID == pPlayerPool->GetLocalPlayerID()) {
		CLocalPlayer* pPlayer = pPlayerPool->GetLocalPlayer();
		pChatWindow->AddChatMessage((CHAR*)pPlayer->GetName(),
			pPlayer->GetPlayerColorAsARGB(), (char*)szText);
	} else {
		CRemotePlayer *pRemotePlayer = pPlayerPool->GetAt(bytePlayerID);
		if(pRemotePlayer != NULL) {
			pRemotePlayer->Say(szText);	
		}
	}
}

//----------------------------------------------------
// Reply to our class request from the server.

void RequestClass(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE byteRequestOutcome=0;
	PLAYER_SPAWN_INFO SpawnInfo;
	CLocalPlayer *pPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();

	bsData.Read(byteRequestOutcome);
	bsData.Read((PCHAR)&SpawnInfo,sizeof(PLAYER_SPAWN_INFO));

	if(byteRequestOutcome) {
		pPlayer->SetSpawnInfo(&SpawnInfo);
		pPlayer->HandleClassSelectionOutcome(true);
	}
	else {
		pPlayer->HandleClassSelectionOutcome(false);
	}
}

//----------------------------------------------------
// The server has allowed us to spawn!

void RequestSpawn(RPCParameters *rpcParams)
{
	BYTE byteRequestOutcome=0;
	RakNet::BitStream bsData(rpcParams);
	bsData.Read(byteRequestOutcome);
	CLocalPlayer *pPlayer = pNetGame->GetPlayerPool()->GetLocalPlayer();
	if (byteRequestOutcome == 2 || (byteRequestOutcome && pPlayer->m_bWaitingForSpawnRequestReply)) {
		pPlayer->Spawn();
	}
	else {
		pPlayer->m_bWaitingForSpawnRequestReply = false;
	}
}

//----------------------------------------------------
// Remote client is spawning.

void Spawn(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	CRemotePlayer *pRemotePlayer;

	BYTE bytePlayerID=0;
	BYTE byteTeam=0;
	int iSkin=0;
	VECTOR vecPos;
	float fRotation=0;
	DWORD dwColor=0;

	bsData.Read(bytePlayerID);
	bsData.Read(byteTeam);
	bsData.Read(iSkin);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fRotation);
	bsData.Read(dwColor);

	pRemotePlayer = pNetGame->GetPlayerPool()->GetAt(bytePlayerID);

	if(pRemotePlayer) 
		pRemotePlayer->Spawn(byteTeam,iSkin,&vecPos,fRotation,dwColor);
		
}

//----------------------------------------------------
// Remote client is dead.

void Death(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);

	BYTE bytePlayerID=0;

	bsData.Read(bytePlayerID);

	CRemotePlayer *pRemotePlayer = pNetGame->GetPlayerPool()->GetAt(bytePlayerID);
	if(pRemotePlayer) {
		CPlayerPed* pRemotePed = pRemotePlayer->GetPlayerPed();
		#ifdef WANT_PICKUP
		if (pRemotePed->GetDistanceFromLocalPlayerPed() <= 100.0f)
		{
			MATRIX4X4 matPlayer;
			pRemotePed->GetMatrix(&matPlayer);

			WEAPON_SLOT_TYPE * WeaponSlot = pRemotePlayer->GetCurrentWeaponSlot()
			if (WeaponSlot->dwType != 0 &&
				WeaponSlot->dwType != WEAPON_GRENADE &&
				WeaponSlot->dwType != WEAPON_TEARGAS &&
				WeaponSlot->dwType != WEAPON_MOLTOV)
			{
				//DWORD hnd = 0;
				//ScriptCommand(&create_pickup_with_ammo, pGame->GetWeaponModelIDFromWeapon(pRemotePed->m_pPed->WeaponSlots[i].dwType), 4, pRemotePed->m_pPed->WeaponSlots[i].dwAmmoInClip, matPlayer.pos.X + rand()%4, matPlayer.pos.Y + rand()%4, matPlayer.pos.Z, &hnd);	
				pNetGame->GetPickupPool()->New(pGame->GetWeaponModelIDFromWeapon(WeaponSlot->dwType), matPlayer.pos.X, matPlayer.pos.Y, matPlayer.pos.Z, WeaponSlot->dwAmmoInClip, bytePlayerID);
			}
		}
		#endif
		pRemotePlayer->HandleDeath();
	}
}

//----------------------------------------------------
// Remote client is trying to enter vehicle gracefully.

void EnterVehicle(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE bytePlayerID=0;
	VEHICLEID VehicleID=0;
	BYTE bytePassenger=0;
	bool bPassenger=false;

	bsData.Read(bytePlayerID);
	bsData.Read(VehicleID);
	bsData.Read(bytePassenger);

	if(bytePassenger) bPassenger = true;

	CRemotePlayer *pRemotePlayer = pNetGame->GetPlayerPool()->GetAt(bytePlayerID);
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(pRemotePlayer &&
		pRemotePlayer->GetDistanceFromLocalPlayer() < 200.0f) {
		pRemotePlayer->EnterVehicle(VehicleID,bPassenger);
	}

	//pChatWindow->AddDebugMessage("Player(%u)::EnterVehicle(%u)",bytePlayerID,byteVehicleID);
}

//----------------------------------------------------
// Remote client is trying to enter vehicle gracefully.

void ExitVehicle(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE bytePlayerID=0;
	VEHICLEID VehicleID=0;

	bsData.Read(bytePlayerID);
	bsData.Read(VehicleID);

	CRemotePlayer *pRemotePlayer = pNetGame->GetPlayerPool()->GetAt(bytePlayerID);
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();

	if(pRemotePlayer &&
		pRemotePlayer->GetDistanceFromLocalPlayer() < 200.0f) {
		pRemotePlayer->ExitVehicle();
	}

	//pChatWindow->AddDebugMessage("Player(%u)::ExitVehicle(%u)",bytePlayerID,byteVehicleID);
}

//----------------------------------------------------

void VehicleSpawn(RPCParameters *rpcParams)
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	RakNet::BitStream bsData(rpcParams);
	VEHICLEID VehicleID=0;
	int iVehicleType;
	VECTOR vecPos;
	VECTOR vecSpawnPos;
	float fRotation;
	float fSpawnRotation;
	float fHealth;
	int iColor1, iColor2;
	int iInterior;
	bool bHasNumberPlate = false;
	bool bHasCarModInfo = false;

	CAR_MOD_INFO m_CarModInfo;
	CHAR cNumberPlate[9];

	memset(&m_CarModInfo,0,sizeof(CAR_MOD_INFO));
	memset(&cNumberPlate[0],0,9);

	bsData.Read(VehicleID);
	bsData.Read(iVehicleType);
	bsData.Read(vecPos.X);
	bsData.Read(vecPos.Y);
	bsData.Read(vecPos.Z);
	bsData.Read(fRotation);
	bsData.Read(iColor1);
	bsData.Read(iColor2);
	bsData.Read(fHealth);
	bsData.Read(vecSpawnPos.X);
	bsData.Read(vecSpawnPos.Y);
	bsData.Read(vecSpawnPos.Z);
	bsData.Read(fSpawnRotation);
	bsData.Read(iInterior);

	bsData.Read(pVehiclePool->m_bHasSiren[VehicleID]);

	bsData.ReadBits((unsigned char*)&pVehiclePool->m_Windows[VehicleID], 4);
	bsData.ReadBits((unsigned char*)&pVehiclePool->m_Doors[VehicleID], 4);

	bsData.Read(bHasNumberPlate);
	if(bHasNumberPlate) {
		bsData.Read(cNumberPlate, 9); // Constant size defined by SA
	}

	bsData.Read(bHasCarModInfo);
	if(bHasCarModInfo) {
		bsData.Read((PCHAR)&m_CarModInfo, sizeof(m_CarModInfo));
	} else {
		m_CarModInfo.iColor0 = -1;
		m_CarModInfo.iColor1 = -1;
	}

	if (!pVehiclePool->New(VehicleID,iVehicleType, &vecPos,fRotation,iColor1,iColor2,&vecSpawnPos,fSpawnRotation, iInterior, cNumberPlate))
		return;
		
	int iVehicle = pVehiclePool->FindGtaIDFromID(VehicleID);

	if (iVehicle)
	{
		for (int i = 0; i < 17; i++)
		{
			if (m_CarModInfo.ucCarMod[i] != 0)
			{
				DWORD data = m_CarModInfo.ucCarMod[i] + 1000;
				DWORD v = 0;

				CGame::RequestModel(data);
				CGame::LoadRequestedModels();
				ScriptCommand(&request_car_component, data);

				int iWait = 10;
				while (!ScriptCommand(&is_component_available, data) && iWait) {
					Sleep(5);
					iWait--;
				}
				if (!iWait) {
					//pChatWindow->AddDebugMessage("Timeout on car component.");
					continue;
				}
				//pChatWindow->AddDebugMessage("CarComponent: %u,%u,%u",VehicleID,data,iVehicleType);
				wLastVehicleComponent = (WORD)data;
				ScriptCommand(&add_car_component, iVehicle, data, &v);
			}
		}

		if (m_CarModInfo.bytePaintJob)
			ScriptCommand(&change_car_skin, iVehicle, m_CarModInfo.bytePaintJob);

		if (m_CarModInfo.iColor0 != -1 || m_CarModInfo.iColor1 != -1)
			pVehiclePool->GetAt(VehicleID)->SetColor(m_CarModInfo.iColor0, m_CarModInfo.iColor1);
	}
}

//----------------------------------------------------

static void VehicleDamage(RPCParameters* rpcParams)
{
	CVehiclePool* pVehiclePool;
	CVehicle* pVehicle;
	VEHICLEID VehicleID;
	int iPanelsStatus, iDoorsStatus;
	unsigned char ucLightsStatus, ucWheelStatus;

	pVehiclePool = pNetGame->GetVehiclePool();

	if (rpcParams->numberOfBitsOfData == 96 && pVehiclePool)
	{
		RakNet::BitStream bsData(rpcParams);

		bsData.Read(VehicleID);
		
		pVehicle = pVehiclePool->GetAt(VehicleID);
		if (pVehicle != NULL)
		{
			bsData.Read(iPanelsStatus);
			bsData.Read(iDoorsStatus);
			bsData.Read(ucLightsStatus);
			bsData.Read(ucWheelStatus);

			pVehicle->UpdateDamage(iPanelsStatus, iDoorsStatus, ucLightsStatus);
			pVehicle->SetCarOrBikeWheelStatus(ucWheelStatus);
		}
	}
}

//----------------------------------------------------

void SetCheckpoint(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	float fX, fY, fZ, fSize;

	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	bsData.Read(fSize);

	VECTOR Pos,Extent;

	Pos.X = fX;
	Pos.Y = fY;
	Pos.Z = fZ;
	Extent.X = fSize;
	Extent.Y = fSize;
	Extent.Z = fSize;

	pGame->SetCheckpointInformation(&Pos, &Extent);
	pGame->ToggleCheckpoints(true);
}

//----------------------------------------------------

void DisableCheckpoint(RPCParameters *rpcParams)
{
	pGame->ToggleCheckpoints(false);
}

//----------------------------------------------------

void SetRaceCheckpoint(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	float fX, fY, fZ;
	BYTE byteType; //, byteSize;
	VECTOR Pos,Next;

	bsData.Read(byteType);
	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	Pos.X = fX;
	Pos.Y = fY;
	Pos.Z = fZ;

	bsData.Read(fX);
	bsData.Read(fY);
	bsData.Read(fZ);
	Next.X = fX;
	Next.Y = fY;
	Next.Z = fZ;

	bsData.Read(fX);

	pGame->SetRaceCheckpointInformation(byteType, &Pos, &Next, fX);
	pGame->ToggleRaceCheckpoints(true);
}

//----------------------------------------------------

void DisableRaceCheckpoint(RPCParameters *rpcParams)
{
	pGame->ToggleRaceCheckpoints(false);
}

//----------------------------------------------------

static void UpdateScoresPingsIPs(RPCParameters* rpcParams)
{
	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	int iBitLength = rpcParams->numberOfBitsOfData;

	// We'll ignore the last 16 bits (possibly player count) in the stream,
	// but also count on it at overall size, or the statement will be false for 0.3.7 servers
	if (!pPlayerPool || iBitLength < 80 || iBitLength > ((80 * MAX_PLAYERS) + 16))
		return;
	
	RakNet::BitStream bsData(rpcParams);

	WORD wPlayerID;
	int  iPlayerScore;
	DWORD dwPlayerPing;

	// Loop until the unreaded bits gets less or equal than 16 bits (2 bytes)
	// counting on the 2 byte player count(?) on the last bits
	while (16 <= bsData.GetNumberOfUnreadBits())
	{
		// Resetting values, in case the stream readings fails,
		// and not update the last processed player again
		wPlayerID = INVALID_PLAYER_ID;
		iPlayerScore = 0;
		dwPlayerPing = 0;

		bsData.Read(wPlayerID);
		bsData.Read(iPlayerScore);
		bsData.Read(dwPlayerPing);

		if (wPlayerID == pPlayerPool->GetLocalPlayerID())
		{
			pPlayerPool->GetLocalPlayer()->m_iScore = iPlayerScore;
			pPlayerPool->GetLocalPlayer()->m_usPing = (WORD)dwPlayerPing;
		}
		else
		{
			CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(wPlayerID);
			if (pRemotePlayer) {
				pRemotePlayer->m_iScore = iPlayerScore;
				pRemotePlayer->m_usPing = (WORD)dwPlayerPing;
			}
		}
	}

	if (pScoreBoard)
		pScoreBoard->UpdateList();
}

//----------------------------------------------------
/*extern RakNetStatisticsStruct RakServerStats;

void SvrStats(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	bsData.Read((char *)&RakServerStats,sizeof(RakNetStatisticsStruct));
}*/

//----------------------------------------------------

void GameModeRestart(RPCParameters *rpcParams)
{
	pNetGame->ShutdownForGameModeRestart();
}

//----------------------------------------------------

void ConnectionRejected(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE byteRejectReason = REJECT_REASON_BAD_VERSION;

	bsData.Read(byteRejectReason);

	switch (byteRejectReason)
	{
	case REJECT_REASON_BAD_VERSION:
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: Incorrect Version.");
		break;
	case REJECT_REASON_BAD_NICKNAME:
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: Unacceptable NickName");
		pChatWindow->AddInfoMessage("Please choose another nick between and 3-20 characters");
		pChatWindow->AddInfoMessage("Please use only a-z, A-Z, 0-9");
		pChatWindow->AddInfoMessage("Use /quit to exit or press ESC and select Quit Game");
		break;
	case REJECT_REASON_BAD_MOD:
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: Bad mod version.");
		break;
	case REJECT_REASON_BAD_PLAYERID:
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: Unable to allocate a player slot.");
		break;
	case REJECT_REASON_IP_LIMIT_REACHED:
		pChatWindow->AddInfoMessage("CONNECTION REJECTED: IP limit reached.");
		break;
	}

	pNetGame->GetRakClient()->Disconnect(500);
}

//----------------------------------------------------

void ClientMessage(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	DWORD dwStrLen;
	DWORD dwColor;

	bsData.Read(dwColor);
	bsData.Read(dwStrLen);
	char* szMsg = (char*)malloc(dwStrLen+1);
	bsData.Read(szMsg, dwStrLen);
	szMsg[dwStrLen] = 0;

	pChatWindow->AddClientMessage(dwColor,szMsg);

	free(szMsg);
}

//----------------------------------------------------

void WorldTime(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE byteWorldTime;
	bsData.Read(byteWorldTime);
	pNetGame->m_byteWorldTime = byteWorldTime;	
}

//----------------------------------------------------

void Pickup(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);

	PICKUP Pickup;
	int iIndex;

	bsData.Read(iIndex);
	bsData.Read((PCHAR)&Pickup, sizeof (PICKUP));

	//pChatWindow->AddDebugMessage("Pickup: %d %d %f %f %f",iModel,iType,x,y,z);

	pNetGame->GetPickupPool()->New(&Pickup, iIndex);
}

//----------------------------------------------------

void DestroyPickup(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);

	int iIndex;

	bsData.Read(iIndex);

	pNetGame->GetPickupPool()->Destroy(iIndex);
}

//----------------------------------------------------

void DestroyWeaponPickup(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);

	BYTE byteIndex;

	bsData.Read(byteIndex);

	pNetGame->GetPickupPool()->DestroyDropped(byteIndex);
}

//----------------------------------------------------

void ScmEvent(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	
	BYTE bytePlayerID;
	int iEvent;
	DWORD dwParam1,dwParam2,dwParam3;

	bsData.Read(bytePlayerID);
	bsData.Read(iEvent);
	bsData.Read(dwParam1);
	bsData.Read(dwParam2);
	bsData.Read(dwParam3);

	ProcessIncommingEvent(bytePlayerID,iEvent,dwParam1,dwParam2,dwParam3);
}

//----------------------------------------------------

void Weather(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE byteWeather;
	bsData.Read(byteWeather);
	pNetGame->m_byteWeather = byteWeather;	
}

//----------------------------------------------------

void SetTimeEx(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE byteHour;
	BYTE byteMinute;
	bsData.Read(byteHour);
	bsData.Read(byteMinute);
	//pNetGame->m_byteHoldTime = 0;
	pGame->SetWorldTime(byteHour, byteMinute);
	pNetGame->m_byteWorldTime = byteHour;
	pNetGame->m_byteWorldMinute = byteMinute;
}

//----------------------------------------------------

void ToggleClock(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE byteClock;
	bsData.Read(byteClock);
	pGame->EnableClock(byteClock);	
	if (byteClock)
	{
		pNetGame->m_byteHoldTime = 0;
	}
	else
	{
		pNetGame->m_byteHoldTime = 1;
		pGame->GetWorldTime((int*)&pNetGame->m_byteWorldTime, (int*)&pNetGame->m_byteWorldMinute);
	}
}

//----------------------------------------------------

void VehicleDestroy(RPCParameters *rpcParams)
{
	CVehiclePool *pVehiclePool = pNetGame->GetVehiclePool();
	RakNet::BitStream bsData(rpcParams);

	VEHICLEID VehicleID;
	VEHICLEID MyVehicleID;

	bsData.Read(VehicleID);

	CVehicle *pVehicle = pVehiclePool->GetAt(VehicleID);
	CRemotePlayer *pRemotePlayer;
	CPlayerPool *pPlayerPool = pNetGame->GetPlayerPool();

	if(!pVehicle) return;
			
	MyVehicleID = pVehiclePool->FindIDFromGtaPtr(pGame->FindPlayerPed()->GetGtaVehicle());

	if (MyVehicleID == VehicleID)
	{
		MATRIX4X4 mat;
		pGame->FindPlayerPed()->GetMatrix(&mat);
		pGame->FindPlayerPed()->RemoveFromVehicleAndPutAt(mat.pos.X,mat.pos.Y,mat.pos.Z);
	}

	// we'll also need to remove any remote player that
	// is in this vehicle.
	BYTE x=0;
	while(x!=MAX_PLAYERS) {
		if(pPlayerPool->GetSlotState(x)) {
			pRemotePlayer = pPlayerPool->GetAt(x);
			if( pRemotePlayer->IsActive() && 
				pRemotePlayer->m_pCurrentVehicle == pVehicle )
			{
				pRemotePlayer->ForceOutOfCurrentVehicle();
			}
		}
		x++;
	}
	pNetGame->GetVehiclePool()->Delete(VehicleID);
}

//----------------------------------------------------

void Instagib(RPCParameters *rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	bsData.Read(pNetGame->m_bInstagib);
}

//----------------------------------------------------

static BYTE GetByteSumAtAddress(DWORD dwAddr, WORD wCount)
{
	BYTE sum = 0;
	WORD byte = 0;
	do {
		sum ^= *(BYTE*)(dwAddr + byte++) & 0xCC;
	} while (byte != wCount);
	return sum;
}

void SendClientCheckResponse(BYTE byteType, DWORD dwAddress, BYTE byteSum)
{
	RakNet::BitStream bsSend;

	bsSend.Write(byteType);
	bsSend.Write(dwAddress);
	bsSend.Write(byteSum);

	pNetGame->Send(RPC_ClientCheck, &bsSend);
}

static void ClientCheck(RPCParameters* rpcParams)
{
	RakNet::BitStream bsData(rpcParams);
	BYTE byteType=0;
	DWORD dwAddress=0, dwTempAddr;
	WORD wOffset=0, wCount=0;
	BYTE byteSum=0;

	if (!bsData.Read(byteType) ||
		!bsData.Read(dwAddress) ||
		!bsData.Read(wOffset) || wOffset > 256 ||
		!bsData.Read(wCount) || wCount > 256 || wCount < 2)
	{
		return;
	}

	switch (byteType)
	{
	case 70:
	{
		if (IsValidModel(dwAddress))
		{
			dwTempAddr = (DWORD)GetModelInfo(dwAddress);
			if (dwTempAddr)
				byteSum = GetByteSumAtAddress(dwTempAddr + wOffset, wCount);
		}
		SendClientCheckResponse(70, dwAddress, byteSum);
		break;
	}
	case 2:
	{
		CPlayerPed* pPlayerPed = pGame->FindPlayerPed();
		if (pPlayerPed)
		{
			if (!pPlayerPed->IsInVehicle() || pPlayerPed->IsAPassenger())
			{
				if (pPlayerPed->m_pPed)
				{
					dwAddress = *(DWORD*)((DWORD)pPlayerPed->m_pPed + 64);
					byteSum = 1;
				}
			}
			else
			{
				if (pPlayerPed->m_pPed->pVehicle)
				{
					dwAddress = *(DWORD*)((DWORD)pPlayerPed->m_pPed->pVehicle + 64);
					byteSum = 2;
				}
			}
		}
		SendClientCheckResponse(2, dwAddress, byteSum);
		break;
	}
	case 71:
	{
		if (IsValidModel(dwAddress))
		{
			INT iWaitCount = 0;
			if (!CGame::IsModelLoaded(dwAddress))
			{
				CGame::RequestModel(dwAddress);
				CGame::LoadRequestedModels();
				while (!CGame::IsModelLoaded(dwAddress))
				{
					if (++iWaitCount > 1000)
						break;
					Sleep(1);
				}
			}
			if (CGame::IsModelLoaded(dwAddress))
			{
				dwTempAddr = (DWORD)GetModelColInfo(dwAddress);
				if (dwTempAddr)
					byteSum = GetByteSumAtAddress(dwTempAddr + wOffset, wCount);

				if (!GetModelUseCount(dwAddress))
					CGame::RemoveModel(dwAddress);
			}
		}
		SendClientCheckResponse(71, dwAddress, byteSum);
		break;
	}
	case 5:
	{
		if (dwAddress >= 0x400000 && dwAddress <= 0x856E00)
		{
			if (dwAddress) // ???
				byteSum = GetByteSumAtAddress(dwAddress + wOffset, wCount);

			SendClientCheckResponse(5, dwAddress, byteSum);
		}
		break;
	}
	case 69:
	{
		// TODO: Change the size of .text section for each version?
		if (dwAddress <= 0xC3500)
		{
			dwTempAddr = (DWORD)hInstance + dwAddress;
			if (dwTempAddr)
				byteSum = GetByteSumAtAddress(dwTempAddr + wOffset, wCount);

			SendClientCheckResponse(69, dwAddress, byteSum);
		}
		break;
	}
	case 72:
	{
		dwAddress = (RakNet::GetTime32() & 0xFFFFFFF | 0x30000000);
		SendClientCheckResponse(72, dwAddress, 0);
		break;
	}
	}
}

//----------------------------------------------------

static void WorldAddActor(RPCParameters* rpcParams)
{
	ACTOR_TRANSMIT Transmit;

	// Note: Due to SA-MP's structure packing, ACTOR_TRANSMIT size is 27 bytes,
	// and not 32 bytes, so everything, except the actor id will be correct.
	// Atleast this way. So maybe it's a TODO?
	// Also Note: In the DL version the ACTOR_TRANSMIT size is 31 bytes,
	// because it's contains some custom skin informations.
	if (pNetGame->GetActorPool() &&
		rpcParams->numberOfBitsOfData == sizeof(ACTOR_TRANSMIT) * 8)
	{
		RakNet::BitStream bsData(rpcParams);

		memset(&Transmit, 0, sizeof(ACTOR_TRANSMIT));

		bsData.Read((PCHAR)&Transmit, sizeof(ACTOR_TRANSMIT));

		// TODO: Change this line
		//if (sub_100B3DE0(ActorData.iModelID))
		{
			pNetGame->GetActorPool()->New(&Transmit);

			// Seems like theres an leftover code here, it's not connected anywhere
			/*if (ActorData.usID < MAX_ACTORS)
			{
				if (pNetGame->GetActorPool()->m_bSlotState[ActorData.usID])
				{
					CActor* pActor = pNetGame->GetActorPool()->m_pActor[ActorData.usID];
				}
			}*/
		}
	}
}

//----------------------------------------------------

static void WorldRemoveActor(RPCParameters* rpcParams)
{
	unsigned short usActorID = INVALID_ACTOR_ID;

	if (pNetGame->GetActorPool() &&
		rpcParams->numberOfBitsOfData == 16)
	{
		RakNet::BitStream bsData(rpcParams);

		bsData.Read(usActorID);
		
		pNetGame->GetActorPool()->Delete(usActorID);
	}
}

//----------------------------------------------------

static void ChatBubble(RPCParameters* rpcParams)
{
	WORD wPlayerID;
	DWORD dwColor;
	float fDistance;
	int iExpireTime;
	BYTE byteTextLen;
	char szText[MAX_CHAT_BUBBLE_TEXT + 1];

	if (pChatBubble && pNetGame->GetPlayerPool())
	{
		RakNet::BitStream bsData(rpcParams);

		bsData.Read(wPlayerID);

		if (pNetGame->GetPlayerPool()->GetSlotState(wPlayerID))
		{
			bsData.Read(dwColor);
			bsData.Read(fDistance);
			bsData.Read(iExpireTime);
			bsData.Read(byteTextLen);

			if (byteTextLen <= MAX_CHAT_BUBBLE_TEXT &&
				byteTextLen == (BYTE)(bsData.GetNumberOfUnreadBits() / 8))
			{
				bsData.Read(szText, byteTextLen);
				szText[byteTextLen] = 0;

				pChatBubble->AddText(wPlayerID, szText, dwColor, fDistance, iExpireTime);
			}
		}
	}
}

//----------------------------------------------------

static void CreateLabel(RPCParameters* rpcParams)
{
	WORD wLabelID = INVALID_LABEL_ID;
	char szText[MAX_LABEL_TEXT+1];
	DWORD dwColor;
	VECTOR vecPos;
	float fDistance;
	BYTE byteTestLOS;
	WORD wAttachedPlayerID = INVALID_PLAYER_ID;
	WORD wAttachedVehicleID = INVALID_PLAYER_ID;

	if (pNetGame->GetLabelPool() &&
		rpcParams->numberOfBitsOfData >= 216)
	{
		RakNet::BitStream bsData(rpcParams);

		bsData.Read(wLabelID);

		if (!pNetGame->GetLabelPool()->GetSlotState(wLabelID))
		{
			SecureZeroMemory(szText, sizeof(szText));

			bsData.Read(dwColor);
			bsData.Read(vecPos);
			bsData.Read(fDistance);
			bsData.Read(byteTestLOS);
			bsData.Read(wAttachedPlayerID);
			bsData.Read(wAttachedVehicleID);

			if (stringCompressor->DecodeString(szText, sizeof(szText), &bsData))
			{
				pNetGame->GetLabelPool()->New(wLabelID, szText, dwColor, vecPos,
					fDistance, byteTestLOS, wAttachedPlayerID, wAttachedVehicleID);
			}
		}
	}
}

//----------------------------------------------------

static void DestroyLabel(RPCParameters* rpcParams)
{
	if (pNetGame->GetLabelPool() &&
		rpcParams->numberOfBitsOfData == 16)
	{
		RakNet::BitStream bsData(rpcParams);
		WORD wLabelID = INVALID_LABEL_ID;

		bsData.Read(wLabelID);

		if (pNetGame->GetLabelPool()->GetSlotState(wLabelID))
		{
			pNetGame->GetLabelPool()->Delete(wLabelID);
		}
	}
}

//----------------------------------------------------

static void VehicleParams(RPCParameters* rpcParams)
{
	VEHICLEID VehicleID = INVALID_VEHICLE_ID;
	VEHICLE_PARAMS Params;
	CVehicle* pVehicle;

	if (pNetGame->GetVehiclePool() &&
		rpcParams->numberOfBitsOfData == 144)
	{
		RakNet::BitStream bsData(rpcParams);
		
		bsData.Read(VehicleID);
		
		pVehicle = pNetGame->GetVehiclePool()->GetAt(VehicleID);
		if (pVehicle)
		{
			bsData.Read((PCHAR)&Params, sizeof(VEHICLE_PARAMS));

			if (Params.byteEngine == VEHICLE_PARAMS_ON)
				pVehicle->SetEngine(true);
			else if (Params.byteEngine == VEHICLE_PARAMS_OFF)
				pVehicle->SetEngine(false);

			if (Params.byteLights == VEHICLE_PARAMS_ON)
				pVehicle->SetLights(true);
			else if (Params.byteLights == VEHICLE_PARAMS_OFF)
				pVehicle->SetLights(false);

			if (Params.byteDoors == VEHICLE_PARAMS_ON)
				pVehicle->SetDoorState(1);
			else if (Params.byteDoors == VEHICLE_PARAMS_OFF)
				pVehicle->SetDoorState(0);

			if (Params.byteObjective == VEHICLE_PARAMS_ON)
				pVehicle->SetObjective(true);
			else if (Params.byteObjective == VEHICLE_PARAMS_OFF)
				pVehicle->SetObjective(false);

			if (Params.byteBoot == VEHICLE_PARAMS_ON)
				pVehicle->ToggleDoor(1, 17, 1.0f);
			else if (Params.byteBoot == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleDoor(1, 17, 0.0f);

			if (Params.byteBonnet == VEHICLE_PARAMS_ON)
				pVehicle->ToggleDoor(1, 16, 1.0f);
			else if (Params.byteBonnet == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleDoor(1, 16, 0.0f);

			if (Params.byteAlarm == VEHICLE_PARAMS_ON)
			{
				pVehicle->SetSirenOn(TRUE);
				pVehicle->SetAlarmState(20000);
			}
			else if (Params.byteAlarm == VEHICLE_PARAMS_OFF)
			{
				pVehicle->SetSirenOn(FALSE);
				pVehicle->SetAlarmState(0);
			}

			if (Params.byteSiren == VEHICLE_PARAMS_ON)
				pVehicle->SetSirenOn(TRUE);
			else if (Params.byteSiren == VEHICLE_PARAMS_OFF)
				pVehicle->SetSirenOn(FALSE);

			if (Params.byteDriverDoor == VEHICLE_PARAMS_ON)
				pVehicle->ToggleDoor(2, 10, 1.0f);
			else if (Params.byteDriverDoor == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleDoor(2, 10, 0.0f);

			if (Params.bytePassengerDoor == VEHICLE_PARAMS_ON)
				pVehicle->ToggleDoor(3, 8, 1.0f);
			else if (Params.bytePassengerDoor == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleDoor(3, 8, 0.0f);

			if (Params.byteBackLeftDoor == VEHICLE_PARAMS_ON)
				pVehicle->ToggleDoor(4, 11, 1.0f);
			else if (Params.byteBackLeftDoor == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleDoor(4, 11, 0.0f);

			if (Params.byteBackRightDoor == VEHICLE_PARAMS_ON)
				pVehicle->ToggleDoor(5, 9, 1.0f);
			else if (Params.byteBackRightDoor == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleDoor(5, 9, 0.0f);

			if (Params.byteDriverWindow == VEHICLE_PARAMS_ON)
				pVehicle->ToggleWindow(10, true);
			else if (Params.byteDriverWindow == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleWindow(10, false);

			if (Params.bytePassengerWindow == VEHICLE_PARAMS_ON)
				pVehicle->ToggleWindow(8, true);
			else if (Params.bytePassengerWindow == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleWindow(8, false);

			if (Params.byteBackLeftWindow == VEHICLE_PARAMS_ON)
				pVehicle->ToggleWindow(11, true);
			else if (Params.byteBackLeftWindow == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleWindow(11, false);

			if (Params.byteBackRightWindow == VEHICLE_PARAMS_ON)
				pVehicle->ToggleWindow(9, true);
			else if (Params.byteBackRightWindow == VEHICLE_PARAMS_OFF)
				pVehicle->ToggleWindow(9, false);
		}
	}
}

//----------------------------------------------------

void SetTimer(RPCParameters* rpcParams)
{
	if (rpcParams->numberOfBitsOfData != 32)
		return;

	RakNet::BitStream bsData(rpcParams);
	DWORD dwTime;

	if (bsData.Read(dwTime))
	{
		pGame->SetTimer(dwTime);
	}
}

//----------------------------------------------------

void RegisterRPCs(RakClientInterface * pRakClient)
{
	REGISTER_STATIC_RPC(pRakClient,ServerJoin);
	REGISTER_STATIC_RPC(pRakClient,ServerQuit);	
	REGISTER_STATIC_RPC(pRakClient,InitGame);
	REGISTER_STATIC_RPC(pRakClient,Chat);
	REGISTER_STATIC_RPC(pRakClient,RequestClass);
	REGISTER_STATIC_RPC(pRakClient,RequestSpawn);
	REGISTER_STATIC_RPC(pRakClient,Spawn);
	REGISTER_STATIC_RPC(pRakClient,Death);
	REGISTER_STATIC_RPC(pRakClient,EnterVehicle);
	REGISTER_STATIC_RPC(pRakClient,ExitVehicle);
	REGISTER_STATIC_RPC(pRakClient,VehicleSpawn);
	REGISTER_STATIC_RPC(pRakClient,VehicleDestroy);
	REGISTER_STATIC_RPC(pRakClient,VehicleDamage);
	REGISTER_STATIC_RPC(pRakClient,SetCheckpoint); //14
	REGISTER_STATIC_RPC(pRakClient,DisableCheckpoint);
	REGISTER_STATIC_RPC(pRakClient,SetRaceCheckpoint);
	REGISTER_STATIC_RPC(pRakClient,DisableRaceCheckpoint);
	REGISTER_STATIC_RPC(pRakClient,UpdateScoresPingsIPs);
	//REGISTER_STATIC_RPC(pRakClient,SvrStats);
	REGISTER_STATIC_RPC(pRakClient,GameModeRestart);
	REGISTER_STATIC_RPC(pRakClient,ConnectionRejected);
	REGISTER_STATIC_RPC(pRakClient,ClientMessage);
	REGISTER_STATIC_RPC(pRakClient,WorldTime);
	REGISTER_STATIC_RPC(pRakClient,Pickup);
	REGISTER_STATIC_RPC(pRakClient,DestroyPickup);
	REGISTER_STATIC_RPC(pRakClient,DestroyWeaponPickup);
	REGISTER_STATIC_RPC(pRakClient,ScmEvent);
	REGISTER_STATIC_RPC(pRakClient,Weather);
	REGISTER_STATIC_RPC(pRakClient,Instagib);
	REGISTER_STATIC_RPC(pRakClient,SetTimeEx);
	REGISTER_STATIC_RPC(pRakClient,ToggleClock);
	REGISTER_STATIC_RPC(pRakClient,ClientCheck);
	REGISTER_STATIC_RPC(pRakClient,WorldAddActor);
	REGISTER_STATIC_RPC(pRakClient,WorldRemoveActor);
	REGISTER_STATIC_RPC(pRakClient,ChatBubble);
	REGISTER_STATIC_RPC(pRakClient,CreateLabel);
	REGISTER_STATIC_RPC(pRakClient,DestroyLabel);
	REGISTER_STATIC_RPC(pRakClient,VehicleParams);
	REGISTER_STATIC_RPC(pRakClient,SetTimer);
}

//----------------------------------------------------

/*void UnRegisterRPCs(RakClientInterface * pRakClient)
{
	UNREGISTER_STATIC_RPC(pRakClient,ServerJoin);
	UNREGISTER_STATIC_RPC(pRakClient,ServerQuit);
	UNREGISTER_STATIC_RPC(pRakClient,InitGame);
	UNREGISTER_STATIC_RPC(pRakClient,Chat);
	UNREGISTER_STATIC_RPC(pRakClient,RequestClass);
	UNREGISTER_STATIC_RPC(pRakClient,RequestSpawn);
	UNREGISTER_STATIC_RPC(pRakClient,Spawn);
	UNREGISTER_STATIC_RPC(pRakClient,Death);
	UNREGISTER_STATIC_RPC(pRakClient,EnterVehicle);
	UNREGISTER_STATIC_RPC(pRakClient,ExitVehicle);
	UNREGISTER_STATIC_RPC(pRakClient,VehicleSpawn);
	UNREGISTER_STATIC_RPC(pRakClient,VehicleDestroy);
	UNREGISTER_STATIC_RPC(pRakClient,SetCheckpoint);
	UNREGISTER_STATIC_RPC(pRakClient,DisableCheckpoint);
	UNREGISTER_STATIC_RPC(pRakClient,SetRaceCheckpoint);
	UNREGISTER_STATIC_RPC(pRakClient,DisableRaceCheckpoint);
	UNREGISTER_STATIC_RPC(pRakClient,UpdateScoresPingsIPs);
	UNREGISTER_STATIC_RPC(pRakClient,SvrStats);
	UNREGISTER_STATIC_RPC(pRakClient,GameModeRestart);
	UNREGISTER_STATIC_RPC(pRakClient,ConnectionRejected);
	UNREGISTER_STATIC_RPC(pRakClient,ClientMessage);
	UNREGISTER_STATIC_RPC(pRakClient,WorldTime);
	UNREGISTER_STATIC_RPC(pRakClient,Pickup);
	UNREGISTER_STATIC_RPC(pRakClient,DestroyPickup);
	UNREGISTER_STATIC_RPC(pRakClient,DestroyWeaponPickup);
	UNREGISTER_STATIC_RPC(pRakClient,ScmEvent);
	UNREGISTER_STATIC_RPC(pRakClient,Weather);
	UNREGISTER_STATIC_RPC(pRakClient,Instagib);
	UNREGISTER_STATIC_RPC(pRakClient,SetTimeEx);
	UNREGISTER_STATIC_RPC(pRakClient,ToggleClock);
	UNREGISTER_STATIC_RPC(pRakClient,ACServerProtected);
}*/

//----------------------------------------------------
