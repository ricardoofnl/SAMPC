//
// Version: $Id: label.cpp,v 1.2 2006/03/20 17:44:19 kyeman Exp $
//

#include "main.h"

extern D3DXMATRIX matView, matProj;

CLabel::CLabel(IDirect3DDevice9* pDevice)
{
	m_pDevice = pDevice;
	D3DXCreateSprite(pDevice, &m_pSprite);
}

CLabel::~CLabel()
{
	SAFE_RELEASE(m_pSprite);
}

void CLabel::Begin()
{
	if (m_pSprite) m_pSprite->Begin(D3DXSPRITE_ALPHABLEND);
}

void CLabel::End()
{
	if (m_pSprite) m_pSprite->End();
}

int CLabel::IsLineOfSightClear(float fX, float fY, float fZ)
{
	CAMERA_AIM* pAimCam = GameGetInternalAim();

	return (int)ScriptCommand(&get_line_of_sight, fX, fY, fZ,
		pAimCam->pos1x, pAimCam->pos1y, pAimCam->pos1z, 1, 0, 0, 1, 0);
}

void CLabel::Draw(D3DXVECTOR3* ppos, char* szText, DWORD dwColor, bool bShadowed, bool bDoLOS)
{
	if (!m_pDevice || (bDoLOS && !IsLineOfSightClear(ppos->x, ppos->y, ppos->z)))
		return;

	D3DVIEWPORT9 Viewport;
	m_pDevice->GetViewport(&Viewport);

	D3DXVECTOR3 Out;
	D3DXMATRIX matIdent;
	D3DXMatrixIdentity(&matIdent);
	D3DXVec3Project(&Out, ppos, &Viewport, &matProj, &matView, &matIdent);

	if (Out.z > 1.0f)
		return;

	RECT rect = {(int)Out.x, (int)Out.y, (int)Out.x+1, (int)Out.y+1};
	pDefaultFont->RenderSmallerText(m_pSprite, szText, rect, DT_NOCLIP | DT_CENTER, dwColor, bShadowed);
}

void CLabel::DeleteDeviceObjects()
{
	if (m_pSprite) m_pSprite->OnLostDevice();
}

void CLabel::RestoreDeviceObjects()
{
	if (m_pSprite) m_pSprite->OnResetDevice();
}
