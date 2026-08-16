//----------------------------------------------------------
//
// SA:MP Multiplayer Modification For GTA:SA
// Copyright 2004-2005 SA:MP team
//
// Version: $Id: chatwindow.cpp,v 1.20 2006/05/08 14:31:50 kyeman Exp $
//
//----------------------------------------------------------

#include "main.h"

#define DRAW_TEXT_FORMAT DT_NOCLIP | DT_SINGLELINE | DT_LEFT

//----------------------------------------------------

CChatWindow::CChatWindow(IDirect3DDevice9* pD3DDevice,
	CFontRender* pFontRender, char* szPath)
{
	m_pD3DDevice = pD3DDevice;
	m_pFontRender = pFontRender;
	m_iEnabled = CHAT_WINDOW_MODE_FULL;
	m_iEnabledCache = -1;

	// Create a sprite to use when drawing text
	D3DXCreateSprite(pD3DDevice, &m_pChatTextSprite);
	D3DXCreateSprite(pD3DDevice, &m_pChatTextureSprite);

	SecureZeroMemory(m_ChatWindowEntries, sizeof(m_ChatWindowEntries));

	m_dwChatTextColor = D3DCOLOR_ARGB(255, 255, 255, 255);
	m_dwChatInfoColor = D3DCOLOR_ARGB(255, 136, 170, 98);
	m_dwChatDebugColor = D3DCOLOR_ARGB(255, 169, 196, 228);
	//m_dwChatBackgroundColor = D3DCOLOR_ARGB(255, 0, 0, 0);
	m_iPageSize = DISP_MESSAGES;
	m_bShowTimeStamp = false;

	m_lFontSizeY = 0;
	m_lTimeFontSizeX = 0;
	m_lChatWindowBottom = 0;
	m_bRenderingToSurface = false;
	m_bSurfaceAvailable = false;
	m_bSurfaceUpdateRequired = false;
	m_bLogFileCreated = false;
	m_szChatLogFile[0] = '\0';

	if(szPath && szPath[0] != '\0')
	{
		sprintf_s(m_szChatLogFile, "%s\\" CHAT_LOG_FILE, szPath);

		FILE* pFile = NULL;
		fopen_s(&pFile, m_szChatLogFile, "w");
		if (pFile)
		{
			m_bLogFileCreated = true;
			fclose(pFile);
		}
	}

	m_pSurface = NULL;
	m_pTexture = NULL;
	m_pRenderToSurface = NULL;
	//m_dwLastUpdateTick = GetTickCount();
	m_iLastTrackPos = 1;
	m_pScrollBar = NULL;
	m_pGameUI = NULL;
	//m_dw282 = 0;

	RestoreDeviceObjects();
}

//----------------------------------------------------

CChatWindow::~CChatWindow()
{
	DeleteDeviceObjects();
}

//----------------------------------------------------

void CChatWindow::RestoreDeviceObjects()
{
	m_bRenderingToSurface = true;

	SAFE_RELEASE(m_pSurface);
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pRenderToSurface);

	m_pD3DDevice->GetDisplayMode(0, &m_DisplayMode);

	if (pConfigFile->GetInt("directmode"))
	{
		AddDebugMessage("ChatWindow: Using direct drawing mode.");
		UpdateFontSizes();
		m_bRenderingToSurface = false;
	}
	else
	{
		HRESULT hr;

		if (m_DisplayMode.Width > 1024)
			hr = D3DXCreateTexture(m_pD3DDevice, 2048, 1024, 1,
				D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pTexture);
		else
			hr = D3DXCreateTexture(m_pD3DDevice, 1024, 512, 1,
				D3DUSAGE_RENDERTARGET, D3DFMT_A8R8G8B8, D3DPOOL_DEFAULT, &m_pTexture);

		if (SUCCEEDED(hr))
		{
			D3DSURFACE_DESC desc;

			m_pTexture->GetSurfaceLevel(0, &m_pSurface);
			m_pSurface->GetDesc(&desc);

			if (SUCCEEDED(D3DXCreateRenderToSurface(m_pD3DDevice, desc.Width,
				desc.Height, desc.Format, TRUE, D3DFMT_D16, &m_pRenderToSurface)))
			{
				UpdateFontSizes();
				m_bSurfaceAvailable = false;
				m_bSurfaceUpdateRequired = true;
			}
			else
			{
				AddDebugMessage("ChatWindow: Can't create a render to surface. Will use direct mode.");
				UpdateFontSizes();
				m_bRenderingToSurface = false;
			}
		}
		else
		{
			AddDebugMessage("ChatWindow: Can't create a render surface texture. Will use direct mode.");
			UpdateFontSizes();
			m_bRenderingToSurface = false;
		}
	}
}

