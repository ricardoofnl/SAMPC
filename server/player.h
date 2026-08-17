/*

	SA:MP Multiplayer Modification
	Copyright 2004-2005 SA:MP Team

    Version: $Id: player.h,v 1.30 2006/05/07 15:35:32 kyeman Exp $

*/

#ifndef SAMPSRV_PLAYER_H
#define SAMPSRV_PLAYER_H

//----------------------------------------------------

class CPlayer
{
private:
	BYTE					m_bytePlayerID;
	WORD					m_wPlayerID;
	char					m_szName[MAX_PLAYER_NAME];
	unsigned char			m_ucNameLength;

	BYTE					m_byteUpdateFromNetwork;

	// Information that is synced.
	ONFOOT_SYNC_DATA		m_ofSync;
	INCAR_SYNC_DATA			m_icSync;
	PASSENGER_SYNC_DATA		m_psSync;
	AIM_SYNC_DATA			m_aimSync;
	SPECTATOR_SYNC_DATA		m_spSync;
	TRAILER_SYNC_DATA		m_trSync;

	UINT					m_wLastKeys;

	bool					m_bHasAimUpdates;
	bool					m_bHasTrailerUpdates;
	//BYTE					m_byteSeatID;
	BYTE					m_byteState;

	VECTOR					m_vecCheckpoint;
	float					m_fCheckpointSize;
	bool					m_bInCheckpoint;

	VECTOR					m_vecRaceCheckpoint;
	VECTOR					m_vecRaceNextCheckpoint;
	BYTE					m_byteRaceCheckpointType;
	float					m_fRaceCheckpointSize;
	bool					m_bInRaceCheckpoint;

	bool m_bStreamedInLabel[MAX_LABEL_GLOBAL];
	WORD m_wStreamedLabelCount;

	bool m_bStreamedInPickup[MAX_PICKUPS];
	unsigned short m_usPickupLimitCount;

	bool m_bIsActorStreamedIn[MAX_ACTORS];
	int m_iStreamedActorCount;

	RakNet::Time32			m_ConnectedTime;
public:
	CVariables				*m_pVariables;
	CPlayerTextDrawPool		*m_pTextDraw;
	CPlayerLabelPool		*m_pLabelPool;
	PLAYER_SPAWN_INFO		m_SpawnInfo;
	bool					m_bHasSpawnInfo;
	BYTE					m_byteWantedLevel;

	VEHICLEID				m_VehicleID;
	DWORD					m_dwColor;
	bool					m_bCheckpointEnabled;
	bool					m_bRaceCheckpointEnabled;
	int						m_iInteriorId;
	int						m_iDrunkLevel;
	bool					m_bIsAdmin;
	int						m_iMoney;
	int						m_iScore;
	bool					m_bTyping;
	RakNet::Time			m_nLastPingUpdate;
	unsigned char			m_ucTeam;
	unsigned char			m_ucFightingStyle;
	unsigned char			m_ucFightingMove;
	bool					m_bIsNPC;
	char					m_szSerial[100];
	bool					m_bSelectingText;
	WORD					m_wDialogID; // INVALID_DIALOG_ID when nothing is open
	WORD					m_wTargetedPlayer;
	WORD					m_wTargetedActor;

	// Weapon data
	DWORD					m_dwSlotAmmo[13];
	BYTE					m_byteSlotWeapon[13];
	
	BYTE					m_byteTime; // Uses
	float					m_fGameTime; // Time in seconds (game minutes)

	BYTE					m_byteSpectateType;
	DWORD					m_SpectateID; // Vehicle or player id

	RakNet::Time			m_tmLastStreamRateTick;

	char m_szClientVersion[MAX_VERSION_NAME];
	unsigned int m_uiRconAttempt;
	unsigned int m_uiMsgRecv;

	int m_iVirtualWorld;

	void SetName(const char* szName, unsigned char ucLenght);
	const char* GetName() const { return m_szName; }
	unsigned char GetNameLength() const { return m_ucNameLength; }

	ONFOOT_SYNC_DATA* GetOnFootSyncData() { return &m_ofSync; }
	INCAR_SYNC_DATA* GetInCarSyncData() { return &m_icSync; }
	PASSENGER_SYNC_DATA* GetPassengerSyncData() { return &m_psSync; }
	AIM_SYNC_DATA* GetAimSyncData() { return &m_aimSync; }
	SPECTATOR_SYNC_DATA* GetSpectatorSyncData() { return &m_spSync; }

	CPlayerLabelPool* GetLabelPool() { return m_pLabelPool; };

	void SetState(BYTE byteState);
	BYTE GetState() { return m_byteState; };

	CPlayer();
	~CPlayer();

	float	m_fHealth;
	float	m_fArmour;
	VECTOR  m_vecPos;
	VECTOR	m_vecMoveSpeed;
	float	m_fRotation;
	bool	m_bCanTeleport;
	float m_fWorldBounds[4];
	WORD m_wSkillLevel[11];

	bool IsActive() { 
		if( m_byteState != PLAYER_STATE_NONE && m_byteState != PLAYER_STATE_SPECTATING ) { return true; }
		return false;
	};
	
	void Deactivate();

	bool m_bPlayerStreamedIn[1000];
	int m_iStreamedInPlayerCount;

