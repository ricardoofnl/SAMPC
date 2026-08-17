//----------------------------------------------------------
//
// SA:MP Multiplayer Modification For GTA:SA
// Copyright 2004-2005 SA:MP team
//
// Version: $Id: vehiclepool.cpp,v 1.29 2006/05/07 17:32:29 kyeman Exp $
//
//----------------------------------------------------------

#include "../main.h"
#include "../game/util.h"

//----------------------------------------------------

CVehiclePool::CVehiclePool()
{
	// loop through and initialize all vehicle properties to 0
	for(VEHICLEID VehicleID = 0; VehicleID < MAX_VEHICLES; VehicleID++) {
		m_bVehicleSlotState[VehicleID] = false;
		m_pVehicles[VehicleID] = NULL;
		m_iVirtualWorld[VehicleID] = 0;
		m_Windows[VehicleID] = { 1,1,1,1 };
		m_Doors[VehicleID] = { 0,0,0,0 };
		m_bHasSiren[VehicleID] = false;
		ResetParams(VehicleID);
	}
	m_bRebuildPlateTextures = true;
	m_PoolSize = INVALID_VEHICLE_ID;
}

//----------------------------------------------------

CVehiclePool::~CVehiclePool()
{
	for(VEHICLEID VehicleID = 0; VehicleID < MAX_VEHICLES; VehicleID++) {
		Delete(VehicleID);
	}
}

//----------------------------------------------------

void CVehiclePool::UpdatePoolSize()
{
	VEHICLEID LastID=INVALID_VEHICLE_ID;
	for (VEHICLEID VehicleID = 0; VehicleID < MAX_VEHICLES; VehicleID++)
	{
		if (m_bVehicleSlotState[VehicleID])
			LastID = VehicleID;
	}
	m_PoolSize = LastID;
}

//----------------------------------------------------

bool CVehiclePool::New( VEHICLEID VehicleID, int iVehicleType,
					    VECTOR * vecPos, float fRotation,
					    int iColor1, int iColor2,
					    VECTOR * vecSpawnPos, float fSpawnRotation, /*int iRespawnDelay,*/
						int iInterior, PCHAR szNumberPlate )
{
	memset(&m_SpawnInfo[VehicleID],0,sizeof(VEHICLE_SPAWN_INFO));

	// Setup the spawninfo for the next respawn.
	m_SpawnInfo[VehicleID].iVehicleType = iVehicleType;
	m_SpawnInfo[VehicleID].vecPos.X = vecSpawnPos->X;
	m_SpawnInfo[VehicleID].vecPos.Y = vecSpawnPos->Y;
	m_SpawnInfo[VehicleID].vecPos.Z = vecSpawnPos->Z;
	m_SpawnInfo[VehicleID].fRotation = fSpawnRotation;
	m_SpawnInfo[VehicleID].iColor1 = iColor1;
	m_SpawnInfo[VehicleID].iColor2 = iColor2;
	
	m_iVirtualWorld[VehicleID] = 0;

	// New vehicle added, resetting values...
	m_Windows[VehicleID] = { 1,1,1,1 };
	m_Doors[VehicleID] = { 0,0,0,0 };
	ResetParams(VehicleID);

	// Now go ahead and spawn it at the location we got passed.
	return Spawn(VehicleID,iVehicleType,vecPos,fRotation,iColor1,iColor2,iInterior,szNumberPlate);
}

//----------------------------------------------------

