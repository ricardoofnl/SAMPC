
#include "main.h"



CDialog::CDialog(IDirect3DDevice9* pDevice)
{
	m_pDevice = pDevice;
	m_iPosX = 0;
	m_iPosY = 0;
	m_iWidth = 600;
	m_iHeight = 300;
	m_iButtonWidth = 100;
	m_iButtonHeight = 30;
	m_pDialog = NULL;
	m_pListBox = NULL;
	m_pEditBox = NULL;
	m_szContent = NULL;
	m_iDialogID = 0;
	m_iDialogStyle = 0;
	m_bSendResponse = false;
	m_bVisible = false;
}

void CDialog::ResetDialogControls()
{
	SAFE_DELETE(m_pDialog);

	m_pDialog = new CDXUTDialog();
	m_pDialog->Init(pDialogResourceManager);
	m_pDialog->SetCallback(CDialog::OnEvent);
	m_pDialog->SetLocation(0, 0);
	m_pDialog->SetSize(600, 300);
	m_pDialog->EnableMouseInput(true);
	m_pDialog->EnableKeyboardInput(true);
	m_pDialog->SetBackgroundColors(D3DCOLOR_ARGB(220, 5, 5, 5));
	m_pDialog->SetVisible(false);

	m_pListBox = new CDXUTListBox(m_pDialog);
	m_pDialog->AddControl(m_pListBox);
	m_pListBox->SetLocation(10,10);
	m_pListBox->SetSize(m_iWidth, m_iHeight - 100);
	m_pListBox->OnInit();
	m_pListBox->GetElement(0)->TextureColor.Init(D3DCOLOR_ARGB(200, 255, 255, 255));
	m_pListBox->m_nColumns = 0;
	m_pListBox->SetEnabled(false);
	m_pListBox->SetVisible(false);

	m_pDialog->AddButton(IDC_DLGBUTTON1, "BUTTON1", 10, 5, m_iButtonWidth, m_iButtonHeight);
	m_pDialog->AddButton(IDC_DLGBUTTON2, "BUTTON2", 110, 5, m_iButtonWidth, m_iButtonHeight);
	m_pDialog->AddIMEEditBox(IDC_DLGEDITBOX, "", 10, 175, 570, 40, true, &m_pEditBox);

	if (pConfigFile->GetInt("ime"))
	{
		CDXUTIMEEditBox::EnableImeSystem(true);
		CDXUTIMEEditBox::StaticOnCreateDevice();
	}

	m_pEditBox->GetElement(0)->TextureColor.Init(D3DCOLOR_ARGB(240, 5, 5, 5));
	m_pEditBox->SetTextColor(D3DCOLOR_ARGB(255, 255, 255, 255));
	m_pEditBox->SetCaretColor(D3DCOLOR_ARGB(255, 150, 150, 150));
	m_pEditBox->SetSelectedBackColor(D3DCOLOR_ARGB(255, 185, 34, 40));
	m_pEditBox->SetSelectedTextColor(D3DCOLOR_ARGB(255, 10, 10, 15));
	m_pEditBox->SetEnabled(false);
	m_pEditBox->SetVisible(false);
}

bool CDialog::MsgProc(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	return CDXUTIMEEditBox::StaticMsgProc(uMsg, wParam, lParam) != false;
}

bool CDialog::IsCandidateActive()
{
	if (m_pEditBox)
	{
		return CDXUTIMEEditBox::IsCandidateActive();
	}
	return false;
}

void CDialog::GetRect(RECT* rect)
{
	rect->left = m_iPosX;
	rect->top = m_iPosY;
	rect->right = m_iPosX + m_iWidth;
	rect->bottom = m_iPosY + m_iHeight;
}

//----------------------------------------------------

// same two font sources the original uses, DXUT draws the frame, caption, buttons
// and controls, pDefaultFont draws the content text
void CDialog::UpdateFont()
{
	if (m_pDialog)
		m_pDialog->UpdateFont();
}

//----------------------------------------------------