//----------------------------------------------------

void CChatWindow::UpdateFontSizes()
{
	RECT rectSize;

	m_pFontRender->GetDXFontCE()->DrawTextA(0,"Y",-1,&rectSize,DT_CALCRECT|DT_SINGLELINE|DT_LEFT,0xFF000000);
	m_lFontSizeY = rectSize.bottom - rectSize.top;

	m_pFontRender->GetDXFontCE()->DrawTextA(0,"[19:58:34]",-1,&rectSize,DT_CALCRECT|DT_SINGLELINE|DT_LEFT,0xFF000000);
	m_lTimeFontSizeX = rectSize.right - rectSize.left;

	UpdateChatWindowBottom();
}

//----------------------------------------------------

// keep in sync with the rect walk in DrawChatLines(), cmdwindow needs it
// before the first frame is drawn
void CChatWindow::UpdateChatWindowBottom()
{
	m_lChatWindowBottom = 10 + (m_iPageSize * (m_lFontSizeY + 1)) + (2 * m_lFontSizeY) + 2;
}

//----------------------------------------------------

void CChatWindow::ForceHide(bool bHide)
{
	if (bHide)
	{
		m_iEnabledCache = m_iEnabled;
		m_iEnabled = CHAT_WINDOW_MODE_OFF;
	}
	else
	{
		m_iEnabled = m_iEnabledCache;
		m_iEnabledCache = -1;
	}
	m_bSurfaceUpdateRequired = true;
}

//----------------------------------------------------

void CChatWindow::DeleteDeviceObjects()
{
	SAFE_RELEASE(m_pSurface);
	SAFE_RELEASE(m_pTexture);
	SAFE_RELEASE(m_pRenderToSurface);
}

//----------------------------------------------------

void CChatWindow::OnLostDevice()
{
	if (m_pChatTextSprite)
		m_pChatTextSprite->OnLostDevice();
	if (m_pChatTextureSprite)
		m_pChatTextureSprite->OnLostDevice();
}

//----------------------------------------------------

void CChatWindow::OnResetDevice()
{
	if (m_pChatTextSprite)
		m_pChatTextSprite->OnResetDevice();
	if (m_pChatTextureSprite)
		m_pChatTextureSprite->OnResetDevice();
}

//----------------------------------------------------

void CChatWindow::FilterInvalidChars(PCHAR szString)
{
	while (*szString) {
		if (*szString > 0 && *szString < ' ') {
			*szString = ' ';
		}
		szString++;
	}
}

//----------------------------------------------------

void CChatWindow::UpdateScrollBar()
{
	if (m_pScrollBar)
	{
		m_pScrollBar->SetLocation(10,40);
		m_pScrollBar->SetSize(20,(m_iPageSize*(m_lFontSizeY+1))-60);
		m_pScrollBar->SetTrackRange(1,100);
		m_pScrollBar->SetPageSize(m_iPageSize);
		m_pScrollBar->SetTrackPos(100-m_iPageSize);
	}
}

//----------------------------------------------------

void CChatWindow::ResetDialogControls(CDXUTDialog* pGameUI)
{
	m_pGameUI = pGameUI;

	if(pGameUI) {
		m_pScrollBar = new CDXUTScrollBar(pGameUI);
		pGameUI->AddControl(m_pScrollBar);
		m_pScrollBar->SetVisible(true);
		m_pScrollBar->SetEnabled(true);
		UpdateScrollBar();
	}
}

