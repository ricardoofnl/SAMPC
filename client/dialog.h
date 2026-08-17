
#pragma once

#define IDC_DLGEDITBOX 19
#define IDC_DLGBUTTON1 20
#define IDC_DLGBUTTON2 21

// dxut draws the caption above the client area, so control coordinates start
// right under it and only need a small margin
#define DIALOG_CONTENT_MARGIN 8
#define DIALOG_SIDE_MARGIN    12
// vertical gap between the content block, the control under it and the buttons
#define DIALOG_ROW_GAP        10
#define DIALOG_CORNER_RADIUS  8
// breathing room between tablist columns
#define DIALOG_COLUMN_GAP     18

// sizes measured off the original 0.3.7-R5 dialog
#define DIALOG_BUTTON_WIDTH  96
#define DIALOG_BUTTON_HEIGHT 26
#define DIALOG_EDIT_HEIGHT   40
#define DIALOG_MIN_WIDTH     230

// dark translucent panel palette. the dxut skin is greyscale and these modulate
// it, so the frame, caption, buttons and list all end up on one surface
#define DLG_COL_PANEL	D3DCOLOR_ARGB(235,  10,  12,  16)
#define DLG_COL_HEADER	D3DCOLOR_ARGB(240,  30,  35,  44)
#define DLG_COL_BORDER	D3DCOLOR_ARGB(255,  72,  84, 100)
#define DLG_COL_ACCENT	D3DCOLOR_ARGB(240,  52,  96, 148)
#define DLG_COL_TEXT	D3DCOLOR_ARGB(255, 226, 232, 240)
#define DLG_COL_CONTENT	0xFFA9C4E4

#define DIALOG_STYLE_MSGBOX 0
#define DIALOG_STYLE_INPUT 1
#define DIALOG_STYLE_LIST 2
#define DIALOG_STYLE_PASSWORD 3
#define DIALOG_STYLE_TABLIST 4
#define DIALOG_STYLE_TABLIST_HEADERS 5

// shared so the scoreboard can sit on the same palette
void TintElement(CDXUTElement* pElement, D3DCOLOR normal,
	D3DCOLOR mouseover, D3DCOLOR pressed, D3DCOLOR text);

class CDialog
{
private:
	IDirect3DDevice9* m_pDevice;
	int m_iPosX;
	int m_iPosY;
	int m_iWidth;
	int m_iHeight;
	int m_iButtonWidth;
	int m_iButtonHeight;
	CDXUTDialog* m_pDialog;
	CDXUTListBox* m_pListBox;
	CDXUTIMEEditBox* m_pEditBox;
	bool m_bVisible;
	int m_iDialogID;
	int m_iDialogStyle;
	char* m_szContent;
	SIZE m_ContentSize;
	char m_szCaption[64];
	bool m_bSendResponse;

	// tablist headers style draws its first line above the list instead of in it
	char m_szHeaders[MAX_LISTBOX_COLUMNS + 1][MAX_LISTBOX_TEXT_IN_COLUMN];
	int m_iHeaderOffset[MAX_LISTBOX_COLUMNS + 1];
	int m_iHeaderColumns;

public:
	CDialog(IDirect3DDevice9* pDevice);

	void ResetDialogControls();
	void StyleControls();
	bool MsgProc(UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool HandleInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	bool IsCandidateActive();
	void GetRect(RECT* rect);
	LONG GetTextWidth(char* szText);
	LONG GetFontHeight();
	void SetupList(char* szContent, SIZE* pSize);
	void Show(int iID, int iStyle, char* szCaption, char* szContent, char* szButton1, char* szButton2, bool bSendResponse);
	void Hide();
	void Draw();
	void DrawRoundedPanel(RECT* pRect, int iCaptionHeight);
	void UpdateFont();
	void SendResponse(bool bResponse);

	bool IsVisible() { return m_bVisible; };

	static void CALLBACK OnEvent(UINT nEvent, int nControlID,
		CDXUTControl* pControl, void* pUserContext);
};
