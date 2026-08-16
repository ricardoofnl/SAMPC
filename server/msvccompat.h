//----------------------------------------------------------
//
// SA:MP Multiplayer Modification For GTA:SA
//
// shims for the MSVC only functions the server sources use, so the
// linux build does not need every call site rewritten
//
//----------------------------------------------------------

#pragma once

#ifndef WIN32

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <math.h>
#include <alloca.h>
#include <stddef.h>

#ifndef RSIZE_MAX
	#define RSIZE_MAX ((size_t)-1 >> 1)
#endif

#define _alloca alloca

#define _copysign  copysign
#define _copysignf copysignf

#define _stricmp  strcasecmp
#define _strnicmp strncasecmp

// MSVC returns 0 on success, callers here ignore it but keep the convention

inline int strcpy_s(char* pDest, size_t nDestSize, const char* pSrc)
{
	if (!pDest || !nDestSize) return 22; // EINVAL
	if (!pSrc) { pDest[0] = '\0'; return 22; }

	size_t nLen = strlen(pSrc);
	if (nLen >= nDestSize) nLen = nDestSize - 1;

	memcpy(pDest, pSrc, nLen);
	pDest[nLen] = '\0';
	return 0;
}

inline int strcat_s(char* pDest, size_t nDestSize, const char* pSrc)
{
	if (!pDest || !nDestSize) return 22;

	size_t nUsed = strnlen(pDest, nDestSize);
	if (nUsed >= nDestSize) return 34; // ERANGE, not terminated
	return strcpy_s(pDest + nUsed, nDestSize - nUsed, pSrc);
}

inline int strncpy_s(char* pDest, size_t nDestSize, const char* pSrc, size_t nCount)
{
	if (!pDest || !nDestSize) return 22;
	if (!pSrc) { pDest[0] = '\0'; return 22; }

	size_t nLen = strnlen(pSrc, nCount);
	if (nLen >= nDestSize) nLen = nDestSize - 1;

	memcpy(pDest, pSrc, nLen);
	pDest[nLen] = '\0';
	return 0;
}

#define strtok_s strtok_r

// array forms, MSVC deduces the destination size and most call sites rely on it

template <size_t N> inline int strcpy_s(char (&szDest)[N], const char* pSrc)
{
	return strcpy_s(szDest, N, pSrc);
}

template <size_t N> inline int strcat_s(char (&szDest)[N], const char* pSrc)
{
	return strcat_s(szDest, N, pSrc);
}

template <size_t N> inline int strncpy_s(char (&szDest)[N], const char* pSrc, size_t nCount)
{
	return strncpy_s(szDest, N, pSrc, nCount);
}

// returns the number of characters written, or -1 on error

inline int sprintf_s(char* pDest, size_t nDestSize, const char* pFormat, ...)
{
	va_list args;
	va_start(args, pFormat);
	int iRet = vsnprintf(pDest, nDestSize, pFormat, args);
	va_end(args);

	if (iRet < 0 || (size_t)iRet >= nDestSize) return -1;
	return iRet;
}

template <size_t N> inline int sprintf_s(char (&szDest)[N], const char* pFormat, ...)
{
	va_list args;
	va_start(args, pFormat);
	int iRet = vsnprintf(szDest, N, pFormat, args);
	va_end(args);

	if (iRet < 0 || (size_t)iRet >= N) return -1;
	return iRet;
}

#endif // !WIN32