//----------------------------------------------------

void CChatWindow::LogEntryToFile(CHAR eType, PCHAR szString, PCHAR szNick, PCHAR szTimeStamp)
{
	if (m_bLogFileCreated)
	{
		FILE* pFile;
		fopen_s(&pFile, m_szChatLogFile, "a");
		if (pFile)
		{
			/*char szTimeStamp[11];
			time_t tm = time(NULL);
			struct tm* t = localtime(&tm);
			strftime(szTimeStamp, sizeof(szTimeStamp), "[%H:%M:%S]", t);*/

			if (eType == CHAT_TYPE_CHAT)
				fprintf(pFile, "%s <%s> %s\r\n", szTimeStamp, szNick, szString);
			else if (eType == CHAT_TYPE_INFO || eType == CHAT_TYPE_DEBUG)
				fprintf(pFile, "%s %s\r\n", szTimeStamp, szString);

			fclose(pFile);
		}
	}
}

//----------------------------------------------------

void CChatWindow::PushBack()
{
	// regions overlap, memcpy would be undefined here
	memmove(&m_ChatWindowEntries[0],&m_ChatWindowEntries[1],
		sizeof(CHAT_WINDOW_ENTRY)*(MAX_MESSAGES-1));
}

//----------------------------------------------------

void CChatWindow::AddToChatWindowBuffer(CHAR eType, PCHAR szString,
	PCHAR szNick, DWORD dwTextColor, DWORD dwChatColor)
{
	int n = MAX_MESSAGES - 1;
	int iBestLineLength;

	PushBack();

	if (!szNick || strlen(szNick) <= MAX_PLAYER_NAME)
	{
		m_ChatWindowEntries[n].eType = eType;
		m_ChatWindowEntries[n].dwTextColor = dwTextColor;
		m_ChatWindowEntries[n].dwNickColor = dwChatColor;
		//m_ChatWindowEntries[n].dwTime = (DWORD)time(NULL);

		struct tm t;
		time_t ct = time(NULL);
		if(localtime_s(&t, &ct) == 0)
			strftime(m_ChatWindowEntries[n].szTimeStamp,11,"[%H:%M:%S]",&t);
		else
			m_ChatWindowEntries[n].szTimeStamp[0] = '\0'; // else the shifted entry's stamp sticks

		if (szNick)
		{
			strcpy_s(m_ChatWindowEntries[n].szNick, szNick);
			strcat_s(m_ChatWindowEntries[n].szNick, ":");
		}
		else
			m_ChatWindowEntries[n].szNick[0] = '\0';

		LogEntryToFile(eType, szString, szNick, m_ChatWindowEntries[n].szTimeStamp);

		if (m_ChatWindowEntries[n].eType != CHAT_TYPE_CHAT || strlen(szString) <= MAX_LINE_LENGTH)
		{
			strncpy_s(m_ChatWindowEntries[n].szMessage, szString, MAX_MESSAGE_LENGTH);
			m_ChatWindowEntries[n].szMessage[MAX_MESSAGE_LENGTH] = '\0';
			m_bSurfaceUpdateRequired = true;
		}
		else
		{
			iBestLineLength = MAX_LINE_LENGTH;

			while (szString[iBestLineLength] != ' ' && iBestLineLength)
				iBestLineLength--;

			if ((MAX_LINE_LENGTH - iBestLineLength) <= 12)
			{
				strncpy_s(m_ChatWindowEntries[n].szMessage,szString,iBestLineLength);
				m_ChatWindowEntries[n].szMessage[iBestLineLength] = '\0';

				PushBack();

				m_ChatWindowEntries[n].eType = eType;
				m_ChatWindowEntries[n].dwNickColor = dwChatColor;
				m_ChatWindowEntries[n].dwTextColor = dwTextColor;
				m_ChatWindowEntries[n].szNick[0] = '\0';

				strcpy_s(m_ChatWindowEntries[n].szMessage,szString+(iBestLineLength+1));

				m_bSurfaceUpdateRequired = true;
			}
			else
			{
				strncpy_s(m_ChatWindowEntries[n].szMessage, szString, MAX_LINE_LENGTH);
				m_ChatWindowEntries[n].szMessage[MAX_LINE_LENGTH] = '\0';

				PushBack();

				m_ChatWindowEntries[n].dwNickColor = dwChatColor;
				m_ChatWindowEntries[n].eType = eType;
				m_ChatWindowEntries[n].dwTextColor = dwTextColor;
				m_ChatWindowEntries[n].szNick[0] = '\0';

				strcpy_s(m_ChatWindowEntries[n].szMessage, szString + MAX_LINE_LENGTH);

				m_bSurfaceUpdateRequired = true;
			}
		}
	}
}