void CDialog::Draw()
{
	if (!m_bVisible || !m_pDialog) return;

	// the chat input box takes priority, the original hides the dialog behind it
	if (pCmdWindow && pCmdWindow->isEnabled()) return;

	pGame->ToggleKeyInputsDisabled(2);

	m_pDialog->SetLocation(m_iPosX, m_iPosY);
	m_pDialog->SetSize(m_iWidth, m_iHeight);
	m_pDialog->OnRender(10.0f);

	if (!m_szContent) return;

	RECT rect;
	GetRect(&rect);

	if (m_iDialogStyle == DIALOG_STYLE_MSGBOX ||
		m_iDialogStyle == DIALOG_STYLE_INPUT ||
		m_iDialogStyle == DIALOG_STYLE_PASSWORD)
	{
		rect.left += 20;
		rect.top += DIALOG_CONTENT_MARGIN;

		pDefaultFont->RenderSmallerText(NULL, m_szContent, rect,
			DT_NOCLIP | DT_EXPANDTABS, 0xFFA9C4E4, true);
	}
}

LONG CDialog::GetTextWidth(char* szText)
{
	ID3DXFont* pFont = m_pDialog->GetFont(1)->pFont;
	if (szText && szText[0] != 0 && pFont)
	{
		char szBuffer[256];
		RECT rect;
		// a list line can be longer than this, and an overflowing strcpy_s kills
		// the process through the invalid parameter handler
		strncpy_s(szBuffer, szText, _TRUNCATE);
		RemoveColorEmbedsFromString(szBuffer);
		pFont->DrawTextA(0, szBuffer, -1, &rect, DT_EXPANDTABS | DT_NOCLIP | DT_CALCRECT, -1);
		return rect.right - rect.left;
	}
	return -1;
}

LONG CDialog::GetFontHeight()
{
	return m_pDialog->GetFont(1)->nHeight;
}

//----------------------------------------------------

// copies up to the next newline, returns where it stopped
static char* CopyDialogLine(char* szText, char* szOut, size_t uiOutSize)
{
	size_t i = 0;

	while (szText[i] && szText[i] != '\n' && i < (uiOutSize - 1))
	{
		szOut[i] = szText[i];
		i++;
	}
	szOut[i] = '\0';

	return szText + i;
}

//----------------------------------------------------

// fills the list box from the newline separated content and reports back the size
// it wants, the original measures and populates in the same pass
void CDialog::SetupList(char* szContent, SIZE* pSize)
{
	m_pListBox->RemoveAllItems();
	m_pListBox->m_nColumns = 0;

	char szLine[256];
	char* pText = szContent;
	int iMaxWidth = 0;
	int iTotalHeight = 0;
	int iIndex = 0;

	while (pText && *pText)
	{
		memset(szLine, 0, sizeof(szLine));
		pText = CopyDialogLine(pText, szLine, sizeof(szLine));

		if (strlen(szLine))
		{
			LONG lWidth = GetTextWidth(szLine);
			if (lWidth > iMaxWidth) iMaxWidth = (int)lWidth;

			iTotalHeight += GetFontHeight();
			m_pListBox->AddItem(szLine, iIndex++, (D3DCOLOR)-1);
		}

		if (!*pText) break;
		if (*pText == '\n') pText++;
	}

	if (pSize)
	{
		pSize->cx = iMaxWidth + 20;
		pSize->cy = iTotalHeight;
	}
}

void CDialog::Hide()
{
	pGame->ToggleKeyInputsDisabled(0);
	m_pDialog->SetVisible(false);
	m_bVisible = false;
}

//----------------------------------------------------

bool CDialog::HandleInput(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	if (!m_bVisible || !m_pDialog) return false;
	if (pCmdWindow && pCmdWindow->isEnabled()) return false;

	if (uMsg == WM_KEYUP)
	{
		// return answers with the left button, escape with the right one
		if (wParam == VK_RETURN)
		{
			SendResponse(true);
			return true;
		}
		if (wParam == VK_ESCAPE)
		{
			SendResponse(false);
			return true;
		}
	}

	return m_pDialog->MsgProc(hwnd, uMsg, wParam, lParam) != false;
}

//----------------------------------------------------

