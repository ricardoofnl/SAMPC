
#include "../main.h"

CLabelPool::CLabelPool()
{
	for (WORD wLabelID = 0; wLabelID < MAX_LABELS; wLabelID++)
	{
		SecureZeroMemory(&m_Labels[wLabelID], sizeof(LABEL));
		m_bSlotState[wLabelID] = false;
	}
}

CLabelPool::~CLabelPool()
{
	for (WORD wLabelID = 0; wLabelID < MAX_LABELS; wLabelID++)
	{
		if (m_bSlotState[wLabelID])
		{
			Delete(wLabelID);
		}
	}
}

void CLabelPool::New(WORD wLabelID, char* szText, DWORD dwColor, VECTOR vecPos, float fDrawDistance,
	bool bTestLOS, WORD wAttachedPlayerID, WORD wAttachedVehicleID)
{
	if (wLabelID < MAX_LABELS)
	{
		if (m_bSlotState[wLabelID])
		{
			if (m_Labels[wLabelID].szText)
				free(m_Labels[wLabelID].szText);

			SecureZeroMemory(&m_Labels[wLabelID], sizeof(LABEL));
			m_bSlotState[wLabelID] = false;
		}

		size_t len = strlen(szText);
		m_Labels[wLabelID].szText = (char*)calloc(1, len + 1);

		if (m_Labels[wLabelID].szText)
		{
			strcpy_s(m_Labels[wLabelID].szText, len + 1, szText);

			//if (strlen(m_Labels[wLabelID].szText) < 400)
				//FormatGameTextKey(m_Labels[wLabelID].szText, 399);
			FormatGameKeysInString(m_Labels[wLabelID].szText);

			m_Labels[wLabelID].dwColor = (dwColor >> 8) | (dwColor << 24);
			m_Labels[wLabelID].fX = vecPos.X;
			m_Labels[wLabelID].fY = vecPos.Y;
			m_Labels[wLabelID].fZ = vecPos.Z;
			m_Labels[wLabelID].fDrawDistance = fDrawDistance;
			m_Labels[wLabelID].bTestLOS = bTestLOS;
			m_Labels[wLabelID].wAttachedPlayerID = wAttachedPlayerID;
			m_Labels[wLabelID].wAttachedVehicleID = wAttachedVehicleID;

			m_bSlotState[wLabelID] = true;
		}
	}
}

bool CLabelPool::Delete(WORD wLabelID)
{
	if (wLabelID < MAX_LABELS && m_bSlotState[wLabelID])
	{
		if (m_Labels[wLabelID].szText)
			free(m_Labels[wLabelID].szText);

		SecureZeroMemory(&m_Labels[wLabelID], sizeof(LABEL));

		m_bSlotState[wLabelID] = false;

		return true;
	}
	return false;
}

void CLabelPool::Draw()
{
	CPlayerPool* pPlayerPool;
	CRemotePlayer* pRemotePlayer;
	//CPlayerPed* pPlayerPed;
	CVehiclePool* pVehiclePool;
	CVehicle* pVehicle;
	D3DXVECTOR3 vecDrawPos;
	VECTOR vecPos, vecResultPos;
	MATRIX4X4 mat;
	bool bShadowed;

	if (!pNetGame || !pLabel || !pGame->FindPlayerPed())
		return;

	pPlayerPool = pNetGame->GetPlayerPool();
	pVehiclePool = pNetGame->GetVehiclePool();

	pLabel->Begin();

	for (short i = 0; i < MAX_LABELS; i++)
	{
		if (!m_bSlotState[i]) continue;
		if (!m_Labels[i].szText || m_Labels[i].szText[0] == '\0') continue;

		bShadowed = (m_Labels[i].dwColor & 0xFF000000) == 0xFF000000;

		if (m_Labels[i].wAttachedPlayerID != INVALID_PLAYER_ID)
		{
			pRemotePlayer = pPlayerPool->GetAt(m_Labels[i].wAttachedPlayerID);

			if (pRemotePlayer && pRemotePlayer->IsActive() && pRemotePlayer->GetPlayerPed() &&
				pRemotePlayer->GetPlayerPed()->GetDistanceFromLocalPlayerPed() <= m_Labels[i].fDrawDistance)
			{
				pRemotePlayer->GetPlayerPed()->GetBonePosition(8, &vecPos);

				vecDrawPos.x = vecPos.X + m_Labels[i].fX;
				vecDrawPos.y = vecPos.Y + m_Labels[i].fY;
				vecDrawPos.z = vecPos.Z + m_Labels[i].fZ;

				vecDrawPos.z += pRemotePlayer->GetPlayerPed()->GetDistanceFromCamera() * 0.05f;

				pLabel->Draw(&vecDrawPos, m_Labels[i].szText, m_Labels[i].dwColor, bShadowed, m_Labels[i].bTestLOS);
			}
		}
		else if (m_Labels[i].wAttachedVehicleID != INVALID_VEHICLE_ID)
		{
			pVehicle = pVehiclePool->GetAt(m_Labels[i].wAttachedVehicleID);

			if (pVehicle && pVehicle->GetDistanceFromLocalPlayerPed() <= m_Labels[i].fDrawDistance)
			{
				pVehicle->GetMatrix(&mat);

				vecPos.X = m_Labels[i].fX;
				vecPos.Y = m_Labels[i].fY;
				vecPos.Z = m_Labels[i].fZ;

				Transform(&vecResultPos, &mat, &vecPos);

				vecDrawPos.x = vecResultPos.X;
				vecDrawPos.y = vecResultPos.Y;
				vecDrawPos.z = vecResultPos.Z;

				pLabel->Draw(&vecDrawPos, m_Labels[i].szText, m_Labels[i].dwColor, bShadowed, m_Labels[i].bTestLOS);
			}
		}
		else
		{
			if (pGame->FindPlayerPed()->GetDistanceFromPoint(
				m_Labels[i].fX, m_Labels[i].fY, m_Labels[i].fZ) <= m_Labels[i].fDrawDistance)
			{
				vecDrawPos.x = m_Labels[i].fX;
				vecDrawPos.y = m_Labels[i].fY;
				vecDrawPos.z = m_Labels[i].fZ;

				pLabel->Draw(&vecDrawPos, m_Labels[i].szText, m_Labels[i].dwColor, bShadowed, m_Labels[i].bTestLOS);
			}
		}
	}

	pLabel->End();
}