bool CVehiclePool::New(VEHICLE_TRANSMIT* pTransmit)
{
	if (m_pVehicles[pTransmit->VehicleID])
	{
		pChatWindow->AddDebugMessage("Warning: vehicle %u was not deleted", pTransmit->VehicleID);
		Delete(pTransmit->VehicleID);
	}

	CVehicle* pVehicle = pGame->NewVehicle(
		pTransmit->iModelID,
		pTransmit->vecPos.X,
		pTransmit->vecPos.Y,
		pTransmit->vecPos.Z,
		pTransmit->fRotation,
		NULL); // TODO: Replace szNumberPlate to bWantSiren?

	if (pVehicle)
	{
		if (pTransmit->byteColor1 != -1 || pTransmit->byteColor2 != -1)
			pVehicle->SetColor(pTransmit->byteColor1, pTransmit->byteColor2);

		pVehicle->SetHealth(pTransmit->fHealth);

		if (pTransmit->byteInterior)
		{
			//if (m_bVehicleSlotState[pTransmit->VehicleID])
			{
				pVehicle->LinkToInterior(pTransmit->byteInterior);
			}
		}

		if (pTransmit->dwPanelsState ||
			pTransmit->dwDoorsState ||
			pTransmit->byteLightsState)
		{
			pVehicle->UpdateDamage(pTransmit->dwPanelsState,
				pTransmit->dwDoorsState, pTransmit->byteLightsState);
		}

		pVehicle->SetCarOrBikeWheelStatus(pTransmit->byteTyresState);

		m_pVehicles[pTransmit->VehicleID] = pVehicle;
	}
	return false;
}

//----------------------------------------------------

bool CVehiclePool::Delete(VEHICLEID VehicleID)
{
	if(!GetSlotState(VehicleID) || !m_pVehicles[VehicleID])
	{
		return false; // Vehicle already deleted or not used.
	}

	//m_pVehicles[VehicleID]->DestroyNumberPlateTexture();

	m_bVehicleSlotState[VehicleID] = false;
	delete m_pVehicles[VehicleID];
	m_pVehicles[VehicleID] = NULL;

	UpdatePoolSize();

	return true;
}

//----------------------------------------------------

bool CVehiclePool::Spawn( VEHICLEID VehicleID, int iVehicleType,
					      VECTOR * vecPos, float fRotation,
					      int iColor1, int iColor2, int iInterior, PCHAR szNumberPlate, int iObjective,
						  int iDoorsLocked )
{	

	if(m_pVehicles[VehicleID] != NULL) {
		Delete(VehicleID);
	}

	m_pVehicles[VehicleID] = pGame->NewVehicle(iVehicleType,
		vecPos->X,vecPos->Y,vecPos->Z,fRotation, szNumberPlate);

	if(m_pVehicles[VehicleID])
	{	
		if(iColor1 != -1 || iColor2 != -1) {
			m_pVehicles[VehicleID]->SetColor(iColor1,iColor2);
		}

		m_bVehicleSlotState[VehicleID] = true;

		if(iObjective) m_pVehicles[VehicleID]->m_byteObjectiveVehicle = 1;
		if(iDoorsLocked) m_pVehicles[VehicleID]->SetDoorState(1);
		if (iInterior > 0)
		{
			LinkToInterior(VehicleID, iInterior);
		}

		// TODO: Need checking at and add model filtering here and/or server
		// Seems like it works on most of the vehicles, but on some vehicles it crashes the game,
		// with gta_sa.exe:0x6D30B5 crash address. ecx at [ecx+18h] looks like not initialized.
		if (m_pVehicles[VehicleID]->GetVehicleSubtype() == VEHICLE_SUBTYPE_CAR) {
			m_pVehicles[VehicleID]->ToggleWindow(10, m_Windows[VehicleID].bDriver);
			m_pVehicles[VehicleID]->ToggleWindow(8, m_Windows[VehicleID].bPassenger);
			m_pVehicles[VehicleID]->ToggleWindow(11, m_Windows[VehicleID].bBackLeft);
			m_pVehicles[VehicleID]->ToggleWindow(9, m_Windows[VehicleID].bBackRight);
		}

		// Wiki: 1 to open, 0 to close
		m_pVehicles[VehicleID]->ToggleDoor(2, 10, m_Doors[VehicleID].bDriver ? 1.0f : 0.0f);
		m_pVehicles[VehicleID]->ToggleDoor(3, 8, m_Doors[VehicleID].bPassenger ? 1.0f : 0.0f);
		m_pVehicles[VehicleID]->ToggleDoor(4, 11, m_Doors[VehicleID].bBackLeft ? 1.0f : 0.0f);
		m_pVehicles[VehicleID]->ToggleDoor(5, 9, m_Doors[VehicleID].bBackRight ? 1.0f : 0.0f);

		// the game vehicle is brand new here, so whatever the server told us before is gone
		ApplyParams(VehicleID);

		m_bIsActive[VehicleID] = true;
		m_bIsWasted[VehicleID] = false;
		m_charNumberPlate[VehicleID][0] = 0;

		return true;
	}
	else
	{
		return false;
	}
}

