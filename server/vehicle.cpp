/*

	SA:MP Multiplayer Modification
	Copyright 2004-2005 SA:MP Team

    Version: $Id: vehicle.cpp,v 1.5 2006/05/07 15:35:32 kyeman Exp $

*/

#include "main.h"

//----------------------------------------------------------

CVehicle::CVehicle( int iModel, VECTOR *vecPos, float fRotation, int iColor1,
 int iColor2, int iRespawnDelay, bool bAddSiren)
{
	// Store the spawn info.
	m_SpawnInfo.iVehicleType = iModel;
	m_SpawnInfo.fRotation = fRotation;
	m_SpawnInfo.vecPos.X = vecPos->X;
	m_SpawnInfo.vecPos.Y = vecPos->Y;
	m_SpawnInfo.vecPos.Z = vecPos->Z;
	m_SpawnInfo.iColor1 = (iColor1 == -1) ? (rand() % 255) : (iColor1);
	m_SpawnInfo.iColor2 = (iColor2 == -1) ? (rand() % 255) : (iColor2);
	m_SpawnInfo.iRespawnDelay = iRespawnDelay;
	m_SpawnInfo.iInterior = 0;

	m_bHasSiren = (VehicleModelWithSiren(iModel) || bAddSiren) ? true : false;

	m_bHasBeenOccupied = false;
	m_dwLastRespawnedTick = GetTickCount();

	// Set the initial pos to spawn pos.
	memset(&m_matWorld,0,sizeof(MATRIX4X4));
	m_matWorld.pos.X = m_SpawnInfo.vecPos.X;
	m_matWorld.pos.Y = m_SpawnInfo.vecPos.Y;
	m_matWorld.pos.Z = m_SpawnInfo.vecPos.Z;

	memset(&m_vecMoveSpeed,0,sizeof(VECTOR));
	memset(&m_vecTurnSpeed,0,sizeof(VECTOR));
	memset(&m_CarModInfo,  0,sizeof(CAR_MOD_INFO));
	memset(&m_szNumberPlate[0],0,sizeof(m_szNumberPlate));

	m_bIsActive = true;
	m_bIsWasted = false;
	m_byteDriverID = INVALID_ID;
	m_ucKillerID = INVALID_ID;
	m_fHealth = 1000.0f;
	m_bDeathHasBeenNotified = false;
	m_iVirtualWorld = 0;
	m_Windows = { 1, 1, 1, 1 }; // Close all window 
	m_Doors = { 0,0,0,0 }; // Close all doors

	m_iPanelDamageStatus = 0;
	m_iDoorDamageStatus = 0;
	m_ucLightDamageStatus = 0;
	m_ucTireDamageStatus = 0;

	m_bOnItsSide = false;
	m_bUpsideDown = false;
	m_bSirenOn = false;
	m_bWrecked = false;
	m_bSunked = false;

	//m_CarModInfo.iColor0 = iColor1;
	//m_CarModInfo.iColor1 = iColor2;

	m_TrailerID = 0;
	m_CabID = 0;
	m_bDead = false;
	bOldSirenState = false; // disabled

	m_Params.byteAlarm = VEHICLE_PARAMS_UNSET;
	m_Params.byteBonnet = VEHICLE_PARAMS_UNSET;
	m_Params.byteBoot = VEHICLE_PARAMS_UNSET;
	m_Params.byteDoors = VEHICLE_PARAMS_UNSET;
	m_Params.byteEngine = VEHICLE_PARAMS_UNSET;
	m_Params.byteLights = VEHICLE_PARAMS_UNSET;
	m_Params.byteObjective = VEHICLE_PARAMS_UNSET;
	m_Params.byteSiren = VEHICLE_PARAMS_UNSET;
	m_Params.byteDriverDoor = VEHICLE_PARAMS_UNSET;
	m_Params.bytePassengerDoor = VEHICLE_PARAMS_UNSET;
	m_Params.byteBackLeftDoor = VEHICLE_PARAMS_UNSET;
	m_Params.byteBackRightDoor = VEHICLE_PARAMS_UNSET;
	m_Params.byteDriverWindow = VEHICLE_PARAMS_UNSET;
	m_Params.bytePassengerWindow = VEHICLE_PARAMS_UNSET;
	m_Params.byteBackLeftWindow = VEHICLE_PARAMS_UNSET;
	m_Params.byteBackRightWindow = VEHICLE_PARAMS_UNSET;
}

