
#include "main.h"

//----------------------------------------------------

CLabelPool::CLabelPool()
{
	for (WORD wLabelID = 0; wLabelID < MAX_LABEL_GLOBAL; wLabelID++)
	{
		memset(&m_Labels[wLabelID], 0, sizeof(LABEL));
		m_bSlotState[wLabelID] = NULL;
	}
}

//----------------------------------------------------

CLabelPool::~CLabelPool()
{
	for (WORD wLabelID = 0; wLabelID < MAX_LABEL_GLOBAL; wLabelID++)
	{
		Delete(wLabelID);
	}
}

//----------------------------------------------------

WORD CLabelPool::New(char* szText, DWORD dwColor, float fX, float fY, float fZ,
	float fDistance, int iVirtualWorld, bool bTestLOS)
{
	WORD wLabelID = 0;
	while (wLabelID < MAX_LABEL_GLOBAL)
	{
		if (!m_bSlotState[wLabelID]) break;
		wLabelID++;
	}
	if (wLabelID == MAX_LABEL_GLOBAL) return INVALID_LABEL_ID;

	LABEL* pLabel = &m_Labels[wLabelID];

	UINT uiLen = strlen(szText);
	pLabel->wLength = (uiLen > MAX_LABEL_TEXT) ? MAX_LABEL_TEXT : (WORD)uiLen;
	pLabel->szText = (char*)calloc(1, uiLen + 1);

	if (pLabel->szText != NULL)
	{
		//strcpy(pLabel->szText, szText);
		strncpy(pLabel->szText, szText, uiLen);

		pLabel->dwColor = dwColor;
		pLabel->fX = fX;
		pLabel->fY = fY;
		pLabel->fZ = fZ;
		pLabel->fDrawDistance = fDistance;
		pLabel->iVirtualWorld = iVirtualWorld;
		pLabel->bTestLOS = bTestLOS;
		pLabel->wAttachedPlayerID = INVALID_PLAYER_ID;
		pLabel->wAttachedVehicleID = INVALID_VEHICLE_ID;

		m_bSlotState[wLabelID] = true;

		CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
		CPlayer* pPlayer;
		if (pPlayerPool)
		{
			for (int x = 0; x = pPlayerPool->GetLastPlayerId(); x++)
			{
				if (pPlayerPool->GetSlotState(x))
				{
					pPlayer = pPlayerPool->GetAt(x);

					if (pPlayer &&
						pPlayer->GetState() != PLAYER_STATE_NONE &&
						pPlayer->GetState() != PLAYER_STATE_SPECTATING &&
						pPlayer->GetDistanceFromPoint(fX, fY, fZ) <= fDistance)
					{
						pPlayer->StreamLabelIn(wLabelID);
					}
				}
			}
		}

		return wLabelID;
	}
	return INVALID_LABEL_ID;
}

//----------------------------------------------------

bool CLabelPool::Delete(WORD wLabelID)
{
	if (wLabelID < MAX_LABEL_GLOBAL && m_bSlotState[wLabelID])
	{
		LABEL* pLabel = &m_Labels[wLabelID];

		if (pLabel->szText)
			free(pLabel->szText);

		DeleteForAll(wLabelID);

		memset(pLabel, 0, sizeof(LABEL));
		m_bSlotState[wLabelID] = false;

		return true;
	}
	return false;
}

//----------------------------------------------------

bool CLabelPool::UpdateText(WORD wLabelID, DWORD dwColor, char* szText)
{
	if (wLabelID < MAX_LABEL_GLOBAL && m_bSlotState[wLabelID])
	{
		DWORD dwTempLen = strlen(szText);
		dwTempLen = (dwTempLen > MAX_LABEL_TEXT) ? MAX_LABEL_TEXT : dwTempLen;
		PCHAR szTempText = (PCHAR)calloc(1, dwTempLen + 1);

		if (szTempText)
		{
			LABEL* pLabel = &m_Labels[wLabelID];

			if (pLabel->szText)
				free(pLabel->szText);

			//strcpy(szTempText, szText);
			strncpy(szTempText, szText, dwTempLen);

			pLabel->szText = szTempText;
			pLabel->wLength = (WORD)dwTempLen;
			pLabel->dwColor = dwColor;

			UpdateForAll(wLabelID);
		}
	}
	return false;
}

