//
// Version: $Id: scoreboard.cpp,v 1.15 2006/04/18 11:58:57 kyeman Exp $
//

#include "main.h"

VOID CALLBACK OnScoreBoardEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext);

CScoreBoard::CScoreBoard(IDirect3DDevice9* pDevice)
{
	m_pDevice = pDevice;
	m_pDialog = NULL;
	m_pListBox = NULL;
	m_dwSortType = 0;
	m_iLastPlayerCount = 0;
	if (pGame && pGame->GetScreenWidth() > 800)
	{
		m_fWidth = 800.0f;
		m_fHeight = 600.0f;
	}
	else
	{
		m_fWidth = 640.0f;
		m_fHeight = 480.0f;
	}
	m_fHeaderHeight = 60.0f;
	m_fNameOffset = 0.085f;
	m_fPingOffset = 0.265625f;
	m_fScoreOffset = 0.4375f;
	CalcClientSize();
	m_bVisible = false;
	ResetDialogControls();
}

void CScoreBoard::CalcClientSize()
{
	RECT rect;
	GetClientRect(pGame->GetMainWindowHwnd(), &rect);
	m_fScalar = 1.0f;
	m_fScreenOffsetX = rect.right * 0.5f - m_fWidth * 0.5f;
	m_fScreenOffsetY = rect.bottom * 0.5f - m_fHeight * 0.5f;
}

// GET REKT!!!!
void CScoreBoard::GetRect(RECT* rect)
{
	rect->left = (LONG)m_fScreenOffsetX;
	rect->right = (LONG)(m_fScreenOffsetX + m_fWidth);
	rect->top = (LONG)m_fScreenOffsetY;
	rect->bottom = (LONG)(m_fScreenOffsetY + m_fHeight);
}

void CScoreBoard::ResetDialogControls()
{
	SAFE_DELETE(m_pDialog);

	m_pDialog = new CDXUTDialog();

	if (m_pDialog)
	{
		m_pDialog->Init(pDialogResourceManager);
		m_pDialog->SetCallback(OnScoreBoardEvent);
		m_pDialog->SetSize((int)m_fWidth, (int)m_fHeight);
		m_pDialog->SetLocation(0, 0);
		// the rounded panel is drawn in Draw(), so the square quad stays off
		m_pDialog->SetBackgroundColors(D3DCOLOR_ARGB(0, 0, 0, 0));
		m_pDialog->EnableMouseInput(true);
		m_pDialog->EnableKeyboardInput(true);
		m_pDialog->SetVisible(false);

		m_pListBox = new CDXUTListBox(m_pDialog);

		m_pDialog->AddControl(m_pListBox);
		m_pListBox->SetLocation(0, (int)m_fHeaderHeight);
		m_pListBox->SetSize((int)m_fWidth, (int)(m_fHeight - m_fHeaderHeight));
		m_pListBox->OnInit();
		// same palette the dialog uses, 0 is the surface and 1 the selection bar
		TintElement(m_pListBox->GetElement(0),
			DLG_COL_PANEL, DLG_COL_PANEL, DLG_COL_PANEL, DLG_COL_TEXT);
		TintElement(m_pListBox->GetElement(1),
			DLG_COL_ACCENT, DLG_COL_ACCENT, DLG_COL_ACCENT, DLG_COL_TEXT);
		TintElement(m_pDialog->GetCaptionElement(),
			DLG_COL_HEADER, DLG_COL_HEADER, DLG_COL_HEADER, DLG_COL_TEXT);
		m_pListBox->m_nColumns = 3;
		m_pListBox->m_nColumnWidth[0] = (int)(m_fNameOffset * m_fWidth);
		m_pListBox->m_nColumnWidth[1] = (int)(m_fScoreOffset * m_fWidth);
		m_pListBox->m_nColumnWidth[2] = (int)(m_fPingOffset * m_fWidth);
		m_pListBox->SetEnabled(false);
		m_pListBox->SetVisible(false);

		CalcClientSize();
	}
}

void CScoreBoard::ProcessClickEvent()
{
	if (m_bVisible && m_pDialog)
	{
		DXUTListBoxItem* pItem;

		pItem = m_pListBox->GetItem(m_pListBox->GetSelectedIndex());
		if (pItem)
		{
			RakNet::BitStream bsSend;

			bsSend.Write((WORD)atol(pItem->strText));
			bsSend.Write((BYTE)0);

			pNetGame->Send(RPC_Click, &bsSend);
		}
		
		//Hide(true);
		m_pDialog->SetVisible(false);
		m_pListBox->SetEnabled(false);
		m_pListBox->SetVisible(false);
		pGame->ToggleKeyInputsDisabled(0);
		m_bVisible = false;
	}
}

typedef struct _PLAYER_SCORE_INFO
{
	DWORD dwId;
	char  szName[MAX_PLAYER_NAME+1]; // a name of exactly MAX_PLAYER_NAME still needs the terminator
	int   iScore;
	DWORD dwPing;
	DWORD dwColor;
	//int   iState;
} PLAYER_SCORE_INFO;