//----------------------------------------------------
// Updates our stored data structures for this
// network vehicle.

void CVehicle::Update(BYTE bytePlayerID, MATRIX4X4 * matWorld, float fHealth, VEHICLEID TrailerID)
{
	// we should ignore any updates if it recently respawned
	if((GetTickCount() - m_dwLastRespawnedTick) < 5000) return;

	m_byteDriverID = bytePlayerID;
	memcpy(&m_matWorld,matWorld,sizeof(MATRIX4X4));
	m_fHealth = fHealth;

	if(TrailerID < MAX_VEHICLES) {
		CVehicle* Trailer;
		if (TrailerID) Trailer = pNetGame->GetVehiclePool()->GetAt(TrailerID);
		else Trailer = pNetGame->GetVehiclePool()->GetAt(m_TrailerID);
		if (Trailer) Trailer->SetCab(m_VehicleID);
		m_TrailerID = TrailerID;
	} else {
		m_TrailerID = 0;
	}
	m_dwLastSeenOccupiedTick = GetTickCount();
	m_bHasBeenOccupied = true;

	if(m_fHealth <= 0.0f) m_bDead = true;
}

//----------------------------------------------------
// Stores and broadcast vehicle damage statuses

void CVehicle::UpdateDamage(PLAYERID PlayerID, int iPanels, int iDoors, unsigned char ucLights, unsigned char ucTires)
{
	RakNet::BitStream bsSend;

	m_iDoorDamageStatus = iDoors;
	m_iPanelDamageStatus = iPanels;
	m_ucLightDamageStatus = ucLights;
	m_ucTireDamageStatus = ucTires;

	bsSend.Write(m_VehicleID);
	bsSend.Write(m_iPanelDamageStatus);
	bsSend.Write(m_iDoorDamageStatus);
	bsSend.Write(m_ucLightDamageStatus);
	bsSend.Write(m_ucTireDamageStatus);

	if (PlayerID != INVALID_PLAYER_ID)
	{
		if (pNetGame->GetFilterScripts())
			pNetGame->GetFilterScripts()->OnVehicleDamageStatusUpdate(m_VehicleID, PlayerID);

		if (pNetGame->GetGameMode())
			pNetGame->GetGameMode()->OnVehicleDamageStatusUpdate(m_VehicleID, PlayerID);
	}

	pNetGame->BroadcastVehicleRPC(RPC_VehicleDamage, &bsSend, m_VehicleID, PlayerID);
}

//----------------------------------------------------
// This is the method used for spawning players that
// already exist in the world when the client connects.