//----------------------------------------------------

bool CLabelPool::AttachToPlayer(WORD wLabelID, WORD wPlayerID,
	float fOffsetX, float fOffsetY, float fOffsetZ)
{
	if (wLabelID < MAX_LABEL_GLOBAL &&
		pNetGame->GetPlayerPool() &&
		pNetGame->GetPlayerPool()->GetSlotState(wLabelID))
	{
		LABEL* pLabel = &m_Labels[wLabelID];

		pLabel->wAttachedPlayerID = wPlayerID;
		pLabel->fX = fOffsetX;
		pLabel->fY = fOffsetY;
		pLabel->fZ = fOffsetZ;

		UpdateForAll(wLabelID);

		return true;
	}
	return false;
}

//----------------------------------------------------

bool CLabelPool::AttachToVehicle(WORD wLabelID, WORD wVehicleID,
	float fOffsetX, float fOffsetY, float fOffsetZ)
{
	if (wLabelID < MAX_LABEL_GLOBAL &&
		pNetGame->GetVehiclePool() &&
		pNetGame->GetVehiclePool()->GetSlotState(wLabelID))
	{
		LABEL* pLabel = &m_Labels[wLabelID];

		pLabel->wAttachedVehicleID = wVehicleID;
		pLabel->fX = fOffsetX;
		pLabel->fY = fOffsetY;
		pLabel->fZ = fOffsetZ;

		UpdateForAll(wLabelID);

		return true;
	}
	return false;
}

//----------------------------------------------------

void CLabelPool::UpdateForAll(WORD wLabelID)
{
	if (pNetGame->GetPlayerPool())
	{
		int x = 0;
		while (x != MAX_PLAYERS)
		{
			if (pNetGame->GetPlayerPool()->GetSlotState(x))
			{
				CPlayer* pPlayer = pNetGame->GetPlayerPool()->GetAt(x);

				if (pPlayer && pPlayer->IsLabelStreamedIn(wLabelID))
				{
					pPlayer->StreamLabelOut(wLabelID);
					pPlayer->StreamLabelIn(wLabelID);
				}
			}
			x++;
		}
	}
}

//----------------------------------------------------

void CLabelPool::DeleteForAll(WORD wLabelID)
{
	if (pNetGame->GetPlayerPool())
	{
		int x = 0;
		while (x != MAX_PLAYERS)
		{
			if (pNetGame->GetPlayerPool()->GetSlotState(x))
			{
				CPlayer* pPlayer = pNetGame->GetPlayerPool()->GetAt(x);

				if (pPlayer && pPlayer->IsLabelStreamedIn(wLabelID))
				{
					pPlayer->StreamLabelOut(wLabelID);
				}
			}
			x++;
		}
	}
}

//----------------------------------------------------

void CLabelPool::InitForPlayer(WORD wLabelID, WORD wPlayerID)
{
	if (m_bSlotState[wLabelID])
	{
		RakNet::BitStream bsSend;

		LABEL* pLabel = &m_Labels[wLabelID];

		bsSend.Write(wLabelID);
		bsSend.Write(pLabel->dwColor);
		bsSend.Write(pLabel->fX);
		bsSend.Write(pLabel->fY);
		bsSend.Write(pLabel->fZ);
		bsSend.Write(pLabel->fDrawDistance);
		// a bool goes on the wire as a single bit, the client reads a whole byte here
		bsSend.Write((BYTE)pLabel->bTestLOS);
		bsSend.Write(pLabel->wAttachedPlayerID);
		bsSend.Write(pLabel->wAttachedVehicleID);
		
		stringCompressor->EncodeString(pLabel->szText, pLabel->wLength + 1, &bsSend);

		pNetGame->SendToPlayer(wPlayerID, RPC_CreateLabel, &bsSend);
	}
}

//----------------------------------------------------

void CLabelPool::DeleteForPlayer(WORD wLabelID, WORD wPlayerID)
{
	if (m_bSlotState[wLabelID])
	{
		RakNet::BitStream bsSend;

		bsSend.Write(wLabelID);

		pNetGame->SendToPlayer(wPlayerID, RPC_DestroyLabel, &bsSend);
	}
}

//----------------------------------------------------
