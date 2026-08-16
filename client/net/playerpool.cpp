//----------------------------------------------------------
//
// SA:MP Multiplayer Modification For GTA:SA
// Copyright 2004-2005 SA:MP team
//
// Version: $Id: playerpool.cpp,v 1.14 2006/05/07 15:38:36 kyeman Exp $
//
//----------------------------------------------------------

#include "../main.h"

/*char szQuitReasons[][32] = {
"Timeout",
"Leaving",
"Kicked"
};*/

static int iExceptPlayerMessageDisplayed=0;

//----------------------------------------------------

CPlayerPool::CPlayerPool()
{
	m_pLocalPlayer = new CLocalPlayer();
	m_byteLocalPlayerID = INVALID_PLAYER_ID;

	// loop through and initialize all net players to null and slot states to false
	for(BYTE bytePlayerID = 0; bytePlayerID < MAX_PLAYERS; bytePlayerID++) {
		m_bPlayerSlotState[bytePlayerID] = false;
		m_pPlayers[bytePlayerID] = NULL;
	}
}

//----------------------------------------------------

CPlayerPool::~CPlayerPool()
{
	delete m_pLocalPlayer;
	m_pLocalPlayer = NULL;

	for(BYTE bytePlayerID = 0; bytePlayerID < MAX_PLAYERS; bytePlayerID++) {
		Delete(bytePlayerID,0);
	}
}

//----------------------------------------------------

bool CPlayerPool::New(BYTE bytePlayerID, PCHAR szPlayerName, bool bIsNPC)
{
	m_pPlayers[bytePlayerID] = new CRemotePlayer();

	if(m_pPlayers[bytePlayerID])
	{
		m_pPlayers[bytePlayerID]->SetID(bytePlayerID);
		m_pPlayers[bytePlayerID]->SetName(szPlayerName);
		m_pPlayers[bytePlayerID]->m_bIsNPC = bIsNPC;
		m_bPlayerSlotState[bytePlayerID] = true;
		//if(pChatWindow) 
			//pChatWindow->AddInfoMessage("*** %s joined the server.",szPlayerName);
		return true;
	}
	else
	{
		return false;
	}
}

//----------------------------------------------------

bool CPlayerPool::Delete(BYTE bytePlayerID, BYTE byteReason)
{
	if(!GetSlotState(bytePlayerID) || !m_pPlayers[bytePlayerID]) {
		return false; // Player already deleted or not used.
	}

	if (m_pLocalPlayer && m_pLocalPlayer->IsSpectating() && m_pLocalPlayer->m_SpectateID == bytePlayerID) {
		m_pLocalPlayer->ToggleSpectating(false);
	}

	m_bPlayerSlotState[bytePlayerID] = false;
	delete m_pPlayers[bytePlayerID];
	m_pPlayers[bytePlayerID] = NULL;

	//if(pChatWindow) {
		//pChatWindow->AddInfoMessage("*** %s left the server. (%s)",
		//m_szPlayerNames[bytePlayerID],szQuitReasons[byteReason]);
	//}

	return true;
}

//----------------------------------------------------

bool CPlayerPool::Process()
{
	// Process all CRemotePlayers
	int localVW = 0;
	if (m_pLocalPlayer) localVW = m_pLocalPlayer->GetVirtualWorld();
	for(BYTE bytePlayerID = 0; bytePlayerID < MAX_PLAYERS; bytePlayerID++) {
		if(true == m_bPlayerSlotState[bytePlayerID]) {
			
			try {
				m_pPlayers[bytePlayerID]->Process(localVW);
			} catch(...) {
				if(!iExceptPlayerMessageDisplayed) {
					pChatWindow->AddDebugMessage("Warning: Error Processing Player(%u)",bytePlayerID);
					//Delete(bytePlayerID,0);
					iExceptPlayerMessageDisplayed++;
				}
			}

			/*if (m_byteVirtualWorld[bytePlayerID] != localVW || m_pPlayers[bytePlayerID]->GetState() == PLAYER_STATE_SPECTATING){
				m_pPlayers[bytePlayerID]->HideForLocal();
			}
			else {
				// Just shows the radar marker if required
				m_pPlayers[bytePlayerID]->ShowForLocal();
			}*/
		}
	}

	// Process the LocalPlayer
	try {
		m_pLocalPlayer->Process();
	} catch(...) {
		if(!iExceptPlayerMessageDisplayed) {
			pChatWindow->AddDebugMessage("Warning: Error Processing Player");
			iExceptPlayerMessageDisplayed++;
		}
	}
	
	return true;
}

//----------------------------------------------------

BYTE CPlayerPool::FindRemotePlayerIDFromGtaPtr(PED_TYPE * pActor)
{
	CPlayerPed *pPlayerPed;

	for(BYTE bytePlayerID = 0; bytePlayerID < MAX_PLAYERS; bytePlayerID++)
	{
		if(true == m_bPlayerSlotState[bytePlayerID])
		{
			pPlayerPed = m_pPlayers[bytePlayerID]->GetPlayerPed();

			if(pPlayerPed) {
				PED_TYPE *pTestActor = pPlayerPed->GetGtaActor();
				if((pTestActor != NULL) && (pActor == pTestActor)) // found it
					return (BYTE)m_pPlayers[bytePlayerID]->GetID();
			}
		}
	}

	return INVALID_PLAYER_ID;	
}

//----------------------------------------------------

BYTE CPlayerPool::GetCount()
{
	BYTE byteCount=0;
	for(BYTE bytePlayerID = 0; bytePlayerID < MAX_PLAYERS; bytePlayerID++) {
		if(true == m_bPlayerSlotState[bytePlayerID]) {
			byteCount++;
		}
	}
	return byteCount;
}

//----------------------------------------------------

int CPlayerPool::GetCount(bool bIncludeNPCs)
{
	int iCount, i;

	iCount = 0;

	if (bIncludeNPCs)
	{
		/*i = 0;
		do
		{
			if (i < MAX_PLAYERS && m_bPlayerSlotState[i] == TRUE)
				iResult++;
			if (i + 1 < MAX_PLAYERS && m_bPlayerSlotState[i + 1] == TRUE)
				iResult++;
			if (i + 2 < MAX_PLAYERS && m_bPlayerSlotState[i + 2] == TRUE)
				iResult++;
			if (i + 3 < MAX_PLAYERS && m_bPlayerSlotState[i + 3] == TRUE)
				iResult++;

			i += 4;
		} while (i < MAX_PLAYERS);*/
		
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (m_bPlayerSlotState[i] == TRUE)
			{
				iCount++;
			}
		}
	}
	else
	{
		for (i = 0; i < MAX_PLAYERS; i++)
		{
			if (m_bPlayerSlotState[i] == TRUE && !IsPlayerNPC(i))
			{
				iCount++;
			}
		}
	}
	return iCount;
}

//----------------------------------------------------

void CPlayerPool::DeactivateAll()
{
	m_pLocalPlayer->m_bIsActive = false;
	m_pLocalPlayer->m_iSelectedClass = 0;

	for(BYTE bytePlayerID = 0; bytePlayerID < MAX_PLAYERS; bytePlayerID++) {
		if(true == m_bPlayerSlotState[bytePlayerID]) {
			m_pPlayers[bytePlayerID]->Deactivate();
		}
	}
}

//----------------------------------------------------

void CPlayerPool::DeleteAll()
{
	for (WORD wPlayerID = 0; wPlayerID < MAX_PLAYERS; wPlayerID++) {
		if (m_bPlayerSlotState[wPlayerID])
			Delete(wPlayerID, 0);
	}
}

//----------------------------------------------------
// EOF