void CVehicle::SpawnForPlayer(BYTE byteForPlayerID)
{
	RakNet::BitStream bsVehicleSpawn;
	
	CAR_MOD_INFO CarModInfo;
    memset(&CarModInfo,0,sizeof(CAR_MOD_INFO));

	bsVehicleSpawn.Write(m_VehicleID);
	bsVehicleSpawn.Write(m_SpawnInfo.iVehicleType);
	bsVehicleSpawn.Write(m_matWorld.pos.X);
	bsVehicleSpawn.Write(m_matWorld.pos.Y);
	bsVehicleSpawn.Write(m_matWorld.pos.Z);
	bsVehicleSpawn.Write(m_SpawnInfo.fRotation);
	bsVehicleSpawn.Write(m_SpawnInfo.iColor1);
	bsVehicleSpawn.Write(m_SpawnInfo.iColor2);
	bsVehicleSpawn.Write(m_fHealth);

	// now add spawn co-ords and rotation
	bsVehicleSpawn.Write(m_SpawnInfo.vecPos.X);
	bsVehicleSpawn.Write(m_SpawnInfo.vecPos.Y);
	bsVehicleSpawn.Write(m_SpawnInfo.vecPos.Z);
	bsVehicleSpawn.Write(m_SpawnInfo.fRotation);
	bsVehicleSpawn.Write(m_SpawnInfo.iInterior);

	bsVehicleSpawn.Write(m_bHasSiren);

	bsVehicleSpawn.WriteBits((unsigned char*)&m_Windows, 4);
	bsVehicleSpawn.WriteBits((unsigned char*)&m_Doors, 4);

	if(m_szNumberPlate[0] == '\0') {
		bsVehicleSpawn.Write(false);
	} else {
		bsVehicleSpawn.Write(true);
		bsVehicleSpawn.Write((PCHAR)m_szNumberPlate, 9);
	}
	if(!memcmp((void *)&m_CarModInfo,(void *)&CarModInfo,sizeof(CAR_MOD_INFO))) {
		bsVehicleSpawn.Write(false);
	} else {
		bsVehicleSpawn.Write(true);
		bsVehicleSpawn.Write((PCHAR)&m_CarModInfo, sizeof(m_CarModInfo));
	}

	pNetGame->GetRakServer()->RPC(RPC_VehicleSpawn ,&bsVehicleSpawn,HIGH_PRIORITY,RELIABLE,
		0,pNetGame->GetRakServer()->GetPlayerIDFromIndex(byteForPlayerID),false,false);

	if(HasParamsSet()) SendParams(byteForPlayerID);
}

//----------------------------------------------------------

// wPlayerID == INVALID_PLAYER_ID broadcasts to everyone
void CVehicle::SendParams(WORD wPlayerID)
{
	RakNet::BitStream bsParams;

	bsParams.Write(m_VehicleID);
	bsParams.Write((PCHAR)&m_Params, sizeof(VEHICLE_PARAMS));

	if(wPlayerID == INVALID_PLAYER_ID) {
		pNetGame->SendToAll(RPC_VehicleParams, &bsParams);
	} else {
		pNetGame->SendToPlayer(wPlayerID, RPC_VehicleParams, &bsParams);
	}
}

//----------------------------------------------------------

bool CVehicle::IsOccupied()
{
	CPlayer *pPlayer;

	// find drivers or passengers of this vehicle
	int x=0;
	while(x!=MAX_PLAYERS) {
		if(pNetGame->GetPlayerPool()->GetSlotState(x)) {
			pPlayer = pNetGame->GetPlayerPool()->GetAt(m_byteDriverID);

			if( pPlayer && (pPlayer->m_VehicleID == m_VehicleID) &&
				 (pPlayer->GetState() == PLAYER_STATE_DRIVER ||
				 pPlayer->GetState() == PLAYER_STATE_PASSENGER) ) {
					 return true;
			}
		}
		x++;
	}

	return false;
}

//----------------------------------------------------------

bool CVehicle::IsATrainPart()
{
	int nModel = m_SpawnInfo.iVehicleType;

	if(nModel == TRAIN_PASSENGER_LOCO) return true;
	if(nModel == TRAIN_PASSENGER) return true;
	if(nModel == TRAIN_FREIGHT_LOCO) return true;
	if(nModel == TRAIN_FREIGHT) return true;
	if(nModel == TRAIN_TRAM) return true;

	return false;
}

//----------------------------------------------------------

void CVehicle::CheckForIdleRespawn()
{	
	// can't respawn an idle train or train part
	if(IsATrainPart()) return;

	if( (GetTickCount() - m_dwLastRespawnedTick) < 10000 ) {
		// We should wait at least 10 seconds after the last
		// respawn before checking. Come back later.
		return;
	}

	if(!IsOccupied()) {
		if( m_bHasBeenOccupied &&
			(GetTickCount() - m_dwLastSeenOccupiedTick) >= (DWORD)m_SpawnInfo.iRespawnDelay ) {
			//printf("Respawning idle vehicle %u\n",m_VehicleID);
			Respawn();
		}
	}
}

//----------------------------------------------------------

