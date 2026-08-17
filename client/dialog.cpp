
#include "main.h"



CDialog::CDialog(IDirect3DDevice9* pDevice)
{
	m_pDevice = pDevice;
	m_iPosX = 0;
	m_iPosY = 0;
	m_iWidth = 600;
	m_iHeight = 300;
	// measured off the original r5 password dialog, 96x26 buttons over a body
	// that never goes under 230 wide, with a fixed 40 pixel edit control
	m_iButtonWidth = DIALOG_BUTTON_WIDTH;
	m_iButtonHeight = DIALOG_BUTTON_HEIGHT;
	m_pDialog = NULL;
	m_pListBox = NULL;
	m_pEditBox = NULL;
	m_szContent = NULL;
	m_iDialogID = 0;
	m_iDialogStyle = 0;
	m_bSendResponse = false;
	m_bVisible = false;
	m_iHeaderColumns = 0;
	memset(m_szHeaders, 0, sizeof(m_szHeaders));
	memset(m_iHeaderOffset, 0, sizeof(m_iHeaderOffset));
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
	m_pDialog->SetVisible(false);

	m_pListBox = new CDXUTListBox(m_pDialog);
	m_pDialog->AddControl(m_pListBox);
	m_pListBox->SetLocation(10,10);
	m_pListBox->SetSize(m_iWidth, m_iHeight - 100);
	m_pListBox->OnInit();
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

	m_pEditBox->SetEnabled(false);
	m_pEditBox->SetVisible(false);

	StyleControls();
}

//----------------------------------------------------

// the dxut skin texture is greyscale, so tinting an element per state is enough
// to pull the whole control onto the dark panel palette
void TintElement(CDXUTElement* pElement, D3DCOLOR normal,
	D3DCOLOR mouseover, D3DCOLOR pressed, D3DCOLOR text)
{
	if (!pElement) return;

	pElement->TextureColor.States[DXUT_STATE_NORMAL] = normal;
	pElement->TextureColor.States[DXUT_STATE_DISABLED] = normal;
	pElement->TextureColor.States[DXUT_STATE_HIDDEN] = 0;
	pElement->TextureColor.States[DXUT_STATE_FOCUS] = mouseover;
	pElement->TextureColor.States[DXUT_STATE_MOUSEOVER] = mouseover;
	pElement->TextureColor.States[DXUT_STATE_PRESSED] = pressed;

	for (int i = 0; i < MAX_CONTROL_STATES; i++)
		pElement->FontColor.States[i] = text;

	pElement->TextureColor.Blend(DXUT_STATE_NORMAL, 10.0f);
	pElement->FontColor.Blend(DXUT_STATE_NORMAL, 10.0f);
}

//----------------------------------------------------

