/*

	SA:MP Multiplayer Modification
	Copyright 2004-2005 SA:MP Team

    Version: $Id: vehiclepool.cpp,v 1.10 2006/04/12 19:26:45 mike Exp $

*/

#include "main.h"

//----------------------------------------------------

CVehiclePool::CVehiclePool()
{
	// loop through and initialize all net players to null and slot states to false
	for(VEHICLEID VehicleID = 0; VehicleID != MAX_VEHICLES; VehicleID++) {
		m_bVehicleSlotState[VehicleID] = false;
		m_pVehicles[VehicleID] = NULL;
		if (VehicleID < 212)
			m_usVehicleModelsUsed[VehicleID] = 0;
	}
	m_iLastVehicleId = -1;
}

//----------------------------------------------------

CVehiclePool::~CVehiclePool()
{	
	for(VEHICLEID VehicleID = 0; VehicleID != MAX_VEHICLES; VehicleID++) {
		Delete(VehicleID);
	}
}

//----------------------------------------------------

VEHICLEID CVehiclePool::New(int iVehicleType, VECTOR * vecPos, float fRotation,
	int iColor1, int iColor2, int iRespawnDelay, bool bAddSiren)
{
	VEHICLEID VehicleID;

	if (!IsVehicleModelIdValid(iVehicleType))
		return 0xFFFF;

	for(VehicleID=1; VehicleID != MAX_VEHICLES; VehicleID++)
	{
		if(m_bVehicleSlotState[VehicleID] == false) break;
	}

	if(VehicleID == MAX_VEHICLES) return 0xFFFF;		

	m_pVehicles[VehicleID] = new CVehicle(iVehicleType,vecPos,fRotation,iColor1,iColor2,iRespawnDelay, bAddSiren);

	if(m_pVehicles[VehicleID])
	{
		m_pVehicles[VehicleID]->SetID(VehicleID);
		m_bVehicleSlotState[VehicleID] = true;
		//m_byteVirtualWorld[VehicleID] = 0;

		m_iLastVehicleId = -1;
		for (int i = 0; i < MAX_VEHICLES; i++)
		{
			if (m_bVehicleSlotState[i])
			{
				m_iLastVehicleId = i;
			}
		}

		m_usVehicleModelsUsed[iVehicleType - 400]++;

		return VehicleID;
	}
	else
	{
		return 0xFFFF;
	}
}

//----------------------------------------------------

bool CVehiclePool::Delete(VEHICLEID VehicleID)
{
	if(!GetSlotState(VehicleID) || !m_pVehicles[VehicleID])
	{
		return false; // Vehicle already deleted or not used.
	}

	m_usVehicleModelsUsed[m_pVehicles[VehicleID]->m_SpawnInfo.iVehicleType - 400]--;

	m_bVehicleSlotState[VehicleID] = false;
	delete m_pVehicles[VehicleID];
	m_pVehicles[VehicleID] = NULL;

	m_iLastVehicleId = -1;
	for (int i = 0; i < MAX_VEHICLES; i++)
	{
		if (m_bVehicleSlotState[i])
		{
			m_iLastVehicleId = i;
		}
	}
	return true;
}

//----------------------------------------------------

void CVehiclePool::Process(float fElapsedTime)
{
	for (int i=0; i<MAX_VEHICLES; i++)
	{
		if (GetSlotState(i))
		{
			GetAt(i)->Process(fElapsedTime);
		}
	}
}

//----------------------------------------------------

void CVehiclePool::InitForPlayer(BYTE bytePlayerID)
{	
	// Spawn all existing vehicles for player.
	CVehicle *pVehicle;
	VEHICLEID x=0;

	while(x!=MAX_VEHICLES) {
		if(GetSlotState(x)) {
			pVehicle = GetAt(x);
			if(pVehicle->IsActive()) pVehicle->SpawnForPlayer(bytePlayerID);
		}
		x++;
	}
}

//----------------------------------------------------