//----------------------------------------------------

void CChatWindow::AddChatMessage(CHAR* szNick, DWORD dwNickColor, CHAR* szMessage)
{
	FilterInvalidChars(szMessage);

	AddToChatWindowBuffer(CHAT_TYPE_CHAT, szMessage, szNick, m_dwChatTextColor, dwNickColor);
}

//----------------------------------------------------

void CChatWindow::AddInfoMessage(CHAR* szFormat, ...)
{
	char tmp_buf[512];
	memset(tmp_buf, 0, 512);

	va_list args;
	va_start(args, szFormat);
	vsprintf_s(tmp_buf, szFormat, args);
	va_end(args);

	FilterInvalidChars(tmp_buf);

	AddToChatWindowBuffer(CHAT_TYPE_INFO, tmp_buf, NULL, m_dwChatInfoColor, 0);
}

//----------------------------------------------------

void CChatWindow::AddDebugMessage(CHAR* szFormat, ...)
{
	char tmp_buf[512];
	memset(tmp_buf, 0, 512);

	va_list args;
	va_start(args, szFormat);
	vsprintf_s(tmp_buf, szFormat, args);
	va_end(args);

	FilterInvalidChars(tmp_buf);

	AddToChatWindowBuffer(CHAT_TYPE_DEBUG, tmp_buf, NULL, m_dwChatDebugColor, 0);
	OutputDebugString(tmp_buf);
}

//----------------------------------------------------

void CChatWindow::AddClientMessage(DWORD dwColor, PCHAR szStr)
{
	if (strlen(szStr) <= 144)
	{
		dwColor = (dwColor >> 8) | 0xFF000000; // convert to ARGB

		FormatGameTextKey(szStr, 255);
		FilterInvalidChars(szStr);

		AddToChatWindowBuffer(CHAT_TYPE_INFO, szStr, NULL, dwColor, 0);
	}
}

//----------------------------------------------------

void CChatWindow::RenderText(CHAR* sz, RECT rect, DWORD dwColor)
{
	ID3DXFont* pFont = m_pFontRender->GetDXFont();
	ID3DXFontCE* pFontCE = m_pFontRender->GetDXFontCE();
	char szBuffer[512] = { 0 };
	INT iCount;

	if (m_iEnabled == CHAT_WINDOW_MODE_FULL)
	{
		//memset(szBuffer, 0, sizeof(szBuffer));
		strncpy_s(szBuffer, sz, sizeof(szBuffer) - 1);
		RemoveColorEmbedsFromString(szBuffer);
		iCount = (INT)strlen(szBuffer);

		if (pGame->GetScreenWidth() <= 1280)
		{
			rect.top -= 1;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.top += 2;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.top -= 1;
			rect.left -= 1;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left += 2;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left -= 1;
		}
		else
		{
			rect.top -= 2;
			rect.bottom -= 2;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left -= 1;
			rect.top += 1;
			rect.bottom += 1;
			rect.right -= 1;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left += 2;
			rect.right += 2;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left -= 1;
			rect.right -= 1;
			rect.top += 3;
			rect.bottom += 3;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left -= 1;
			rect.top -= 1;
			rect.bottom -= 1;
			rect.right -= 1;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left += 2;
			rect.right += 2;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.top -= 1;
			rect.bottom -= 1;
			rect.left -= 3;
			rect.right -= 3;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left += 4;
			rect.right += 4;
			pFont->DrawTextA(m_pChatTextSprite, szBuffer, iCount, &rect, DRAW_TEXT_FORMAT, 0xFF000000);
			rect.left -= 2;
			rect.right -= 2;
		}
	}

	pFontCE->DrawTextA(m_pChatTextSprite, sz, strlen(sz), &rect, DRAW_TEXT_FORMAT, dwColor | 0xFF000000);
}

