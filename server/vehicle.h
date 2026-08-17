/*

	SA:MP Multiplayer Modification
	Copyright 2004-2005 SA:MP Team

	file:
		vehicle.h
	desc:
		Vehicle handling header file.

    Version: $Id: vehicle.h,v 1.3 2006/04/12 19:26:45 mike Exp $

*/

#ifndef SAMPSRV_VEHICLE_H
#define SAMPSRV_VEHICLE_H

#define RANDOM_VEHICLE_COLOR (rand() % (256))

typedef struct _VEHICLE_SPAWN_INFO
{
	int iVehicleType;
	VECTOR vecPos;
	float fRotation;
	int iColor1;
	int iColor2;
	int iRespawnDelay;
	int iInterior;
} VEHICLE_SPAWN_INFO;

//----------------------------------------------------

class CVehicle
{
public:

	VEHICLEID				m_VehicleID;
	VEHICLEID				m_TrailerID;
	VEHICLEID				m_CabID;
	BYTE					m_byteDriverID;
	BYTE					m_bytePassengers[7];
	bool					m_bIsActive;
	bool					m_bIsWasted;
	VEHICLE_SPAWN_INFO		m_SpawnInfo;
	MATRIX4X4				m_matWorld;
	VECTOR					m_vecMoveSpeed;
	VECTOR					m_vecTurnSpeed;
	float					m_fHealth;
	bool					m_bDead;
	unsigned char			m_ucKillerID;
	_CAR_MOD_INFO			m_CarModInfo;
	CHAR					m_szNumberPlate[9];
	bool					m_bDeathHasBeenNotified;
	bool					m_bHasBeenOccupied;
	DWORD					m_dwLastSeenOccupiedTick;
	DWORD					m_dwLastRespawnedTick;
	int						m_iVirtualWorld;
	bool					m_bHasSiren;
	VEHICLE_OPEN_CLOSE_FLAG m_Windows;
	VEHICLE_OPEN_CLOSE_FLAG m_Doors;
	VEHICLE_PARAMS m_Params;

	int m_iPanelDamageStatus;
	int m_iDoorDamageStatus;
	unsigned char m_ucLightDamageStatus;
	unsigned char m_ucTireDamageStatus;

	bool m_bOnItsSide;
	bool m_bUpsideDown;
	bool m_bSirenOn;
	bool m_bWrecked;
	bool m_bSunked;

	bool bOldSirenState;

	void Process(float fElapsedTime);

	CVehicle(int iModel,VECTOR *vecPos,float fRotation,int iColor1,int iColor2, int iRespawnTime, bool bAddSiren);
	//~CVehicle(){};

	bool IsActive() { return m_bIsActive; };
	bool IsWasted() { return m_bIsWasted; };

	void SetID(VEHICLEID VehicleID) { m_VehicleID = VehicleID; };
	void SetCab(VEHICLEID VehicleID) { m_CabID = VehicleID; };
	VEHICLE_SPAWN_INFO * GetSpawnInfo() { return &m_SpawnInfo; };

	void SpawnForPlayer(BYTE byteForPlayerID);
	void SendParams(WORD wPlayerID = INVALID_PLAYER_ID);
	void SetVehicleInterior(int iIntSet) { m_SpawnInfo.iInterior = iIntSet; };
	void SetDead() { m_bDead = true; }; // Respawns due to death in ~10s
	void SetHealth(float fHealth);
	void SetNumberPlate(PCHAR Plate);
	void CheckForIdleRespawn();
	void Respawn();
	bool IsOccupied();
	bool IsATrainPart();
	bool HasParamsSet();
	bool HandleSiren(unsigned char ucPlayerId, bool bSirenState);
	void SetColor(WORD wPlayerID, int iColor1, int iColor2);
	void SetPaintjob(WORD wPlayerID, int iPaintjob);

	void Update(BYTE bytePlayerID, MATRIX4X4 * matWorld, float fHealth, VEHICLEID TrailerID);
	void UpdateDamage(PLAYERID PlayerID, int iPanels, int iDoors, unsigned char ucLights, unsigned char ucTires);

	float GetSquaredDistanceFrom2DPoint(float fX, float fY);
	float GetDistanceFromPoint(float fX, float fY, float fZ);

	void SetVirtualWorld(int iVirtualWorld);
};

#endif