void SwapPlayerInfo(PLAYER_SCORE_INFO* psi1, PLAYER_SCORE_INFO* psi2)
{
	PLAYER_SCORE_INFO plrinf;
	memcpy(&plrinf, psi1, sizeof(PLAYER_SCORE_INFO));
	memcpy(psi1, psi2, sizeof(PLAYER_SCORE_INFO));
	memcpy(psi2, &plrinf, sizeof(PLAYER_SCORE_INFO));
}

void CScoreBoard::UpdateList()
{
	if (!pNetGame || !pNetGame->GetPlayerPool() || !m_bVisible || !m_pDialog)
		return;

	int iLastIndexSaved = m_pListBox->GetSelectedIndex();
	int iLastBarPosSaved = m_pListBox->GetScrollBarPosition();

	m_pListBox->RemoveAllItems();

	CPlayerPool* pPlayerPool = pNetGame->GetPlayerPool();
	CLocalPlayer* pLocalPlayer = pPlayerPool->GetLocalPlayer();
	int playercount = pPlayerPool->GetCount(false) + 1;

	PLAYER_SCORE_INFO* Players;
	Players = (PLAYER_SCORE_INFO*)calloc(playercount, sizeof(PLAYER_SCORE_INFO));
	if (!Players)
		return;

	strcpy_s(Players[0].szName, pLocalPlayer->GetName());
	Players[0].dwColor = pLocalPlayer->GetPlayerColorAsARGB();
	Players[0].iScore = pLocalPlayer->m_iScore;
	Players[0].dwPing = pLocalPlayer->m_usPing;
	Players[0].dwId = pPlayerPool->GetLocalPlayerID();
	
	int i = 1;
	int x;
	for (x=0; x<MAX_PLAYERS; x++)
	{
		if (pPlayerPool->GetSlotState(x) == TRUE &&
			x != pPlayerPool->GetLocalPlayerID() &&
			!pPlayerPool->IsPlayerNPC(x))
		{
			CRemotePlayer* pRemotePlayer = pPlayerPool->GetAt(x);

			Players[i].dwPing = pRemotePlayer->m_usPing;
			strcpy_s(Players[i].szName, pRemotePlayer->GetName());
			Players[i].iScore = pRemotePlayer->m_iScore;
			Players[i].dwColor = pRemotePlayer->GetPlayerColorAsARGB();
			Players[i].dwId = x;
			
			i++;
		}
	}

	// bubble sorts, same shape and direction as SA-MP's
	if (m_dwSortType == 1 && (playercount - 1) > 0)
	{
		// by name, ascending
		for (int iPass = playercount - 1; iPass > 0; iPass--)
		{
			for (int iAt = 0; iAt < iPass; iAt++)
			{
				if (strcmp(Players[iAt+1].szName, Players[iAt].szName) < 0)
					SwapPlayerInfo(&Players[iAt], &Players[iAt+1]);
			}
		}
	}
	else if (m_dwSortType == 2 && (playercount - 1) > 0)
	{
		// by score, descending
		for (int iPass = playercount - 1; iPass > 0; iPass--)
		{
			for (int iAt = 0; iAt < iPass; iAt++)
			{
				if (Players[iAt+1].iScore > Players[iAt].iScore)
					SwapPlayerInfo(&Players[iAt], &Players[iAt+1]);
			}
		}
	}


	char szBuffer[11]; //[260];
	for (x = 0; x < playercount; x++)
	{
		sprintf_s(szBuffer, "%u", Players[x].dwId);
		m_pListBox->AddItem(szBuffer, Players[x].dwId, Players[x].dwColor);

		// otherwise CDXUTListBox::Render highlights every row, see CDialog::SetupList
		DXUTListBoxItem* pRow = m_pListBox->GetItem(x);
		if (pRow) pRow->bForceUnselected = true;

		if (m_pListBox->GetItem(x) && Players[x].szName[0] != '\0')
		{
			m_pListBox->AddItemToColumn(x, 0, Players[x].szName);

			sprintf_s(szBuffer, "%d", Players[x].iScore);
			m_pListBox->AddItemToColumn(x, 1, szBuffer);

			sprintf_s(szBuffer, "%u", Players[x].dwPing);
			m_pListBox->AddItemToColumn(x, 2, szBuffer);
		}
	}

	if (iLastIndexSaved < 0)
		m_pListBox->SelectItem(-1);
	else
		m_pListBox->SelectItem(iLastIndexSaved);

	free(Players);

	m_pListBox->SetScrollBarTrackPosition(iLastBarPosSaved);
}

void CScoreBoard::Show()
{
	if (!m_bVisible && m_pDialog)
	{
		m_pDialog->SetVisible(true);
		m_pListBox->SetEnabled(true);
		m_pListBox->UpdateRects();
		m_pListBox->SetVisible(true);
		
		UpdateList();

		pGame->ToggleKeyInputsDisabled(3);
		
		m_bVisible = true;
	}
}