// bResponse is true for button1, false for button2 and for escape
void CDialog::SendResponse(bool bResponse)
{
	if (!m_bVisible) return;

	char szInputText[MAX_DIALOG_RESPONSE_TEXT + 1];
	short sListItem = -1;

	szInputText[0] = '\0';

	if (m_iDialogStyle == DIALOG_STYLE_LIST ||
		m_iDialogStyle == DIALOG_STYLE_TABLIST ||
		m_iDialogStyle == DIALOG_STYLE_TABLIST_HEADERS)
	{
		sListItem = (short)m_pListBox->GetSelectedIndex(-1);

		DXUTListBoxItem* pItem = m_pListBox->GetItem(sListItem);
		if (pItem)
		{
			strncpy_s(szInputText, pItem->strText, _TRUNCATE);
			RemoveColorEmbedsFromString(szInputText);
		}
	}
	else if (m_iDialogStyle == DIALOG_STYLE_INPUT ||
			 m_iDialogStyle == DIALOG_STYLE_PASSWORD)
	{
		strncpy_s(szInputText, m_pEditBox->GetText(), _TRUNCATE);
	}

	if (m_bSendResponse && pNetGame)
	{
		RakNet::BitStream bsSend;
		BYTE byteTextLen = (BYTE)strlen(szInputText);

		bsSend.Write((WORD)m_iDialogID);
		bsSend.Write((BYTE)(bResponse ? 1 : 0));
		bsSend.Write((WORD)sListItem);
		bsSend.Write(byteTextLen);
		if (byteTextLen)
			bsSend.Write(szInputText, byteTextLen);

		pNetGame->Send(RPC_ScrDialogResponse, &bsSend);
	}

	Hide();
}

