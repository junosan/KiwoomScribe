/*
   Copyright 2015 Hosang Yoon

   Licensed under the Apache License, Version 2.0 (the "License");
   you may not use this file except in compliance with the License.
   You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

   Unless required by applicable law or agreed to in writing, software
   distributed under the License is distributed on an "AS IS" BASIS,
   WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
   See the License for the specific language governing permissions and
   limitations under the License.
*/

// FetchCode.cpp : Defines the entry point for the console application.
//

#include "stdafx.h"
#include "FetchCode.h"

#include <iostream>
#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// The one and only application object

CWinApp theApp;

using namespace std;

int _tmain(int argc, TCHAR* argv[], TCHAR* envp[])
{
	HMODULE hModule = ::GetModuleHandle(NULL);

	if (hModule == NULL)
	{
		_tprintf(_T("Fatal Error: GetModuleHandle failed\n"));
		return 1;
	}

	if (!AfxWinInit(hModule, NULL, ::GetCommandLine(), 0))
	{
		_tprintf(_T("Fatal Error: MFC initialization failed\n"));
		return 1;
	}

	const size_t szBuf = (1 << 16);
	TCHAR pcBuf[szBuf];

	GetModuleFileName(AfxGetInstanceHandle(), pcBuf, szBuf);
	*strrchr(pcBuf, '\\') = '\0';
	CString sAppPath = pcBuf;

	CString sELWCodes, sETFCodes;
	CString sCmd;
	
	int nELW = 0;
	for (int iPg = 1; iPg <= 4; iPg++)
	{
		sCmd.Format(_T("%s\\curl.exe \"http://paxnet.moneta.co.kr/stock/elw/topVolElw.jsp?pg=%d\" -o %s\\curl.html -s"), sAppPath, iPg, sAppPath);
		system(sCmd);

		FILE *pf;
		_tfopen_s(&pf, sAppPath + CString(_T("\\curl.html")), _T("rt"));
		if (pf == NULL) continue; 
		
		TCHAR *pcLine;
		while (NULL != (pcLine = fgets(pcBuf, szBuf, pf)))
		{
			CString sLine(pcLine);
			int idx_0 = sLine.Find(_T("<tr class=\"table_gray\">"));
			int idx_1 = sLine.Find(_T("<tr class=\"table_darkgray\">"));
			if ((idx_0 == -1) && (idx_1 == -1))
				continue;

			int idx_o;
			while (-1 != (idx_o = sLine.Find(_T('<'))))
			{
				int idx_c = sLine.Find(_T('>'));
				if (-1 == idx_c)
					idx_c = sLine.GetLength() - 1;
				sLine.Delete(idx_o, idx_c - idx_o + 1);
				sLine.Insert(idx_o, _T(";"));
			}

			CString sItem;
			CString sTk;
			int iTk = 0;
			while (AfxExtractSubString(sTk, sLine, iTk, _T(';')))
			{
				if ((iTk == 5) || (iTk == 8) || (iTk == 33))
				{
					if (sTk.IsEmpty())
					{
						iTk = -1;
						break;
					}
					sItem.Append(sTk.Trim());
					if ((iTk == 5) || (iTk == 8))
						sItem.Append(_T("_"));
					else
					{
						sItem.Append(_T(";"));
						break;
					}
				}
				iTk++;
			}

			if (iTk == 33 && sItem.Find(_T("KOSPI200")) != -1)
			{
				sELWCodes.Append(sItem);
				std::cout << sItem << '\n';
				++nELW;
			}
		}

		fclose(pf);
	}
	sELWCodes.Delete(sELWCodes.GetLength() - 1);

	int nETF = 0;
	for (int iPg = 1; iPg <= 2; iPg++)
	{
		sCmd.Format(_T("%s\\curl.exe \"http://finance.daum.net/quote/etf.daum?col=volume&order=desc&page=%d\" -o %s\\curl.html -s"), sAppPath, iPg, sAppPath);
		system(sCmd);

		FILE *pf;
		_tfopen_s(&pf, sAppPath + CString(_T("\\curl.html")), _T("rt"));
		if (pf == NULL) continue; 
		
		TCHAR *pcLine;
		while (NULL != (pcLine = fgets(pcBuf, szBuf, pf)))
		{
			CString sLine(pcLine);
			CString sMatch(_T("<td class=\"txt\"><a href=\"/item/main.daum?code=A"));
			int idx_beg = sLine.Find(sMatch);
			if (-1 == idx_beg) continue;

			CString sItem = sLine.Mid(idx_beg + sMatch.GetLength(), 6) + CString(_T(";"));

			sETFCodes.Append(sItem);
			std::cout << sItem << '\n';
			if (50 == ++nETF) break;
		}

		fclose(pf);
	}
	sETFCodes.Delete(sETFCodes.GetLength() - 1);

	std::cout << "Retrieved " << nELW << " ELW and " << nETF << " ETF codes\n";

	::WritePrivateProfileStringA(_T("SAVEDATA"), _T("ELW_CODE"), sELWCodes, sAppPath + CString(_T("\\..\\config.ini")));
	::WritePrivateProfileStringA(_T("SAVEDATA"), _T("ETF_CODE"), sETFCodes, sAppPath + CString(_T("\\..\\config.ini")));

	getc(stdin);

	return 0;
}