void CVehiclePool::InitVehicleForPlayer(VEHICLEID VehicleID, WORD wPlayerID)
{
	CVehicle* pVehicle = m_pVehicles[VehicleID];

	if (!m_bVehicleSlotState[VehicleID] || !pVehicle) return;

	RakNet::BitStream bsVehicle;
	VEHICLE_TRANSMIT Vehicle;

	if (pVehicle->m_matWorld.up.X == 0.0f &&
		pVehicle->m_matWorld.up.Y == 0.0f ||
		pVehicle->m_SpawnInfo.iVehicleType == 537 || // VEHICLE_FREIGHT
		pVehicle->m_SpawnInfo.iVehicleType == 538) // VEHICLE_STREAK
	{
		Vehicle.fRotation = pVehicle->m_SpawnInfo.fRotation;
	}
	else
	{
		Vehicle.fRotation = atan2f(pVehicle->m_matWorld.up.Y, -(pVehicle->m_matWorld.up.X));
		Vehicle.fRotation *= (180.0f / PI);

		if (Vehicle.fRotation <= 0.0f)
			Vehicle.fRotation += 360.0f;
		else if (Vehicle.fRotation >= 360.0f)
			Vehicle.fRotation -= 360.0f;
	}

	Vehicle.VehicleID = VehicleID;
	Vehicle.iModelID = pVehicle->m_SpawnInfo.iVehicleType;
	Vehicle.vecPos.X = pVehicle->m_matWorld.pos.X;
	Vehicle.vecPos.Y = pVehicle->m_matWorld.pos.Y;
	Vehicle.vecPos.Z = pVehicle->m_matWorld.pos.Z;
	Vehicle.byteColor1 = (BYTE)pVehicle->m_SpawnInfo.iColor1;
	Vehicle.byteColor2 = (BYTE)pVehicle->m_SpawnInfo.iColor2;
	Vehicle.fHealth = pVehicle->m_fHealth;
	Vehicle.byteInterior = (BYTE)pVehicle->m_SpawnInfo.iInterior;
	Vehicle.dwDoorsState = pVehicle->m_iDoorDamageStatus;
	Vehicle.dwPanelsState = pVehicle->m_iPanelDamageStatus;
	Vehicle.byteLightsState = pVehicle->m_ucLightDamageStatus;
	Vehicle.byteTyresState = pVehicle->m_ucTireDamageStatus;
	Vehicle.byteHasSiren = pVehicle->m_bHasSiren;

	bsVehicle.Write((PCHAR)&Vehicle, sizeof(VEHICLE_TRANSMIT));
	bsVehicle.Write((PCHAR)&pVehicle->m_CarModInfo, sizeof(CAR_MOD_INFO));

	pNetGame->SendToPlayer(wPlayerID, RPC_VehicleSpawn, &bsVehicle);

	BYTE bytePlateLen = (BYTE)strlen(pVehicle->m_szNumberPlate);
	if (bytePlateLen)
	{
		RakNet::BitStream bsPlate;

		bsPlate.Write(VehicleID);
		bsPlate.Write(bytePlateLen);
		bsPlate.Write(pVehicle->m_szNumberPlate, bytePlateLen);

		pNetGame->SendToPlayer(wPlayerID, RPC_ScrNumberPlate, &bsPlate);
	}

	if (pVehicle->HasParamsSet())
	{
		pVehicle->SendParams(wPlayerID);
	}
}

//----------------------------------------------------

void CVehiclePool::DeleteVehicleForPlayer(VEHICLEID VehicleID, WORD wPlayerID)
{
	RakNet::BitStream bsData;

	bsData.Write(VehicleID);

	pNetGame->SendToPlayer(wPlayerID, RPC_VehicleDestroy, &bsData);
}

//----------------------------------------------------

unsigned int CVehiclePool::GetNumberOfModels()
{
	unsigned int uiIndex, uiCount;

	uiCount = 0;
	for (uiIndex = 0; uiIndex < 212; uiIndex++) {
		if (m_usVehicleModelsUsed[uiIndex])
			uiCount++;
	}
	return uiCount;
}

//----------------------------------------------------

/*void CVehiclePool::SetVehicleVirtualWorld(VEHICLEID VehicleID, BYTE byteVirtualWorld)
{
	if (VehicleID >= MAX_VEHICLES) return;
	
	m_byteVirtualWorld[VehicleID] = byteVirtualWorld;
	// Tell existing players it's changed
	RakNet::BitStream bsData;
	bsData.Write(VehicleID); // player id
	bsData.Write(byteVirtualWorld); // vw id
	RakServerInterface *pRak = pNetGame->GetRakServer();
	pRak->RPC(RPC_ScrSetVehicleVirtualWorld , &bsData, HIGH_PRIORITY, RELIABLE, 0, UNASSIGNED_PLAYER_ID, true, false);
}*/
	
//----------------------------------------------------
