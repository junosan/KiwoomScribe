
// RealtimeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "Realtime.h"
#include "RealtimeDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CRealtimeDlg dialog

// Static variables
TCHAR CRealtimeDlg::m_sKRXCodes[m_ncKRXCodesBufSize];
unsigned int CRealtimeDlg::m_nKRXCodes;
unsigned int CRealtimeDlg::m_piKRXCodes[m_ncMaxItems];
std::ofstream CRealtimeDlg::m_pStreams[m_ncMaxItems * 2];
CRealtimeDlg *CRealtimeDlg::pCurInstance;


CRealtimeDlg::CRealtimeDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CRealtimeDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CRealtimeDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_KHOPENAPICTRL1, theApp.m_cKHOpenAPI);
	DDX_Control(pDX, IDC_BUTTON_LOGIN, m_btLogin);
	DDX_Control(pDX, IDC_BUTTON_RUN, m_btRun);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_btStop);
	DDX_Control(pDX, IDCANCEL, m_btCancel);
}

BEGIN_MESSAGE_MAP(CRealtimeDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CRealtimeDlg::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_BUTTON_RUN, &CRealtimeDlg::OnBnClickedButtonRun)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CRealtimeDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDCANCEL, &CRealtimeDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CRealtimeDlg message handlers

BOOL CRealtimeDlg::OnInitDialog()
{
	CDialogEx::OnInitDialog();

	// Set the icon for this dialog.  The framework does this automatically
	//  when the application's main window is not a dialog
	SetIcon(m_hIcon, TRUE);			// Set big icon
	SetIcon(m_hIcon, FALSE);		// Set small icon

	// TODO: Add extra initialization here

	pCurInstance = this;

	m_btRun.EnableWindow(FALSE);
	m_btStop.EnableWindow(FALSE);

	// Parse KRX code list and count # of items
	::GetPrivateProfileString(_T("REALTIME"), _T("KRX_CODE"), _T("000660"), m_sKRXCodes, m_ncKRXCodesBufSize, theApp.m_sAppPath + _T("\\Data\\config.ini"));

	TCHAR *psSemi = m_sKRXCodes;
	m_nKRXCodes = 1;
	while ((psSemi = wcschr(psSemi, ';')) != NULL)
	{
		psSemi++;
		m_nKRXCodes++;
	}

	// Read and display KRX codes
	TCHAR sTemp[7];
	const unsigned int bufSize = 1024;
	char pcDispBuf[bufSize], pcTemp[10];
	TCHAR sDisp[bufSize];
	CString sDirBase = theApp.m_sAppPath + "\\Data\\";

	sprintf_s(pcDispBuf, "Data\\config.ini\n");
	for (unsigned int i = 0; i < m_nKRXCodes; i++)
	{
		wcsncpy_s(sTemp, m_sKRXCodes + i * 7, 6);
		m_piKRXCodes[i] = _wtoi(sTemp);

		// Create folder for each item (if not present)
		if (!::PathIsDirectory(sDirBase + sTemp))
			::CreateDirectory(sDirBase + sTemp, NULL);

		sprintf_s(pcTemp, "%06d   ", m_piKRXCodes[i]);
		strcat_s(pcDispBuf, pcTemp);
		if ((i % 5) == 4)
			strcat_s(pcDispBuf, "\n");
	}

	MultiByteToWideChar(CP_ACP, 0, pcDispBuf, bufSize, sDisp, bufSize);
	((CWnd*)GetDlgItem(IDC_STATIC_DISP))->SetWindowText(sDisp);

	// Used for API communication
	m_sScrNo = _T("9999");

	// As this is different from the current date,
	// initial call to event function will trigger ofstream assignments
	m_sLastDate = _T("00000000");

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CRealtimeDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // device context for painting

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// Center icon in client rectangle
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// Draw the icon
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialogEx::OnPaint();
	}
}

// The system calls this function to obtain the cursor to display while the user drags
//  the minimized window.
HCURSOR CRealtimeDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CRealtimeDlg::OnBnClickedButtonLogin()
{
	// TODO: Add your control notification handler code here

	if (theApp.m_cKHOpenAPI.CommConnect() < 0)
	{
		AfxMessageBox(_T("KHOpenAPI.CommConnect: Login failed."));
	}

}

BEGIN_EVENTSINK_MAP(CRealtimeDlg, CDialogEx)
	ON_EVENT(CRealtimeDlg, IDC_KHOPENAPICTRL1, 5, CRealtimeDlg::OnEventConnect, VTS_I4)
	ON_EVENT(CRealtimeDlg, IDC_KHOPENAPICTRL1, 2, CRealtimeDlg::OnReceiveRealData, VTS_BSTR VTS_BSTR VTS_BSTR)
END_EVENTSINK_MAP()