void CScoreBoard::Hide(bool bDisableControls)
{
	if (m_bVisible && m_pDialog)
	{
		m_pDialog->SetVisible(false);
		m_pListBox->SetEnabled(false);
		m_pListBox->SetVisible(false);
		
		if(bDisableControls)
			pGame->ToggleKeyInputsDisabled(0);
		
		m_bVisible = false;
	}

}

void CScoreBoard::Draw()
{
	char szPlayers[64];
	
	if (!m_bVisible)
		return;

	int playercount = 0;
	if (pNetGame)
	{
		playercount = pNetGame->GetPlayerPool()->GetCount(false) + 1;
		if (playercount != m_iLastPlayerCount)
		{
			m_iLastPlayerCount = playercount;
			UpdateList();
		}
		pNetGame->UpdatePlayerPings();
	}

	if (m_pDialog)
	{
		int iWidth = pGame->GetScreenWidth();
		int iHeight = pGame->GetScreenHeight();
		if (iWidth > 0 && iHeight > 0)
		{
			m_pDialog->SetLocation(
				iWidth / 2 - (int)m_fWidth / 2,
				iHeight / 2 - (int)m_fHeight / 2);
		}

		// same rounded surface the dialogs use, the header strip is the band the
		// hostname and column titles are drawn over
		RECT panel;
		GetRect(&panel);
		DrawRoundedPanel(m_pDevice, &panel, (int)m_fHeaderHeight,
			DLG_COL_PANEL, DLG_COL_HEADER);

		m_pDialog->OnRender(10.0f);
	}


	sprintf_s(szPlayers, "Players: %d", playercount);

	RECT rect;
	rect.left = (LONG)(m_fScreenOffsetX + 5.0f);
	rect.top = (LONG)(m_fScreenOffsetY + 5.0f);
	rect.right = (LONG)(m_fScreenOffsetX + m_fWidth);
	rect.bottom = (LONG)(m_fHeaderHeight + m_fScreenOffsetY);
	if(pNetGame && pNetGame->m_szHostName[0] != '\0')
		pDefaultFont->RenderSmallerText(NULL, pNetGame->m_szHostName, rect, 288, 0xFFBEBEBE, true);

	SIZE size;
	pDefaultFont->MeasureSmallerText(&size, "Players: 10000", DT_LEFT);
	
	rect.left = (LONG)(m_fWidth - (float)size.cx + m_fScreenOffsetX);
	rect.right = (LONG)(m_fScreenOffsetX + m_fWidth);
	pDefaultFont->RenderSmallerText(NULL, szPlayers, rect, 288, 0xFFBEBEBE, true);

	rect.left = (LONG)(m_fScalar * 10.0f + m_fScreenOffsetX);
	rect.top = (LONG)((m_fHeaderHeight - pDefaultFont->GetSmallerFontSizeY() - 3) + m_fScreenOffsetY);
	rect.right = (LONG)(m_fScalar * 56.0f + m_fScreenOffsetX);
	rect.bottom = (LONG)(m_fHeaderHeight + m_fScreenOffsetY);
	pDefaultFont->RenderSmallerText(NULL, "id", rect, 288, 0xFF95B0D0, true);

	rect.left += (LONG)(m_fNameOffset * m_fWidth);
	rect.right += (LONG)(m_fNameOffset * m_fWidth);
	pDefaultFont->RenderSmallerText(NULL, "name", rect, 288, 0xFF95B0D0, true);

	rect.left += (LONG)(m_fScoreOffset * m_fWidth);
	rect.right += (LONG)(m_fScoreOffset * m_fWidth);
	pDefaultFont->RenderSmallerText(NULL, "score", rect, 288, 0xFF95B0D0, true);

	rect.left += (LONG)(m_fPingOffset * m_fWidth);
	rect.right += (LONG)(m_fPingOffset * m_fWidth);
	pDefaultFont->RenderSmallerText(NULL, "ping", rect, 288, 0xFF95B0D0, true);	
}

VOID CALLBACK OnScoreBoardEvent(UINT nEvent, int nControlID, CDXUTControl* pControl, void* pUserContext)
{
	if (nEvent == EVENT_LISTBOX_ITEM_DBLCLK)
	{
		pScoreBoard->ProcessClickEvent();
	}
}

void CScoreBoard::MsgProc(HWND hwnd, UINT uMsg,
	WPARAM wParam, LPARAM lParam)
{
	if (m_pDialog && m_pListBox)
	{
		POINT p;
		GetCursorPos(&p);
		m_pListBox->HandleMouse(uMsg, p, wParam, lParam);
		m_pListBox->HandleKeyboard(uMsg, wParam, lParam);
		m_pDialog->MsgProc(hwnd, uMsg, wParam, lParam);		
	}
}