//----------------------------------------------------

void CChatWindow::DrawChatLines()
{
	RECT rect;
	RECT rectSize;
	int x=0;
	int iMessageAt;
	//char szTimeStamp[64];

	if (!m_pScrollBar)
		return;

	rect.top = 10;
	rect.left = 45;
	rect.bottom = 110;
	rect.right = 550;

	if (m_pScrollBar->GetTrackPos() < 1)
		m_pScrollBar->SetTrackPos(1);

	iMessageAt = m_pScrollBar->GetTrackPos();
	if (iMessageAt < 1)
		iMessageAt = 1;

	if (m_iPageSize)
	{
		while (x != m_iPageSize)
		{
			/*if (m_bShowTimeStamp)
			{
				struct tm* t = localtime((const time_t*)&m_ChatWindowEntries[iMessageAt].dwTime);
				memset(szTimeStamp, 0, sizeof(szTimeStamp));
				if (t)
					strftime(szTimeStamp, sizeof(szTimeStamp), "[%H:%M:%S]", t);
			}*/

			switch (m_ChatWindowEntries[iMessageAt].eType)
			{
			case CHAT_TYPE_CHAT:
				if (m_bShowTimeStamp && m_ChatWindowEntries[iMessageAt].szTimeStamp[0] != '\0' &&
					m_ChatWindowEntries[iMessageAt].szNick[0] != '\0')
				{
					RenderText(m_ChatWindowEntries[iMessageAt].szTimeStamp, rect, m_ChatWindowEntries[iMessageAt].dwTextColor);
					rect.left = m_lTimeFontSizeX + 50;
				}
				if (m_ChatWindowEntries[iMessageAt].szNick[0] != '\0')
				{
					m_pFontRender->GetDXFontCE()->DrawTextA(0, m_ChatWindowEntries[iMessageAt].szNick,
						-1, &rectSize, DT_CALCRECT | DT_LEFT, 0xFF000000);
					RenderText(m_ChatWindowEntries[iMessageAt].szNick, rect, m_ChatWindowEntries[iMessageAt].dwNickColor);
					rect.left += (rectSize.right - rectSize.left) + 5;
				}
				RenderText(m_ChatWindowEntries[iMessageAt].szMessage, rect, m_ChatWindowEntries[iMessageAt].dwTextColor);
				break;

			case CHAT_TYPE_INFO:
			case CHAT_TYPE_DEBUG:
				if (m_bShowTimeStamp && m_ChatWindowEntries[iMessageAt].szTimeStamp[0] != '\0')
				{
					RenderText(m_ChatWindowEntries[iMessageAt].szTimeStamp, rect, m_ChatWindowEntries[iMessageAt].dwTextColor);
					rect.left = m_lTimeFontSizeX + 50;
				}
				RenderText(m_ChatWindowEntries[iMessageAt].szMessage,rect, m_ChatWindowEntries[iMessageAt].dwTextColor);
				break;
			}

			rect.top += m_lFontSizeY + 1;
			rect.bottom = rect.top + m_lFontSizeY + 1;
			rect.left = 45;

			iMessageAt++;
			x++;
		}
		m_lChatWindowBottom = m_lFontSizeY + rect.bottom + 1;
	}
}

//----------------------------------------------------