	void StreamPlayerIn(WORD wPlayerID);
	void StreamPlayerOut(WORD wPlayerID);

	bool IsPickupStreamedIn(int iPickupID);
	void StreamPickupIn(int iPickupID);
	void StreamPickupOut(int iPickupID);

	bool IsLabelStreamedIn(WORD wLabelID);
	void StreamLabelIn(WORD wLabelID);
	void StreamLabelOut(WORD wLabelID);

	bool IsActorStreamedIn(int iActorID);
	void StreamActorIn(int iActorID);
	void StreamActorOut(int iActorID);

	void UpdatePosition(float x, float y, float z);
	void ProcessMarkers();
	void ProcessStreaming();

	// Process this player during the server loop.
	void Process(float fElapsedTime);
	void BroadcastSyncData();
	void Say(unsigned char * szText, size_t byteTextLength);
	void SetID(BYTE bytePlayerID)
	{
		m_bytePlayerID = bytePlayerID;
		m_wPlayerID = bytePlayerID;
		if (m_pLabelPool)
			m_pLabelPool->SetPlayerID(m_bytePlayerID);
	};
	
	void StoreOnFootFullSyncData(ONFOOT_SYNC_DATA * pofSync);
	void StoreInCarFullSyncData(INCAR_SYNC_DATA * picSync);
	void StorePassengerFullSyncData(PASSENGER_SYNC_DATA *ppsSync);
	void StoreSpectatorFullSyncData(SPECTATOR_SYNC_DATA *pspSync);
	void StoreAimSyncData(AIM_SYNC_DATA *paimSync);
	void StoreTrailerFullSyncData(TRAILER_SYNC_DATA* trSync);
	void SetSpawnInfo(PLAYER_SPAWN_INFO *pSpawn);

	PLAYER_SPAWN_INFO * GetSpawnInfo() { return &m_SpawnInfo; };

	void HandleDeath(BYTE byteReason, BYTE byteWhoWasResponsible);
	void Spawn();
	void SpawnForWorld(BYTE byteTeam, int iSkin, VECTOR * vecPos, float fRotation);
	void SpawnForPlayer(BYTE byteForPlayerID);

	void EnterVehicle(VEHICLEID VehicleID,BYTE bytePassenger);
	void ExitVehicle(VEHICLEID VehicleID);

	float GetDistanceFromPoint(float fX, float fY, float fZ);
	float GetSquaredDistanceFrom3DPoint(float fX, float fY, float fZ);
	float GetSquaredDistanceFrom2DPoint(float fX, float fY);

	void SetPlayerColor(DWORD dwColor);
	DWORD GetPlayerColor() { return m_dwColor; };

	void SetCheckpoint(float fX, float fY, float fZ, float fSize);
	void ToggleCheckpoint(bool bEnabled);
	void SetRaceCheckpoint(int iType, float fX, float fY, float fZ, float fNX, float fNY, float fNZ, float fSize);
	void ToggleRaceCheckpoint(bool bEnabled);

	bool IsInCheckpoint() { return m_bInCheckpoint; };
	bool IsInRaceCheckpoint() { return m_bInRaceCheckpoint; };
	BYTE GetTeam() { return m_ucTeam; };
	void SetTeam(unsigned char ucTeam);
	unsigned char GetCurrentWeapon() {
		if (m_byteState == PLAYER_STATE_PASSENGER)
			return m_psSync.byteCurrentWeapon;
		else if (m_byteState == PLAYER_STATE_DRIVER)
			return m_icSync.byteCurrentWeapon;
		// Return onfoot weapon in any other state
		return m_ofSync.byteCurrentWeapon;
	};
	
	int GetWeaponSlot(int iWeaponID);
	//WEAPON_SLOT_TYPE* GetWeaponSlotsData();
	void SetWeaponSlot(BYTE byteSlot, DWORD dwWeapon, DWORD dwAmmo);
	
	DWORD GetSlotWeapon(BYTE bSlot) { return m_byteSlotWeapon[bSlot]; };
	DWORD GetSlotAmmo(BYTE bSlot) { return m_dwSlotAmmo[bSlot]; };
	void SetCurrentWeaponAmmo(DWORD dwAmmo);
	void SetWantedLevel(BYTE byteLevel) { m_byteWantedLevel = byteLevel; };
	BYTE GetWantedLevel() { return m_byteWantedLevel; };
	
	void SetTime(BYTE byteHour, BYTE byteMinute);
	void SetClock(BYTE byteClock);

	BYTE CheckWeapon(BYTE weapon);
	void CheckKeyUpdatesForScript(WORD wKeys);

	BYTE GetSpecialAction() {
		if(GetState() == PLAYER_STATE_ONFOOT) return m_ofSync.byteSpecialAction;
		return SPECIAL_ACTION_NONE;
	};

	void SetVirtualWorld(int iVirtualWorld);
	int GetVirtualWorld() const {
		return m_iVirtualWorld;
	}

	void SetSkillLevel(int iSkill, int iLevel);

	unsigned long GetCurrentWeaponAmmo();

	void UpdateTimer();

	bool SendClientCheck(BYTE byteType, DWORD dwAddress, WORD wOffset, WORD wCount);
};

#endif

//----------------------------------------------------
// EOF