void CVehiclePool::LinkToInterior(VEHICLEID VehicleID, int iInterior)
{
	if(m_bVehicleSlotState[VehicleID]) {
		m_SpawnInfo[VehicleID].iInterior = iInterior;
		m_pVehicles[VehicleID]->LinkToInterior(iInterior);
	}
}

//----------------------------------------------------

void CVehiclePool::AssignSpecialParamsToVehicle(VEHICLEID VehicleID, BYTE byteObjective, BYTE byteDoorsLocked)
{
	if(!GetSlotState(VehicleID)) return;

	m_SpawnInfo[VehicleID].iObjective = byteObjective;
	m_SpawnInfo[VehicleID].iDoorsLocked = byteDoorsLocked;
	
	CVehicle *pVehicle = m_pVehicles[VehicleID];

	if(pVehicle && m_bIsActive[VehicleID]) {
		if (byteObjective) {
			pVehicle->m_byteObjectiveVehicle = 1;
			pVehicle->m_bSpecialMarkerEnabled = false;
		} else {
			pVehicle->m_byteObjectiveVehicle = 0;
			pVehicle->m_bSpecialMarkerEnabled = true;
		}
		pVehicle->SetDoorState(byteDoorsLocked);
	}
}

//----------------------------------------------------

void CVehiclePool::ResetParams(VEHICLEID VehicleID)
{
	if(VehicleID >= MAX_VEHICLES) return;

	memset(&m_Params[VehicleID], VEHICLE_PARAMS_UNSET, sizeof(VEHICLE_PARAMS));
}

//----------------------------------------------------