void CDialog::StyleControls()
{
	if (!m_pDialog) return;

	// the square quad is left fully transparent, DrawRoundedPanel puts down the
	// rounded one instead
	m_pDialog->SetBackgroundColors(D3DCOLOR_ARGB(0, 0, 0, 0));

	// likewise the caption strip is part of the rounded panel now, only its text
	// still comes from dxut
	TintElement(m_pDialog->GetCaptionElement(),
		D3DCOLOR_ARGB(0, 0, 0, 0), D3DCOLOR_ARGB(0, 0, 0, 0), D3DCOLOR_ARGB(0, 0, 0, 0),
		DLG_COL_TEXT);

	// element 0 is the button face, element 1 the fill layer that lights up
	for (int i = IDC_DLGBUTTON1; i <= IDC_DLGBUTTON2; i++)
	{
		CDXUTButton* pButton = m_pDialog->GetButton(i);
		if (!pButton) continue;

		TintElement(pButton->GetElement(0),
			DLG_COL_HEADER, DLG_COL_BORDER, DLG_COL_ACCENT, DLG_COL_TEXT);
		TintElement(pButton->GetElement(1),
			D3DCOLOR_ARGB(0, 0, 0, 0), DLG_COL_ACCENT, DLG_COL_ACCENT, DLG_COL_TEXT);
	}

	if (m_pListBox)
	{
		// 0 is the list surface, 1 the selection bar
		TintElement(m_pListBox->GetElement(0),
			DLG_COL_PANEL, DLG_COL_PANEL, DLG_COL_PANEL, DLG_COL_TEXT);
		TintElement(m_pListBox->GetElement(1),
			DLG_COL_ACCENT, DLG_COL_ACCENT, DLG_COL_ACCENT, DLG_COL_TEXT);
	}

	if (m_pEditBox)
	{
		// the edit box is a nine slice, 0 is the middle and 1..8 the frame
		TintElement(m_pEditBox->GetElement(0),
			DLG_COL_PANEL, DLG_COL_PANEL, DLG_COL_PANEL, DLG_COL_TEXT);

		for (int i = 1; i <= 8; i++)
			TintElement(m_pEditBox->GetElement(i),
				DLG_COL_BORDER, DLG_COL_BORDER, DLG_COL_BORDER, DLG_COL_TEXT);

		m_pEditBox->SetTextColor(DLG_COL_TEXT);
		m_pEditBox->SetCaretColor(DLG_COL_TEXT);
		m_pEditBox->SetSelectedBackColor(DLG_COL_ACCENT);
		m_pEditBox->SetSelectedTextColor(DLG_COL_TEXT);
	}
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

// a triangle fan with the four corners cut into arcs, drawn in place of the square
// quad CDXUTDialog would otherwise put down
void CDialog::DrawRoundedPanel(RECT* pRect, int iCaptionHeight)
{
	if (!m_pDevice) return;

	struct PANEL_VERTEX
	{
		float x, y, z, h;
		D3DCOLOR color;
	};

	const int iCornerSteps = 6;
	const float fRadius = (float)DIALOG_CORNER_RADIUS;

	// centre first, then the outline, then back to the start to close the fan
	PANEL_VERTEX vertices[(4 * (iCornerSteps + 1)) + 2];
	int iVertex = 0;

	float fLeft = (float)pRect->left;
	float fTop = (float)pRect->top;
	float fRight = (float)pRect->right;
	float fBottom = (float)pRect->bottom;

	vertices[iVertex].x = (fLeft + fRight) * 0.5f;
	vertices[iVertex].y = (fTop + fBottom) * 0.5f;
	vertices[iVertex].z = 0.5f;
	vertices[iVertex].h = 1.0f;
	vertices[iVertex].color = DLG_COL_PANEL;
	iVertex++;

	// corner centres, clockwise from the top left, with the arc sweep for each
	const float fCornerX[4] = { fLeft + fRadius, fRight - fRadius, fRight - fRadius, fLeft + fRadius };
	const float fCornerY[4] = { fTop + fRadius, fTop + fRadius, fBottom - fRadius, fBottom - fRadius };
	const float fStart[4] = { 180.0f, 270.0f, 0.0f, 90.0f };

	for (int iCorner = 0; iCorner < 4; iCorner++)
	{
		for (int iStep = 0; iStep <= iCornerSteps; iStep++)
		{
			float fAngle = (fStart[iCorner] + (90.0f * iStep / iCornerSteps)) * 0.01745329f;

			vertices[iVertex].x = fCornerX[iCorner] + (cosf(fAngle) * fRadius);
			vertices[iVertex].y = fCornerY[iCorner] + (sinf(fAngle) * fRadius);
			vertices[iVertex].z = 0.5f;
			vertices[iVertex].h = 1.0f;
			// the caption strip sits at the top, so tint those rows brighter
			vertices[iVertex].color =
				(vertices[iVertex].y < (fTop + (float)iCaptionHeight)) ? DLG_COL_HEADER : DLG_COL_PANEL;
			iVertex++;
		}
	}

	vertices[iVertex] = vertices[1];
	iVertex++;

	DWORD dwOldFVF, dwOldZ, dwOldAlpha, dwOldSrc, dwOldDst, dwOldCull, dwOldLight;
	m_pDevice->GetFVF(&dwOldFVF);
	m_pDevice->GetRenderState(D3DRS_ZENABLE, &dwOldZ);
	m_pDevice->GetRenderState(D3DRS_ALPHABLENDENABLE, &dwOldAlpha);
	m_pDevice->GetRenderState(D3DRS_SRCBLEND, &dwOldSrc);
	m_pDevice->GetRenderState(D3DRS_DESTBLEND, &dwOldDst);
	m_pDevice->GetRenderState(D3DRS_CULLMODE, &dwOldCull);
	m_pDevice->GetRenderState(D3DRS_LIGHTING, &dwOldLight);

	m_pDevice->SetTexture(0, NULL);
	m_pDevice->SetPixelShader(NULL);
	m_pDevice->SetVertexShader(NULL);
	m_pDevice->SetFVF(D3DFVF_XYZRHW | D3DFVF_DIFFUSE);
	m_pDevice->SetRenderState(D3DRS_ZENABLE, FALSE);
	m_pDevice->SetRenderState(D3DRS_LIGHTING, FALSE);
	m_pDevice->SetRenderState(D3DRS_CULLMODE, D3DCULL_NONE);
	m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, TRUE);
	m_pDevice->SetRenderState(D3DRS_SRCBLEND, D3DBLEND_SRCALPHA);
	m_pDevice->SetRenderState(D3DRS_DESTBLEND, D3DBLEND_INVSRCALPHA);

	m_pDevice->DrawPrimitiveUP(D3DPT_TRIANGLEFAN, iVertex - 2, vertices, sizeof(PANEL_VERTEX));

	m_pDevice->SetRenderState(D3DRS_LIGHTING, dwOldLight);
	m_pDevice->SetRenderState(D3DRS_CULLMODE, dwOldCull);
	m_pDevice->SetRenderState(D3DRS_DESTBLEND, dwOldDst);
	m_pDevice->SetRenderState(D3DRS_SRCBLEND, dwOldSrc);
	m_pDevice->SetRenderState(D3DRS_ALPHABLENDENABLE, dwOldAlpha);
	m_pDevice->SetRenderState(D3DRS_ZENABLE, dwOldZ);
	m_pDevice->SetFVF(dwOldFVF);
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

	int iCaptionHeight = m_pDialog->GetCaptionHeight();

	// the dxut background is a square quad, so it is turned off and replaced with
	// a rounded one underneath the controls
	if (!m_pDialog->GetMinimized())
	{
		RECT panel;
		GetRect(&panel);
		DrawRoundedPanel(&panel, iCaptionHeight);
	}

	m_pDialog->OnRender(10.0f);

	if (!m_szContent) return;

	// minimised leaves only the caption bar on screen, the body text is not part
	// of the dxut dialog so it has to be dropped here or it floats on its own
	if (m_pDialog->GetMinimized()) return;

	if (m_iDialogStyle == DIALOG_STYLE_MSGBOX ||
		m_iDialogStyle == DIALOG_STYLE_INPUT ||
		m_iDialogStyle == DIALOG_STYLE_PASSWORD)
	{
		RECT rect;
		GetRect(&rect);

		// dxut shifts every control down past the caption, this text is drawn
		// straight to the screen so it has to clear the caption itself
		rect.left += DIALOG_SIDE_MARGIN;
		rect.right -= DIALOG_SIDE_MARGIN;
		rect.top += iCaptionHeight + DIALOG_CONTENT_MARGIN;
		rect.bottom = rect.top + m_ContentSize.cy;

		pDefaultFont->RenderSmallerText(NULL, m_szContent, rect,
			DT_EXPANDTABS, DLG_COL_CONTENT, true);
	}
	else if (m_iDialogStyle == DIALOG_STYLE_TABLIST_HEADERS && m_iHeaderColumns)
	{
		RECT rect;
		GetRect(&rect);

		int iTop = rect.top + iCaptionHeight + DIALOG_CONTENT_MARGIN;

		for (int i = 0; i < m_iHeaderColumns; i++)
		{
			RECT header;

			header.left = rect.left + DIALOG_SIDE_MARGIN + 4 + m_iHeaderOffset[i];
			header.right = rect.right - DIALOG_SIDE_MARGIN;
			header.top = iTop;
			header.bottom = iTop + (int)GetFontHeight();

			pDefaultFont->RenderSmallerText(NULL, m_szHeaders[i], header,
				DT_LEFT, DLG_COL_TEXT, true);
		}
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

// splits a line on tabs, returns how many columns it found
static int SplitDialogColumns(char* szLine, char szColumns[MAX_LISTBOX_COLUMNS + 1][MAX_LISTBOX_TEXT_IN_COLUMN])
{
	int iColumn = 0;
	size_t iOut = 0;

	for (int i = 0; i <= MAX_LISTBOX_COLUMNS; i++)
		szColumns[i][0] = '\0';

	for (size_t i = 0; szLine[i]; i++)
	{
		if (szLine[i] == '\t')
		{
			szColumns[iColumn][iOut] = '\0';
			if (iColumn == MAX_LISTBOX_COLUMNS) break;
			iColumn++;
			iOut = 0;
			continue;
		}

		if (iOut < (MAX_LISTBOX_TEXT_IN_COLUMN - 1))
			szColumns[iColumn][iOut++] = szLine[i];
	}
	szColumns[iColumn][iOut] = '\0';

	return iColumn + 1;
}

//----------------------------------------------------

// fills the list box from the newline separated content and reports back the size
// it wants, the original measures and populates in the same pass. tab separated
// lines become real list box columns, otherwise the trailing ones get clipped
void CDialog::SetupList(char* szContent, SIZE* pSize)
{
	bool bColumns = (m_iDialogStyle == DIALOG_STYLE_TABLIST ||
					 m_iDialogStyle == DIALOG_STYLE_TABLIST_HEADERS);

	m_pListBox->RemoveAllItems();
	m_pListBox->m_nColumns = 0;

	char szLine[256];
	char szColumns[MAX_LISTBOX_COLUMNS + 1][MAX_LISTBOX_TEXT_IN_COLUMN];
	int iColumnWidth[MAX_LISTBOX_COLUMNS + 1] = { 0 };
	int iUsedColumns = 1;

	char* pText = szContent;
	int iMaxWidth = 0;
	int iTotalHeight = 0;
	int iIndex = 0;

	// headers style spends its first line on the column titles
	bool bWantHeader = (m_iDialogStyle == DIALOG_STYLE_TABLIST_HEADERS);

	while (pText && *pText)
	{
		memset(szLine, 0, sizeof(szLine));
		pText = CopyDialogLine(pText, szLine, sizeof(szLine));

		if (strlen(szLine))
		{
			if (bColumns)
			{
				int iCount = SplitDialogColumns(szLine, szColumns);
				if (iCount > iUsedColumns) iUsedColumns = iCount;

				for (int i = 0; i < iCount; i++)
				{
					LONG lWidth = GetTextWidth(szColumns[i]);
					if (lWidth > iColumnWidth[i]) iColumnWidth[i] = (int)lWidth;
				}

				if (bWantHeader)
				{
					// the titles are drawn above the list, not as a row
					memcpy(m_szHeaders, szColumns, sizeof(m_szHeaders));
					m_iHeaderColumns = iCount;
					bWantHeader = false;
					continue;
				}

				iTotalHeight += GetFontHeight();
				m_pListBox->AddItem(szColumns[0], iIndex, (D3DCOLOR)-1);
			}
			else
			{
				LONG lWidth = GetTextWidth(szLine);
				if (lWidth > iMaxWidth) iMaxWidth = (int)lWidth;

				iTotalHeight += GetFontHeight();
				m_pListBox->AddItem(szLine, iIndex, (D3DCOLOR)-1);
			}

			// CDXUTListBox::Render draws the selection sprite unless the item is
			// forced unselected, so without this every row comes out highlighted
			DXUTListBoxItem* pItem = m_pListBox->GetItem(iIndex);
			if (pItem) pItem->bForceUnselected = true;

			if (bColumns)
			{
				for (int i = 1; i < iUsedColumns; i++)
					m_pListBox->AddItemToColumn(iIndex, i - 1, szColumns[i]);
			}

			iIndex++;
		}

		if (!*pText) break;
		if (*pText == '\n') pText++;
	}

	if (bColumns)
	{
		// AddItemToColumn only accepts columns below m_nColumns, so it is set
		// before the widths are handed out
		m_pListBox->m_nColumns = iUsedColumns - 1;

		iMaxWidth = 0;
		for (int i = 0; i < iUsedColumns; i++)
		{
			int iWidth = iColumnWidth[i] + DIALOG_COLUMN_GAP;

			if (i < MAX_LISTBOX_COLUMNS)
				m_pListBox->m_nColumnWidth[i] = iWidth;

			iMaxWidth += iWidth;
			m_iHeaderOffset[i] = (i == 0) ? 0 : m_iHeaderOffset[i - 1] + iColumnWidth[i - 1] + DIALOG_COLUMN_GAP;
		}
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
		m_iHeaderColumns = 0;

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

		// everything stacks from the top margin down, so a row can never land on
		// top of the one before it
		int iRowTop = DIALOG_CONTENT_MARGIN;
		int iEditTop = 0;
		int iListTop = 0;
		int iListHeight = 0;

		switch (m_iDialogStyle)
		{
		case DIALOG_STYLE_MSGBOX:
		{
			m_iWidth = m_ContentSize.cx + (2 * DIALOG_SIDE_MARGIN);
			if (m_iWidth < DIALOG_MIN_WIDTH) m_iWidth = DIALOG_MIN_WIDTH;

			iRowTop += m_ContentSize.cy;
			break;
		}
		case DIALOG_STYLE_INPUT:
		case DIALOG_STYLE_PASSWORD:
		{
			m_iWidth = m_ContentSize.cx + (2 * DIALOG_SIDE_MARGIN);
			if (m_iWidth < DIALOG_MIN_WIDTH) m_iWidth = DIALOG_MIN_WIDTH;

			iRowTop += m_ContentSize.cy + DIALOG_ROW_GAP;

			iEditTop = iRowTop;

			m_pEditBox->SetText("");
			m_pEditBox->SetVisible(true);
			m_pEditBox->SetEnabled(true);

			iRowTop += DIALOG_EDIT_HEIGHT;
			break;
		}
		case DIALOG_STYLE_LIST:
		case DIALOG_STYLE_TABLIST:
		case DIALOG_STYLE_TABLIST_HEADERS:
		{
			SIZE listSize;
			SetupList(m_szContent, &listSize);

			m_iWidth = listSize.cx + (2 * DIALOG_SIDE_MARGIN);
			if (m_iWidth < DIALOG_MIN_WIDTH) m_iWidth = DIALOG_MIN_WIDTH;
			if (m_iWidth > 800) m_iWidth = 800;

			iListHeight = listSize.cy + (2 * (int)GetFontHeight());
			if (iListHeight < 120) iListHeight = 120;
			if (iListHeight > 400) iListHeight = 400;

			// the header row is drawn above the list, so it needs its own space
			if (m_iHeaderColumns)
				iRowTop += (int)GetFontHeight() + 4;

			iListTop = iRowTop;

			m_pListBox->SetVisible(true);
			m_pListBox->SetEnabled(true);

			iRowTop += iListHeight;
			break;
		}
		}

		// the button row closes the box. dxut offsets every control down by the
		// caption height, so the panel has to carry that on top of the stack
		int iButtonTop = iRowTop + DIALOG_ROW_GAP;
		m_iHeight = m_pDialog->GetCaptionHeight() + iButtonTop + m_iButtonHeight +
					DIALOG_CONTENT_MARGIN;

		// two buttons plus the gap between them have to fit
		int iButtonRowWidth = (2 * m_iButtonWidth) + DIALOG_ROW_GAP + (2 * DIALOG_SIDE_MARGIN);
		if (m_iWidth < iButtonRowWidth)
			m_iWidth = iButtonRowWidth;

		// the final width is only known here, so the controls get placed now
		int iInnerWidth = m_iWidth - (2 * DIALOG_SIDE_MARGIN);

		if (iEditTop)
		{
			m_pEditBox->SetLocation(DIALOG_SIDE_MARGIN, iEditTop);
			m_pEditBox->SetSize(iInnerWidth, DIALOG_EDIT_HEIGHT);
		}
		if (iListHeight)
		{
			m_pListBox->SetLocation(DIALOG_SIDE_MARGIN, iListTop);
			m_pListBox->SetSize(iInnerWidth, iListHeight);
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

		// the buttons take the row the stack left for them
		if (pButton1)
			pButton1->SetLocation(DIALOG_SIDE_MARGIN, iButtonTop);
		if (pButton)
			pButton->SetLocation(DIALOG_SIDE_MARGIN + m_iButtonWidth + DIALOG_ROW_GAP,
				iButtonTop);

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