void CRealtimeDlg::OnEventConnect(long nErrCode)
{
	// TODO: Add your message handler code here
	
	TCHAR str[64];

	if (nErrCode < 0)
	{
		swprintf_s(str, 64, _T("KHOpenAPI.OnEventConnect: Error %d"), nErrCode);
		AfxMessageBox(str);
	}
	else
	{
		((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Login successful."));

		m_btLogin.EnableWindow(FALSE);
		m_btRun.EnableWindow(TRUE);
	}

}


void CRealtimeDlg::OnBnClickedButtonRun()
{
	// TODO: Add your control notification handler code here

	long ret;
	const unsigned int bufSize = 64;
	TCHAR sDisp[bufSize];


	// Set up real time monitor API
	if ((ret = theApp.m_cKHOpenAPI.SetRealReg(m_sScrNo, m_sKRXCodes, _T("10;15;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80"), _T("0"))) < 0)
	{
		swprintf_s(sDisp, bufSize, _T("KHOpenAPI.SetRealReg: Error #%d"), ret);
		AfxMessageBox(sDisp);
	}
	else
	{
		m_btRun.EnableWindow(FALSE);
		m_btStop.EnableWindow(TRUE);
		m_btCancel.EnableWindow(FALSE);

		((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Started real-time data dump."));
	}

}


void CRealtimeDlg::OnReceiveRealData(LPCTSTR sRealKey, LPCTSTR sRealType, LPCTSTR sRealData)
{
	// TODO: Add your message handler code here

	const unsigned int bufSize = 1024;
	char pcTemp[bufSize], pcWriteBuf[bufSize];
	int iKRXCode, iC;

	// (Re)assign files to streams -- on first run or if date changed
	CTime t = CTime::GetCurrentTime();
	CString sTemp = t.Format("%Y%m%d");
	if (sTemp.Compare(m_sLastDate))
	{
		m_sLastDate = sTemp;

		for (unsigned int i = 0; i < m_nKRXCodes; i++)
		{
			// Stream for trade updates
			if (m_pStreams[i].is_open())
				m_pStreams[i].close();

			sTemp.Format(_T("%06d"), m_piKRXCodes[i]);
			sTemp = theApp.m_sAppPath + CString("\\Data\\") + sTemp + CString("\\") + m_sLastDate + CString(".txt");
			m_pStreams[i].open(sTemp.GetBuffer(), std::ofstream::app);

			// Stream for price table updates
			if (m_pStreams[i + m_ncMaxItems].is_open())
				m_pStreams[i + m_ncMaxItems].close();

			sTemp.Format(_T("%06d"), m_piKRXCodes[i]);
			sTemp = theApp.m_sAppPath + CString("\\Data\\") + sTemp + CString("\\") + m_sLastDate + CString("t.txt");
			m_pStreams[i + m_ncMaxItems].open(sTemp.GetBuffer(), std::ofstream::app);
		}
	}


	// sRealType
	// addr   4 5 6 7
	// 호가 0xC8A3B0A1 ("주식호가")
	// 시간 0xBDC3B0A3 ("주식시간외호가")
	// 체결 0xC3BCB0E1 ("주식체결")

	// KRX code
	iKRXCode = _ttoi(sRealKey);
#ifdef DISP_VERBOSE
	TCHAR sDisp[bufSize];
	char pcDispBuf[bufSize];
	sprintf_s(pcDispBuf, "Code\t\t%06d\n", iKRXCode);
#endif

	// Price table update
	if (((sRealType[4] == 0xC8) && (sRealType[5] == 0xA3) && (sRealType[6] == 0xB0) && (sRealType[7] == 0xA1)) ||
		((sRealType[4] == 0xBD) && (sRealType[5] == 0xC3) && (sRealType[6] == 0xB0) && (sRealType[7] == 0xA3)))
	{
		// Time
		iC = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 21));
#ifdef DISP_VERBOSE
		sprintf_s(pcTemp, "Time\t\t%06d\n", iC);
		strcat_s(pcDispBuf, pcTemp);
#endif
		sprintf_s(pcWriteBuf, "%06d\t", iC);

		// Sell price 10 ~ 1 (high->low)
		for (unsigned int i = 10; i >= 1; i--)
		{
			iC = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 40 + i)));
#ifdef DISP_VERBOSE
			sprintf_s(pcTemp, "Sell price %d\t%d\t", i, iC);
			strcat_s(pcDispBuf, pcTemp);
#endif
			sprintf_s(pcTemp, "%d\t", iC);
			strcat_s(pcWriteBuf, pcTemp);

			iC = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 60 + i)));
#ifdef DISP_VERBOSE
			sprintf_s(pcTemp, "%d\n", iC);
			strcat_s(pcDispBuf, pcTemp);
#endif
			sprintf_s(pcTemp, "%d\t", iC);
			strcat_s(pcWriteBuf, pcTemp);
		}

		// Buy price 1 ~ 10 (high->low)
		for (unsigned int i = 1; i <= 10; i++)
		{
			iC = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 50 + i)));
#ifdef DISP_VERBOSE
			sprintf_s(pcTemp, "Buy price %d\t%d\t", i, iC);
			strcat_s(pcDispBuf, pcTemp);