void CVehiclePool::ApplyParams(VEHICLEID VehicleID)
{
	CVehicle *pVehicle = GetAt(VehicleID);
	if(!pVehicle) return;

	VEHICLE_PARAMS *pParams = &m_Params[VehicleID];

	if(pParams->byteEngine == VEHICLE_PARAMS_ON)
		pVehicle->SetEngine(true);
	else if(pParams->byteEngine == VEHICLE_PARAMS_OFF)
		pVehicle->SetEngine(false);

	if(pParams->byteLights == VEHICLE_PARAMS_ON)
		pVehicle->SetLights(true);
	else if(pParams->byteLights == VEHICLE_PARAMS_OFF)
		pVehicle->SetLights(false);

	if(pParams->byteDoors == VEHICLE_PARAMS_ON)
		pVehicle->SetDoorState(1);
	else if(pParams->byteDoors == VEHICLE_PARAMS_OFF)
		pVehicle->SetDoorState(0);

	if(pParams->byteObjective == VEHICLE_PARAMS_ON)
		pVehicle->SetObjective(true);
	else if(pParams->byteObjective == VEHICLE_PARAMS_OFF)
		pVehicle->SetObjective(false);

	if(pParams->byteBoot == VEHICLE_PARAMS_ON)
		pVehicle->ToggleDoor(1, 17, 1.0f);
	else if(pParams->byteBoot == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleDoor(1, 17, 0.0f);

	if(pParams->byteBonnet == VEHICLE_PARAMS_ON)
		pVehicle->ToggleDoor(1, 16, 1.0f);
	else if(pParams->byteBonnet == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleDoor(1, 16, 0.0f);

	if(pParams->byteAlarm == VEHICLE_PARAMS_ON)
	{
		pVehicle->SetSirenOn(TRUE);
		pVehicle->SetAlarmState(20000);
	}
	else if(pParams->byteAlarm == VEHICLE_PARAMS_OFF)
	{
		pVehicle->SetSirenOn(FALSE);
		pVehicle->SetAlarmState(0);
	}

	if(pParams->byteSiren == VEHICLE_PARAMS_ON)
		pVehicle->SetSirenOn(TRUE);
	else if(pParams->byteSiren == VEHICLE_PARAMS_OFF)
		pVehicle->SetSirenOn(FALSE);

	if(pParams->byteDriverDoor == VEHICLE_PARAMS_ON)
		pVehicle->ToggleDoor(2, 10, 1.0f);
	else if(pParams->byteDriverDoor == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleDoor(2, 10, 0.0f);

	if(pParams->bytePassengerDoor == VEHICLE_PARAMS_ON)
		pVehicle->ToggleDoor(3, 8, 1.0f);
	else if(pParams->bytePassengerDoor == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleDoor(3, 8, 0.0f);

	if(pParams->byteBackLeftDoor == VEHICLE_PARAMS_ON)
		pVehicle->ToggleDoor(4, 11, 1.0f);
	else if(pParams->byteBackLeftDoor == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleDoor(4, 11, 0.0f);

	if(pParams->byteBackRightDoor == VEHICLE_PARAMS_ON)
		pVehicle->ToggleDoor(5, 9, 1.0f);
	else if(pParams->byteBackRightDoor == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleDoor(5, 9, 0.0f);

	// TODO: Need checking at and add model filtering here and/or server
	// Seems like it works on most of the vehicles, but on some vehicles it crashes the game,
	// with gta_sa.exe:0x6D30B5 crash address. ecx at [ecx+18h] looks like not initialized.
	if(pVehicle->GetVehicleSubtype() != VEHICLE_SUBTYPE_CAR) return;

	if(pParams->byteDriverWindow == VEHICLE_PARAMS_ON)
		pVehicle->ToggleWindow(10, true);
	else if(pParams->byteDriverWindow == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleWindow(10, false);

	if(pParams->bytePassengerWindow == VEHICLE_PARAMS_ON)
		pVehicle->ToggleWindow(8, true);
	else if(pParams->bytePassengerWindow == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleWindow(8, false);

	if(pParams->byteBackLeftWindow == VEHICLE_PARAMS_ON)
		pVehicle->ToggleWindow(11, true);
	else if(pParams->byteBackLeftWindow == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleWindow(11, false);

	if(pParams->byteBackRightWindow == VEHICLE_PARAMS_ON)
		pVehicle->ToggleWindow(9, true);
	else if(pParams->byteBackRightWindow == VEHICLE_PARAMS_OFF)
		pVehicle->ToggleWindow(9, false);
}

//----------------------------------------------------

VEHICLEID CVehiclePool::FindIDFromGtaPtr(VEHICLE_TYPE * pGtaVehicle)
{
	int x=1;
	
	while(x!=MAX_VEHICLES) {
		if(m_pVehicles[x] != NULL && pGtaVehicle == m_pVehicles[x]->m_pVehicle) return x;
		x++;
	}

	return INVALID_VEHICLE_ID;
}

//----------------------------------------------------

int CVehiclePool::FindGtaIDFromID(int iID)
{
	if (m_pVehicles[iID] == NULL)
		return 0;

	return GamePool_Vehicle_GetIndex(m_pVehicles[iID]->m_pVehicle);
}

//----------------------------------------------------

int CVehiclePool::FindGtaIDFromGtaPtr(VEHICLE_TYPE * pGtaVehicle)
{
	return GamePool_Vehicle_GetIndex(pGtaVehicle);
}

//----------------------------------------------------

void CVehiclePool::ProcessForVirtualWorld(VEHICLEID vehicleId, int iPlayerWorld)
{
	int iVehicleVW = m_iVirtualWorld[vehicleId];
	if (iPlayerWorld != iVehicleVW)
	{
		if(m_pVehicles[vehicleId]->m_dwMarkerID)
		{
			ScriptCommand(&disable_marker, m_pVehicles[vehicleId]->m_dwMarkerID);
			m_pVehicles[vehicleId]->m_dwMarkerID = 0;
		}
	}
}

//-----------------------------------------------------------

void CVehiclePool::ProcessLicensePlateTextures()
{
	if(m_bRebuildPlateTextures)
	{
		if (pLicensePlate && !pLicensePlate->m_pDefaultTexture)
			pLicensePlate->m_pDefaultTexture = pLicensePlate->Make("XYZSR998");

		for (VEHICLEID VehicleID = 0; VehicleID <= m_PoolSize; VehicleID++) {
			if (m_bVehicleSlotState[VehicleID]) {
				//m_pVehicles[VehicleID]->UpdatePlateTexture();
			}
		}
	}
}

//-----------------------------------------------------------

/*void CVehiclePool::DestroyNumberPlateTextures()
{
	for (VEHICLEID VehicleID = 0; VehicleID <= m_PoolSize; VehicleID++) {
		if (m_bVehicleSlotState[VehicleID]) {
			m_pVehicles[VehicleID]->DestroyNumberPlateTexture();
		}
	}
}*/

//-----------------------------------------------------------

void CVehiclePool::Process()
{
	// Process all vehicles in the vehicle pool.
	CVehicle *pVehicle;
	DWORD dwThisTime = GetTickCount();
	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();

	//if(!pLocalPlayer->IsActive()) return;
	
	int localVW = 0;
	if (pLocalPlayer) localVW = pLocalPlayer->GetVirtualWorld();

	for(VEHICLEID x = 0; x != MAX_VEHICLES; x++)
	{
		if(GetSlotState(x) == true)
		{
			// It's in use.
			pVehicle = m_pVehicles[x];

			if(m_bIsActive[x])
			{
				/*
				if(!pVehicle->IsOccupied()) {
					pVehicle->ProcessEngineAudio(0);
				}*/

				if(pVehicle->IsDriverLocalPlayer()) {
					pVehicle->SetInvulnerable(false);
				} else {
					pVehicle->SetInvulnerable(true);
				}

				if (pVehicle->GetHealth() == 0.0f) // || pVehicle->IsWrecked()) // It's dead
				{
					if (pLocalPlayer->m_LastVehicle == x) // Notify server of death
					{
						NotifyVehicleDeath(x);
					}
					continue;
				}
				
				// Peter: This caused every vehicle outside the worldbounds
				// that's not occupied to respawn every time this is called.

				/*if(pVehicle->HasExceededWorldBoundries(
					pNetGame->m_WorldBounds[0],pNetGame->m_WorldBounds[1],
					pNetGame->m_WorldBounds[2],pNetGame->m_WorldBounds[3]))
				{
					if (!pVehicle->IsOccupied()) {
						SetForRespawn(x);
						continue;
					}
				}*/

				if( pVehicle->GetVehicleSubtype() != VEHICLE_SUBTYPE_BOAT &&
					// HasSunk() returns TRUE even if touches it
					//pVehicle->HasSunk() ) // Not boat and has sunk.
					// IsWrecked() only returns TRUE if the engine is dead, and not off
					pVehicle->IsWrecked() )
				{
					if (pLocalPlayer->m_LastVehicle == x) {
						NotifyVehicleDeath(x);
					}
					continue;
				}
				
				// Code to respawn vehicle after it has been idle for the amount of time specified
				pVehicle->UpdateLastDrivenTime();

				// Active and in world.

/*		
#ifdef _DEBUG
				CHAR szBuffer2[1024];
				if (!pVehicle->IsAdded() && pVehicle->GetDistanceFromLocalPlayerPed() < LOCKING_DISTANCE) {
					sprintf(szBuffer2, "Vehicle streamed into locking distance: %d:%u\n", x,m_byteVirtualWorld[x]);
					OutputDebugString(szBuffer2);
				}
				if (pVehicle->IsAdded() && pVehicle->GetDistanceFromLocalPlayerPed() >= LOCKING_DISTANCE) {
					sprintf(szBuffer2, "Vehicle streamed out of locking distance: %d:%u\n", x,m_byteVirtualWorld[x]);				
					OutputDebugString(szBuffer2);
				}
#endif */
				// Remove or Add vehicles as they leave/enter a radius around the player
				if( (pVehicle->GetDistanceFromLocalPlayerPed() < LOCKING_DISTANCE)
					&& m_iVirtualWorld[x] == localVW ) {

					pVehicle->Add();
					//pVehicle->SetLockedState(0);
					

					CVehicle* pTrailer = pVehicle->GetTrailer();
					if (pTrailer && !pTrailer->IsAdded())
					{
						MATRIX4X4 matPos;
						pVehicle->GetMatrix(&matPos);
						pTrailer->TeleportTo(matPos.pos.X, matPos.pos.Y, matPos.pos.Z);
						pTrailer->Add();
					}

				} else {
					//pVehicle->SetLockedState(1);
					pVehicle->Remove();					
				}

				pVehicle->ProcessMarkers(); // car scanning shit

				/*
				if( (pVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_PLANE ||
					pVehicle->GetVehicleSubtype() == VEHICLE_SUBTYPE_HELI) &&
					!pVehicle->IsOccupied() ) {
					pVehicle->SetEngineState(FALSE);
				}*/

				if(!pVehicle->HasADriver()) {
					pVehicle->SetHornState(0);
					pVehicle->SetEngineState(false);
				}

				// Put at the END so other processing is still done!
				ProcessForVirtualWorld(x, localVW);
			}
			else // !m_bIsActive
			{
				if(!pVehicle->IsOccupied()) {
					if(m_iRespawnDelay[x] > 0) {
						m_iRespawnDelay[x]--;
					}
					else {
#ifdef _DEBUG
						CHAR szBuffer2[1024];
						sprintf_s(szBuffer2, "Inactive vehicle getting respawned: %d\n", x);
						OutputDebugString(szBuffer2);
#endif
						Spawn(x,m_SpawnInfo[x].iVehicleType,&m_SpawnInfo[x].vecPos, m_SpawnInfo[x].fRotation,
							m_SpawnInfo[x].iColor1,m_SpawnInfo[x].iColor2,m_SpawnInfo[x].iInterior,m_charNumberPlate[x],m_SpawnInfo[x].iObjective,m_SpawnInfo[x].iDoorsLocked);
					}
				}	
			}			
		}
	} // end for each vehicle
}

//----------------------------------------------------

void CVehiclePool::SetForRespawn(VEHICLEID VehicleID, int iRespawnDelay)
{
	CVehicle *pVehicle = m_pVehicles[VehicleID];

	if(pVehicle) {
		m_bIsActive[VehicleID] = false;
		m_bIsWasted[VehicleID] = true;
		m_iRespawnDelay[VehicleID] = iRespawnDelay;
	}
}

//----------------------------------------------------

void CVehiclePool::NotifyVehicleDeath(VEHICLEID VehicleID)
{
	RakNet::BitStream bsDeath;
	bsDeath.Write(VehicleID);
	pNetGame->GetRakClient()->RPC(RPC_VehicleDestroyed, &bsDeath, HIGH_PRIORITY, RELIABLE_SEQUENCED, 0, false);
	pNetGame->GetPlayerPool()->GetLocalPlayer()->m_LastVehicle = 0xFFFF; // Mark as notification sent
}

//----------------------------------------------------

int CVehiclePool::FindNearestToLocalPlayerPed()
{
	float fLeastDistance=10000.0f;
	float fThisDistance;
	VEHICLEID ClosestSoFar=INVALID_VEHICLE_ID;

	VEHICLEID x=0;
	while(x < MAX_VEHICLES) {
		if(GetSlotState(x) && m_bIsActive[x]) {
			fThisDistance = m_pVehicles[x]->GetDistanceFromLocalPlayerPed();
			if(fThisDistance < fLeastDistance) {
				fLeastDistance = fThisDistance;
				ClosestSoFar = x;
			}
		}
		x++;
	}

	return ClosestSoFar;
}

//----------------------------------------------------