void CVehicle::Process(float fElapsedTime)
{
	// Check for an idle vehicle.. but don't do this
	// every ::Process because it would be too CPU intensive.
	if(!m_bDeathHasBeenNotified && m_SpawnInfo.iRespawnDelay != (-1) && ((rand() % 20) == 0)) {
		CheckForIdleRespawn();
		return;
	}
	
	// we'll check for a dead vehicle.
	if(m_bDead)
	{
		if (!m_bDeathHasBeenNotified)
		{
			m_bDeathHasBeenNotified = true;
			m_szNumberPlate[0] = 0;

			if(pNetGame->GetGameMode() && pNetGame->GetFilterScripts()) {
				pNetGame->GetFilterScripts()->OnVehicleDeath(m_VehicleID, m_ucKillerID);
				pNetGame->GetGameMode()->OnVehicleDeath(m_VehicleID, m_ucKillerID);
			}
			m_dwLastSeenOccupiedTick = GetTickCount();
			m_ucKillerID = INVALID_ID;
		}
		if(!(rand() % 20) && GetTickCount() - m_dwLastSeenOccupiedTick > 10000)
		{
			Respawn();
		}
	}
}

//----------------------------------------------------------

void CVehicle::Respawn()
{
	RakServerInterface *pRak = pNetGame->GetRakServer();

	memset(&m_CarModInfo,  0, sizeof (CAR_MOD_INFO));
	m_CarModInfo.iColor0 = m_SpawnInfo.iColor1;
	m_CarModInfo.iColor1 = m_SpawnInfo.iColor2;
	m_matWorld.pos.X = m_SpawnInfo.vecPos.X;
	m_matWorld.pos.Y = m_SpawnInfo.vecPos.Y;
	m_matWorld.pos.Z = m_SpawnInfo.vecPos.Z;
				
	RakNet::BitStream bsVehicle;
	bsVehicle.Write(m_VehicleID);
	pRak->RPC(RPC_ScrRespawnVehicle , &bsVehicle, HIGH_PRIORITY, 
		RELIABLE, 0, UNASSIGNED_PLAYER_ID, true, false);
	
	m_bDead = false;
	m_bDeathHasBeenNotified = false;
	m_bHasBeenOccupied = false;
	m_dwLastRespawnedTick = GetTickCount();

	if(pNetGame->GetFilterScripts() && pNetGame->GetGameMode()) {
		pNetGame->GetFilterScripts()->OnVehicleSpawn(m_VehicleID);
		pNetGame->GetGameMode()->OnVehicleSpawn(m_VehicleID);
	}
}

//----------------------------------------------------------

void CVehicle::SetHealth(float fHealth)
{
	m_fHealth = fHealth;
	RakNet::BitStream bsReturn;
	bsReturn.Write(m_VehicleID);
	bsReturn.Write(fHealth);
	pNetGame->GetRakServer()->RPC(RPC_ScrSetVehicleHealth , &bsReturn, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true, false);
}

//----------------------------------------------------------

void CVehicle::SetNumberPlate(PCHAR Plate)
{
	strcpy(m_szNumberPlate, Plate);
	RakNet::BitStream bsPlate;
	bsPlate.Write(m_VehicleID);
	bsPlate.Write(Plate, 9);
	pNetGame->GetRakServer()->RPC(RPC_ScrNumberPlate , &bsPlate, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true, false);
}

//----------------------------------------------------------

float CVehicle::GetDistanceFromPoint(float fX, float fY, float fZ)
{
	float
		x = m_matWorld.pos.X - fX,
		y = m_matWorld.pos.Y - fY,
		z = m_matWorld.pos.Z - fZ;

	return sqrtf(z * z + y * y + x * x);
}

//----------------------------------------------------------

float CVehicle::GetSquaredDistanceFrom2DPoint(float fX, float fY)
{
	float
		fDX = m_matWorld.pos.X - fX,
		fDY = m_matWorld.pos.Y - fY;

	return fDY * fDY + fDX * fDX;
}

//----------------------------------------------------------