#endif
			sprintf_s(pcTemp, "%d\t", iC);
			strcat_s(pcWriteBuf, pcTemp);

			iC = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 70 + i)));
#ifdef DISP_VERBOSE
			sprintf_s(pcTemp, "%d\n", iC);
			strcat_s(pcDispBuf, pcTemp);
#endif
			if (i != 10)
				sprintf_s(pcTemp, "%d\t", iC);
			else
				sprintf_s(pcTemp, "%d\n", iC);
			strcat_s(pcWriteBuf, pcTemp);
		}

#ifdef DISP_VERBOSE
		MultiByteToWideChar(CP_ACP, 0, pcDispBuf, bufSize, sDisp, bufSize);
		((CWnd*)GetDlgItem(IDC_STATIC_DISP))->SetWindowText(sDisp);
#endif

		m_pStreams[KRXCodeToInd(iKRXCode) + m_ncMaxItems].write(pcWriteBuf, strlen(pcWriteBuf));
	}
	// Trade update
	else if ((sRealType[4] == 0xC3) && (sRealType[5] == 0xBC) && (sRealType[6] == 0xB0) && (sRealType[7] == 0xE1))
	{
		// Time
		iC = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 20));
#ifdef DISP_VERBOSE
		sprintf_s(pcTemp, "Time\t\t%06d\n", iC);
		strcat_s(pcDispBuf, pcTemp);
#endif
		sprintf_s(pcWriteBuf, "%06d\t", iC);

		// Quantity
		iC = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 15)));
#ifdef DISP_VERBOSE
		sprintf_s(pcTemp, "Trade quant\t%d\n", iC);
		strcat_s(pcDispBuf, pcTemp);
#endif
		sprintf_s(pcTemp, "%d\t", iC);
		strcat_s(pcWriteBuf, pcTemp);

		// Trade price
		iC = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 10)));
#ifdef DISP_VERBOSE
		sprintf_s(pcTemp, "Trade price\t%d\n", iC);
		strcat_s(pcDispBuf, pcTemp);
#endif
		sprintf_s(pcTemp, "%d\t", iC);
		strcat_s(pcWriteBuf, pcTemp);

		// (quantization step) == (sell price 1) - (buy price 1)
		// Sell price 1
		iC = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 27)));
#ifdef DISP_VERBOSE
		sprintf_s(pcTemp, "Sell price 1\t%d\n", iC);
		strcat_s(pcDispBuf, pcTemp);
#endif
		sprintf_s(pcTemp, "%d\t", iC);
		strcat_s(pcWriteBuf, pcTemp);

		// Buy price 1
		iC = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 28)));
#ifdef DISP_VERBOSE
		sprintf_s(pcTemp, "Buy price 1\t%d\n", iC);
		strcat_s(pcDispBuf, pcTemp);
#endif
		sprintf_s(pcTemp, "%d\n", iC);
		strcat_s(pcWriteBuf, pcTemp);

#ifdef DISP_VERBOSE
		MultiByteToWideChar(CP_ACP, 0, pcDispBuf, bufSize, sDisp, bufSize);
		((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(sDisp);
#endif

		m_pStreams[KRXCodeToInd(iKRXCode)].write(pcWriteBuf, strlen(pcWriteBuf));
	}
	
	// Timer for auto flush when events stop arriving for more than 3 seconds
	SetTimer(1, 3000, TimerFlushCallback);
}

void CRealtimeDlg::OnBnClickedButtonStop()
{
	// TODO: Add your control notification handler code here

	theApp.m_cKHOpenAPI.SetRealRemove(m_sScrNo, _T("ALL"));

	m_btStop.EnableWindow(FALSE);

	((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Waiting for remaining events."));

	// Timer for auto flush when events stop arriving for more than 3 seconds
	SetTimer(1, 3000, TimerFlushCallback);
}

void CRealtimeDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here

	CDialogEx::OnCancel();

	for (unsigned int i = 0; i < m_nKRXCodes; i++)
	{
		if (m_pStreams[i].is_open())
		{
			m_pStreams[i].close();
			m_pStreams[i + m_ncMaxItems].close();
		}
	}

	// This very often causes infinite loop in the background after app closes
	// theApp.m_cKHOpenAPI.CommTerminate();
	// PostQuitMessage(0);

	// Force exit without invoking theApp.m_cKHOpenAPI.CommTerminate() in destructors
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	abort();
}

VOID CALLBACK CRealtimeDlg::TimerFlushCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	for (unsigned int i = 0; i < m_nKRXCodes; i++)
		if (m_pStreams[i].is_open())
			m_pStreams[i].flush();

	((CWnd*)(pCurInstance->GetDlgItem(IDC_STATIC_DISP2)))->SetWindowText(_T("Flushed ofstream."));

	pCurInstance->KillTimer(1);
	
	if (pCurInstance->m_btStop.IsWindowEnabled() == FALSE)
	{
		pCurInstance->m_btRun.EnableWindow(TRUE);
		pCurInstance->m_btCancel.EnableWindow(TRUE);
	}

}