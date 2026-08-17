
#pragma once

#define IDC_DLGEDITBOX 19
#define IDC_DLGBUTTON1 20
#define IDC_DLGBUTTON2 21

#define DIALOG_STYLE_MSGBOX 0
#define DIALOG_STYLE_INPUT 1
#define DIALOG_STYLE_LIST 2
#define DIALOG_STYLE_PASSWORD 3
#define DIALOG_STYLE_TABLIST 4
#define DIALOG_STYLE_TABLIST_HEADERS 5

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

public:
	CDialog(IDirect3DDevice9* pDevice);

	void ResetDialogControls();
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
	void UpdateFont();
	void SendResponse(bool bResponse);

	bool IsVisible() { return m_bVisible; };

	static void CALLBACK OnEvent(UINT nEvent, int nControlID,
		CDXUTControl* pControl, void* pUserContext);
};