void CVehicle::SetVirtualWorld(int iVirtualWorld)
{
	m_iVirtualWorld = iVirtualWorld;

	RakNet::BitStream bsData;
	bsData.Write(m_VehicleID); // player id
	bsData.Write(iVirtualWorld); // vw id
	pNetGame->SendToAll(RPC_ScrSetVehicleVirtualWorld, &bsData);

}

//----------------------------------------------------------

bool CVehicle::HasParamsSet()
{
	VEHICLE_PARAMS ParamsUnset;

	// the fields are BYTE, so an unset one holds 0xFF and never compares equal to -1
	memset(&ParamsUnset, VEHICLE_PARAMS_UNSET, sizeof(VEHICLE_PARAMS));

	return memcmp(&m_Params, &ParamsUnset, sizeof(VEHICLE_PARAMS)) != 0;
}

//----------------------------------------------------------

bool CVehicle::HandleSiren(unsigned char ucPlayerId, bool bSirenState)
{
	if (m_bHasSiren) {
		if (bOldSirenState != bSirenState) {
			CFilterScripts* pFilterScripts = pNetGame->GetFilterScripts();
			CGameMode* pGameMode = pNetGame->GetGameMode();
			int ret = 0;
			if (pFilterScripts)
				ret = pFilterScripts->OnVehicleSirenStateChange(ucPlayerId, m_VehicleID, bSirenState);
			if (pGameMode && !ret)
				pGameMode->OnVehicleSirenStateChange(ucPlayerId, m_VehicleID, bSirenState);

			bOldSirenState = bSirenState;
		}
		return true;
	}
	return false;
}

//----------------------------------------------------------

void CVehicle::SetColor(WORD wPlayerID, int iColor1, int iColor2)
{
	// Additional/Fix: Setting the colors here, otherwise client will generate different
	// color for every clients, making it looked like it's "desyncronized".
	int _iColor1 = (iColor1 != -1 && (DWORD)iColor1 < 256) ? iColor1 : RANDOM_VEHICLE_COLOR;
	int _iColor2 = (iColor2 != -1 && (DWORD)iColor2 < 256) ? iColor2 : RANDOM_VEHICLE_COLOR;

	if (wPlayerID == INVALID_PLAYER_ID ||
		pNetGame->GetGameMode() &&
		pNetGame->GetFilterScripts() &&
		pNetGame->GetGameMode()->OnVehicleRespray(wPlayerID, m_VehicleID, _iColor1, _iColor2) &&
		pNetGame->GetFilterScripts()->OnVehicleRespray(wPlayerID, m_VehicleID, _iColor1, _iColor2))
	{
		m_CarModInfo.iColor0 = _iColor1;
		m_CarModInfo.iColor1 = _iColor2;

		RakNet::BitStream bsData;
		bsData.Write(wPlayerID);
		bsData.Write((int)EVENT_TYPE_CARCOLOR);
		bsData.Write((DWORD)m_VehicleID);
		bsData.Write(_iColor1);
		bsData.Write(_iColor2);

		// TODO: Change it to vehicle broadcast rpc function from CNetGame
		pNetGame->SendToAll(RPC_ScmEvent, &bsData);
	}
}

void CVehicle::SetPaintjob(WORD wPlayerID, int iPaintjob)
{
	if (wPlayerID == INVALID_PLAYER_ID ||
		pNetGame->GetGameMode() &&
		pNetGame->GetFilterScripts() &&
		pNetGame->GetGameMode()->OnVehiclePaintjob(wPlayerID, m_VehicleID, iPaintjob) &&
		pNetGame->GetFilterScripts()->OnVehiclePaintjob(wPlayerID, m_VehicleID, iPaintjob))
	{
		m_CarModInfo.bytePaintJob = iPaintjob + 1;
		
		RakNet::BitStream bsData;
		bsData.Write(wPlayerID);
		bsData.Write((int)EVENT_TYPE_PAINTJOB);
		bsData.Write((DWORD)m_VehicleID);
		bsData.Write((DWORD)iPaintjob);
		bsData.Write((DWORD)0);

		// TODO: Change it to vehicle broadcast rpc function from CNetGame
		pNetGame->SendToAll(RPC_ScmEvent, &bsData);
	}
}