void CDialog::Show(int iID, int iStyle, char* szCaption,
	char* szContent, char* szButton1, char* szButton2, bool bSendResponse)
{
	SIZE size;

	if (!m_pDialog || !m_pListBox || !m_pEditBox || !pDefaultFont) return;

	if (iID >= 0)
	{
		if (pCmdWindow && pCmdWindow->isEnabled())
			pCmdWindow->Disable();

		m_iDialogID = iID;
		m_iDialogStyle = iStyle;
		m_bSendResponse = bSendResponse;

		SecureZeroMemory(m_szCaption, sizeof(m_szCaption));
		strncpy_s(m_szCaption, szCaption, _TRUNCATE);

		if (m_szContent)
			free(m_szContent);
		DWORD dwLen = strlen(szContent);
		m_szContent = (char*)calloc(1, dwLen + 64);
		if (!m_szContent) return;
		// the size argument is the buffer, not the string, dwLen alone is one short
		// and strcpy_s answers that by terminating the process
		strcpy_s(m_szContent, dwLen + 64, szContent);

		pDefaultFont->MeasureSmallerText(&m_ContentSize, m_szContent, DT_EXPANDTABS);

		if (szCaption[0] != '\0')
		{
			pDefaultFont->MeasureText(&size, "Y", 0); // DT_LEFT ?
			m_pDialog->SetCaptionText(szCaption);
			m_pDialog->EnableCaption(true);
			m_pDialog->SetCaptionHeight(size.cy + 4);
			
		}

		// every style opts in to what it needs, otherwise the edit box from a
		// previous input dialog stays on screen over the next one
		m_pEditBox->SetVisible(false);
		m_pEditBox->SetEnabled(false);
		m_pListBox->SetVisible(false);
		m_pListBox->SetEnabled(false);

		int iEditHeight = (int)(m_pDialog->GetFont(1)->nHeight * 1.6f + 14.0f);
		int iContentTop = DIALOG_CONTENT_MARGIN + m_ContentSize.cy;

		switch (m_iDialogStyle)
		{
		case DIALOG_STYLE_MSGBOX:
		{
			m_iWidth = m_ContentSize.cx + 40;
			if (m_iWidth < (2 * m_iButtonWidth) + 30)
				m_iWidth = (2 * m_iButtonWidth) + 30;

			m_iHeight = DIALOG_CONTENT_MARGIN + m_ContentSize.cy + m_iButtonHeight + 30;
			break;
		}
		case DIALOG_STYLE_INPUT:
		case DIALOG_STYLE_PASSWORD:
		{
			m_iWidth = m_ContentSize.cx + 40;
			if (m_iWidth < (2 * m_iButtonWidth) + 30)
				m_iWidth = (2 * m_iButtonWidth) + 30;

			m_iHeight = DIALOG_CONTENT_MARGIN + m_ContentSize.cy + iEditHeight +
						m_iButtonHeight + 30;

			m_pEditBox->SetLocation(10, iContentTop);
			m_pEditBox->SetSize(m_iWidth - 20, iEditHeight);
			m_pEditBox->SetText("");
			m_pEditBox->SetVisible(true);
			m_pEditBox->SetEnabled(true);
			break;
		}
		case DIALOG_STYLE_LIST:
		case DIALOG_STYLE_TABLIST:
		case DIALOG_STYLE_TABLIST_HEADERS:
		{
			SIZE listSize;
			SetupList(m_szContent, &listSize);

			m_iWidth = listSize.cx + 40;
			if (m_iWidth < 400) m_iWidth = 400;
			if (m_iWidth > 800) m_iWidth = 800;
			if (m_iWidth < (2 * m_iButtonWidth) + 30)
				m_iWidth = (2 * m_iButtonWidth) + 30;

			int iListHeight = listSize.cy + (2 * (int)GetFontHeight());
			if (iListHeight < 200) iListHeight = 200;
			if (iListHeight > 400) iListHeight = 400;

			// content margin, list, gap, buttons, bottom margin
			m_iHeight = DIALOG_CONTENT_MARGIN + iListHeight + m_iButtonHeight + 30;

			m_pListBox->SetLocation(5, DIALOG_CONTENT_MARGIN);
			m_pListBox->SetSize(m_iWidth - 10, iListHeight);
			m_pListBox->SetVisible(true);
			m_pListBox->SetEnabled(true);
			break;
		}
		}

		CDXUTButton* pButton1 = m_pDialog->GetButton(IDC_DLGBUTTON1);
		if (pButton1)
			pButton1->SetText(szButton1);

		CDXUTButton* pButton = m_pDialog->GetButton(IDC_DLGBUTTON2);
		if (pButton)
		{
			if (szButton2[0] != '\0')
			{
				pButton->SetText(szButton2);
				pButton->SetVisible(true);
			}
			else
			{
				pButton->SetText("");
				pButton->SetVisible(false);
			}
		}

		// centre it now the style has settled on a size
		m_iPosX = (pGame->GetScreenWidth() - m_iWidth) / 2;
		m_iPosY = (pGame->GetScreenHeight() - m_iHeight) / 2;
		if (m_iPosX < 0) m_iPosX = 0;
		if (m_iPosY < 0) m_iPosY = 0;

		m_pDialog->SetLocation(m_iPosX, m_iPosY);
		m_pDialog->SetSize(m_iWidth, m_iHeight);

		// buttons sit on the bottom edge, the caption is drawn above the frame
		if (pButton1)
			pButton1->SetLocation(10, m_iHeight - m_iButtonHeight - 10);
		if (pButton)
			pButton->SetLocation(10 + m_iButtonWidth + 10, m_iHeight - m_iButtonHeight - 10);

		m_pDialog->SetVisible(true);

		switch (m_iDialogStyle)
		{
		case DIALOG_STYLE_INPUT:
		case DIALOG_STYLE_PASSWORD:
			m_pDialog->RequestFocus(m_pEditBox);
			break;
		case DIALOG_STYLE_LIST:
		case DIALOG_STYLE_TABLIST:
		case DIALOG_STYLE_TABLIST_HEADERS:
			m_pDialog->RequestFocus(m_pListBox);
			m_pListBox->SelectItem(0);
			break;
		}

		m_bVisible = true;
	}
	else if (m_bVisible)
	{
		Hide();
	}
}

void CALLBACK CDialog::OnEvent(UINT nEvent, int nControlID,
	CDXUTControl* pControl, void* pUserContext)
{
	if (!pDialog) return;

	switch (nControlID)
	{
	case IDC_DLGBUTTON1:
		pDialog->SendResponse(true);
		break;
	case IDC_DLGBUTTON2:
		pDialog->SendResponse(false);
		break;
	}
}