void CChatWindow::Draw()
{
	if (m_pScrollBar)
	{
		if (m_iEnabled && (m_pScrollBar->GetTrackPos() != (100 - m_iPageSize) || pCmdWindow->isEnabled()))
			m_pScrollBar->SetVisible(true);
		else
			m_pScrollBar->SetVisible(false);

		if (m_iEnabled)
		{
			if (m_bRenderingToSurface && m_bSurfaceAvailable)
			{
				m_pChatTextureSprite->Begin(D3DXSPRITE_ALPHABLEND);
				m_pChatTextureSprite->Draw(m_pTexture, NULL, NULL, NULL, 0xFFFFFFFF);
				m_pChatTextureSprite->End();
			}
			else
			{
				m_pChatTextSprite->Begin(D3DXSPRITE_ALPHABLEND);
				DrawChatLines();
				m_pChatTextSprite->End();
			}
		}
	}
}

//----------------------------------------------------

void CChatWindow::UpdateSurface()
{
	if (m_bRenderingToSurface && m_pD3DDevice && m_pRenderToSurface &&
		m_pTexture && m_pSurface && m_pScrollBar) // && m_pFontRender)
	{
		if (pGame->IsMenuActive())
		{
			m_bSurfaceAvailable = false;
			m_bSurfaceUpdateRequired = true;
		}
		else
		{
			if (m_iEnabled && (m_bSurfaceUpdateRequired || m_iLastTrackPos != m_pScrollBar->GetTrackPos()))
			{
				m_pRenderToSurface->BeginScene(m_pSurface, NULL);
				m_pD3DDevice->Clear(0, NULL, D3DCLEAR_TARGET | D3DCLEAR_ZBUFFER, 0, 0, 0);
				m_pChatTextSprite->Begin(D3DXSPRITE_ALPHABLEND);
				m_pD3DDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_BLENDFACTOR);

				DrawChatLines();

				m_pChatTextSprite->End();
				m_pRenderToSurface->EndScene(0);

				m_bSurfaceUpdateRequired = false;
				m_bSurfaceAvailable = true;
				//m_dwLastUpdateTick = GetTickCount();
				m_iLastTrackPos = m_pScrollBar->GetTrackPos();
			}
		}
	}
}

//----------------------------------------------------

void CChatWindow::ResetPage()
{
	if (m_pScrollBar)
	{
		m_pScrollBar->SetTrackPos(100 - m_iPageSize);
	}
}

//----------------------------------------------------

void CChatWindow::Scroll(INT iAmount)
{
	if (m_iEnabled && m_pScrollBar)
	{
		UINT uiNumberOfLines;
		SystemParametersInfo(SPI_GETWHEELSCROLLLINES, 0, &uiNumberOfLines, 0);
		m_pScrollBar->Scroll(-(iAmount * (INT)uiNumberOfLines));
	}
}

//----------------------------------------------------

void CChatWindow::CycleMode()
{
	if (m_iEnabledCache != -1)
		return;

	m_bSurfaceUpdateRequired = true;

	if((m_iEnabled--) <= 0)
		m_iEnabled = CHAT_WINDOW_MODE_FULL;
}

//----------------------------------------------------

void CChatWindow::PageUp()
{
	if (m_iEnabled && m_pScrollBar &&
		!pGame->IsMenuActive() && !pCmdWindow->IsCandidateActive())
	{
		int iPosition = m_pScrollBar->GetTrackPos() - m_iPageSize;
		
		if (iPosition < 1)
			iPosition = 1;
		
		m_pScrollBar->SetTrackPos(iPosition);
	}
}

//----------------------------------------------------

void CChatWindow::PageDown()
{
	if (m_iEnabled && m_pScrollBar &&
		!pGame->IsMenuActive() && !pCmdWindow->IsCandidateActive())
	{
		int iNewPos = m_pScrollBar->GetTrackPos() + m_iPageSize;

		if (iNewPos > (100 - m_iPageSize))
			iNewPos = 100 - m_iPageSize;

		m_pScrollBar->SetTrackPos(iNewPos);
	}
}

//----------------------------------------------------

void CChatWindow::SetPageSize(INT iPageSize)
{
	if (iPageSize >= 10 && iPageSize <= 100)
	{
		m_iPageSize = iPageSize;
		UpdateChatWindowBottom();
		UpdateScrollBar();
		m_bSurfaceUpdateRequired = true;
	}
}

//----------------------------------------------------
// EOF