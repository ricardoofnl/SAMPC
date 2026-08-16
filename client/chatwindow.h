//----------------------------------------------------------
//
// SA:MP Multiplayer Modification For GTA:SA
// Copyright 2004-2005 SA:MP team
//
// Version: $Id: chatwindow.h,v 1.10 2006/05/08 13:28:46 kyeman Exp $
//
//----------------------------------------------------------

#pragma once

#define MAX_MESSAGE_LENGTH			144
#define MAX_LINE_LENGTH				MAX_MESSAGE_LENGTH / 2
#define MAX_MESSAGES				100
#define DISP_MESSAGES				10

enum eChatMessageType {
	CHAT_TYPE_NONE = 0,
	CHAT_TYPE_CHAT = 2,
	CHAT_TYPE_INFO = 4,
	CHAT_TYPE_DEBUG = 8
};

typedef struct _CHAT_WINDOW_ENTRY
{
	CHAR eType;
	CHAR szMessage[MAX_MESSAGE_LENGTH+1];
	CHAR szNick[MAX_PLAYER_NAME+1];
	DWORD dwTextColor;
	DWORD dwNickColor;
	char szTimeStamp[12];
} CHAT_WINDOW_ENTRY;

#define CHAT_WINDOW_MODE_OFF	0
#define CHAT_WINDOW_MODE_LIGHT	1
#define CHAT_WINDOW_MODE_FULL	2

//----------------------------------------------------

class CChatWindow
{
private:
	IDirect3DDevice9* m_pD3DDevice;
	ID3DXSprite* m_pChatTextSprite;
	ID3DXSprite* m_pChatTextureSprite;
	IDirect3DTexture9* m_pTexture;
	IDirect3DSurface9* m_pSurface;
	ID3DXRenderToSurface* m_pRenderToSurface;
	D3DDISPLAYMODE m_DisplayMode;
	CDXUTDialog* m_pGameUI;
	CDXUTScrollBar* m_pScrollBar;
	//CDXUTEditBox* m_pEditBackground;
	DWORD m_dwChatTextColor;
	DWORD m_dwChatInfoColor;
	DWORD m_dwChatDebugColor;
	//DWORD m_dwChatBackgroundColor;
	int m_iEnabled;
	CFontRender* m_pFontRender;
	CHAT_WINDOW_ENTRY m_ChatWindowEntries[MAX_MESSAGES];
	bool m_bSurfaceUpdateRequired;
	char m_szChatLogFile[MAX_PATH];
	bool m_bLogFileCreated;
	int m_iPageSize;
	LONG m_lFontSizeY;
	LONG m_lTimeFontSizeX;
	bool m_bShowTimeStamp;
	int m_iLastTrackPos;
	bool m_bRenderingToSurface;
	//DWORD m_dwLastUpdateTick;
	bool m_bSurfaceAvailable;
	LONG m_lChatWindowBottom;
	int m_iEnabledCache;

public:
	void RestoreDeviceObjects();
	void DeleteDeviceObjects();
	void OnLostDevice();
	void OnResetDevice();
	void UpdateScrollBar();
	
	void PushBack();
	void FilterInvalidChars(PCHAR szString);
	void LogEntryToFile(CHAR eType, PCHAR szString, PCHAR szNick, PCHAR szTimeStamp);
	void AddToChatWindowBuffer(CHAR eType,PCHAR szString,
		PCHAR szNick,DWORD dwTextColor,DWORD dwChatColor);
	
	void Draw();
	void AddChatMessage(CHAR *szNick, DWORD dwNickColor, CHAR *szMessage);
	void AddInfoMessage(CHAR *szFormat, ...);
	void AddDebugMessage(CHAR *szFormat, ...);
	void AddClientMessage(DWORD dwColor, PCHAR szStr);

	void PageUp();
	void PageDown();

	void CycleMode();

	void RenderText(CHAR *sz,RECT rect,DWORD dwColor);
	void ResetDialogControls(CDXUTDialog *pGameUI);

	CChatWindow(IDirect3DDevice9* pD3DDevice, CFontRender* pFontRender, char* szPath);
	~CChatWindow();

	LONG GetChatWindowBottom() { return m_lChatWindowBottom; };
	bool IsEnabled() { return m_iEnabled != 0; };
	bool IsTimeStampVisible() { return m_bShowTimeStamp; };

	void DrawChatLines();
	void UpdateSurface();
	void ResetPage();
	void Scroll(INT iAmount);	
	void UpdateFontSizes();
	void UpdateChatWindowBottom();
	void SetPageSize(INT iPageSize);
	void ForceHide(bool bHide);

	void SetTimeStampVisisble(bool bVisible)
	{
		m_bShowTimeStamp = bVisible;
		m_bSurfaceUpdateRequired = true;
	};
};

//----------------------------------------------------
// EOF