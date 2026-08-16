
#include "main.h"

#ifdef WIN32
static DWORD WINAPI HTTPThread(LPVOID pArg)
#else
static void* HTTPThread(void* pArg)
#endif
{
	HTTP_ENTRY* pEntry = (HTTP_ENTRY*)pArg;
	char* szURL, * szData;

	if (pEntry && pEntry->GetHttpClient())
	{
		//if (pNetGame)
			//pEntry->StartTime = pNetGame->GetRakTime();

		pEntry->iState = 2;

		szURL = (char*)pEntry->url.c_str();
		szData = (char*)pEntry->data.c_str();

		pEntry->GetHttpClient()->ProcessURL(pEntry->iType, szURL, szData, "sa-mp.com");
		
		pEntry->iState = 3;
		//pEntry->dwUnk = 0;
	}
	return 0;
}

bool StartThread(void* pParams)
{
#ifdef WIN32
	HANDLE hThread;

	hThread = CreateThread(NULL, 0, HTTPThread, pParams, 0, NULL);
	if (hThread && hThread != (HANDLE)-1)
	{
		CloseHandle(hThread);
		return true;
	}
	return false;
#else
	pthread_attr_t attr;
	pthread_t thread;

	pthread_attr_init(&attr);
	pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
	return pthread_create(&thread, &attr, HTTPThread, pParams) == 0;
#endif
}

CThreadedHttp::CThreadedHttp()
{
	memset(m_pEntries, 0, sizeof(m_pEntries));
}

void CThreadedHttp::Process()
{
	HTTP_ENTRY* pEntry;
	int idx, response_code;
	cell* phys_addr, amx_addr;

	for (int i = 0; i < MAX_SCRIPT_HTTP_ENTRIES; i++)
	{
		pEntry = m_pEntries[i];

		if (pEntry && pEntry->GetHttpClient() && pEntry->iState == 3)
		{
			if (!pEntry->bQuietMode && pEntry->pAMX)
			{
				if (amx_FindPublic(pEntry->pAMX, pEntry->szCallback, &idx) == AMX_ERR_NONE)
				{
					if (pEntry->GetHttpClient()->GetErrorCode() != HTTP_SUCCESS)
					{
						amx_PushString(pEntry->pAMX, &amx_addr, &phys_addr, "", 0, 0);
						response_code = pEntry->GetHttpClient()->GetErrorCode();
					}
					else
					{
						amx_PushString(pEntry->pAMX, &amx_addr, &phys_addr, pEntry->GetHttpClient()->GetDocument(), 0, 0);
						response_code = pEntry->GetHttpClient()->GetResponseCode();
					}
					amx_Push(pEntry->pAMX, response_code);
					amx_Push(pEntry->pAMX, pEntry->iIndex);
					amx_Exec(pEntry->pAMX, NULL, idx);
					amx_Release(pEntry->pAMX, amx_addr);
				}
			}

			SAFE_DELETE(m_pEntries[i])
		}
	}
}

bool CThreadedHttp::AddEntry(int iIndex, int iType, char* szUrl, char* szData,
	char* szBindAddress, bool bQuietMode, AMX* pAMX, char* szCallback)
{
	int i;

	for (i = 0; i < MAX_SCRIPT_HTTP_ENTRIES; i++)
	{
		if (m_pEntries[i] == NULL)
			break;
	}
	if (i == MAX_SCRIPT_HTTP_ENTRIES)
	{
		return false;
	}

	m_pEntries[i] = new HTTP_ENTRY(szBindAddress);
	if (m_pEntries[i])
	{
		m_pEntries[i]->iState = 1;
		m_pEntries[i]->iIndex = iIndex;
		m_pEntries[i]->iType = iType;
		if (szUrl && szUrl[0] != '\0') // && strlen(szUrl))
		{
			m_pEntries[i]->url.assign(szUrl, strlen(szUrl));

			if (szData)
				m_pEntries[i]->data.assign(szData, strlen(szData));

			m_pEntries[i]->bQuietMode = bQuietMode;
			m_pEntries[i]->pAMX = pAMX;
			if (szCallback)
				strcpy(m_pEntries[i]->szCallback, szCallback);
			else
				m_pEntries[i]->szCallback[0] = '\0';

			StartThread(m_pEntries[i]);
			return true;
		}
	}
	return false;
}
