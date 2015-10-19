
// RealtimeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "SaveData.h"
#include "SaveDataDlg.h"
#include "afxdialogex.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// CSaveDataDlg dialog

// Static variables
TCHAR CSaveDataDlg::m_sKRXCodes_0[m_ncKRXCodesBufSize];
TCHAR CSaveDataDlg::m_sKRXCodes_1[m_ncKRXCodesBufSize];
unsigned int CSaveDataDlg::m_nKRXCodes;
unsigned int CSaveDataDlg::m_piKRXCodes[m_ncMaxItems];
std::ofstream CSaveDataDlg::m_pStreams[m_ncMaxItems * 2];
CSaveDataDlg *CSaveDataDlg::pCurInstance;


CSaveDataDlg::CSaveDataDlg(CWnd* pParent /*=NULL*/)
	: CDialogEx(CSaveDataDlg::IDD, pParent)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CSaveDataDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialogEx::DoDataExchange(pDX);
	DDX_Control(pDX, IDC_KHOPENAPICTRL1, theApp.m_cKHOpenAPI);
	DDX_Control(pDX, IDC_BUTTON_LOGIN, m_btLogin);
	DDX_Control(pDX, IDC_BUTTON_RUN, m_btRun);
	DDX_Control(pDX, IDC_BUTTON_STOP, m_btStop);
	DDX_Control(pDX, IDCANCEL, m_btCancel);
}

BEGIN_MESSAGE_MAP(CSaveDataDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CSaveDataDlg::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_BUTTON_RUN, &CSaveDataDlg::OnBnClickedButtonRun)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CSaveDataDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDCANCEL, &CSaveDataDlg::OnBnClickedCancel)
END_MESSAGE_MAP()


// CSaveDataDlg message handlers

BOOL CSaveDataDlg::OnInitDialog()
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

	TCHAR sTemp[m_ncKRXCodesBufSize];

	// Parse KRX code list and count # of items
	m_nKRXCodes = 0;
	sTemp[0] = _T('\0');
	CString sItem;
	int i;
	::GetPrivateProfileString(_T("SAVEDATA"), _T("KRX_CODE_0"), _T("000660"), m_sKRXCodes_0, m_ncKRXCodesBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	i = 0;
	while (AfxExtractSubString(sItem, m_sKRXCodes_0, i++, _T(';')))
	{
		int iCode = _ttoi(sItem);
		unsigned int iC;
		for (iC = 0; iC < m_nKRXCodes; iC++)
			if (iCode == m_piKRXCodes[iC])
				break;
		if (iC == m_nKRXCodes) // Avoid duplicate
		{
			m_piKRXCodes[m_nKRXCodes++] = iCode;
			strcat_s (sTemp, sItem);
			strcat_s (sTemp, _T(" "));
			if ((i % 10) == 0)
				strcat_s(sTemp, "\n");
		}
	}

	::GetPrivateProfileString(_T("SAVEDATA"), _T("KRX_CODE_1"), _T("000660"), m_sKRXCodes_1, m_ncKRXCodesBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	i = 0;
	while (AfxExtractSubString(sItem, m_sKRXCodes_1, i++, _T(';')))
	{
		int iCode = _ttoi(sItem);
		unsigned int iC;
		for (iC = 0; iC < m_nKRXCodes; iC++)
			if (iCode == m_piKRXCodes[iC])
				break;
		if (iC == m_nKRXCodes) // Avoid duplicate
		{
			m_piKRXCodes[m_nKRXCodes++] = iCode;
			strcat_s (sTemp, sItem);
			strcat_s (sTemp, _T(" "));
			if ((i % 10) == 0)
				strcat_s(sTemp, "\n");
		}
	}

	((CWnd*)GetDlgItem(IDC_STATIC_DISP))->SetWindowText(sTemp);
	DisplayUpdatedTime();

	// Save Data path
	::GetPrivateProfileString(_T("SAVEDATA"), _T("DATA_DIR"), _T("SaveData\\Data"), sTemp, m_ncKRXCodesBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	m_sDataPath = theApp.m_sAppPath + _T("\\") + sTemp;

	::GetPrivateProfileString(_T("SAVEDATA"), _T("POS"), _T("100 100"), sTemp, m_ncKRXCodesBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	int x, y;
	if (2 == sscanf_s(sTemp, _T("%d %d"), &x, &y))
		SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	// Used for API communication
	m_sScrNo_0 = _T("9999");
	m_sScrNo_1 = _T("9998");

	// This will be properly initialized on the first 'Run'
	m_sLastDate = _T("00000000");

	return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CSaveDataDlg::OnPaint()
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
HCURSOR CSaveDataDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}



void CSaveDataDlg::OnBnClickedButtonLogin()
{
	// TODO: Add your control notification handler code here

	if (theApp.m_cKHOpenAPI.CommConnect() < 0)
	{
		AfxMessageBox(_T("KHOpenAPI.CommConnect: Login failed"));
	}

}

BEGIN_EVENTSINK_MAP(CSaveDataDlg, CDialogEx)
	ON_EVENT(CSaveDataDlg, IDC_KHOPENAPICTRL1, 5, CSaveDataDlg::OnEventConnect, VTS_I4)
	ON_EVENT(CSaveDataDlg, IDC_KHOPENAPICTRL1, 2, CSaveDataDlg::OnReceiveRealData, VTS_BSTR VTS_BSTR VTS_BSTR)
END_EVENTSINK_MAP()


void CSaveDataDlg::OnEventConnect(long nErrCode)
{
	// TODO: Add your message handler code here
	
	TCHAR str[1024];

	if (nErrCode < 0)
	{
		sprintf_s(str, 1024, _T("KHOpenAPI.OnEventConnect: Error %d"), nErrCode);
		AfxMessageBox(str);
	}
	else
	{
		((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Login successful"));
		DisplayUpdatedTime();

		m_btLogin.EnableWindow(FALSE);
		m_btRun.EnableWindow(TRUE);
	}

}


void CSaveDataDlg::OnBnClickedButtonRun()
{
	// TODO: Add your control notification handler code here

	long ret;
	const unsigned int bufSize = 1024;
	TCHAR sDisp[bufSize];
	bool bSuccess = true;

	// Set up real time monitor API (KRX codes & FIDs)
	if (bSuccess && ((ret = theApp.m_cKHOpenAPI.SetRealReg(m_sScrNo_0, m_sKRXCodes_0, _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80"), _T("0"))) < 0))
		bSuccess = false;

	if (bSuccess && ((ret = theApp.m_cKHOpenAPI.SetRealReg(m_sScrNo_1, m_sKRXCodes_1, _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80"), _T("0"))) < 0))
		bSuccess = false;

	if (bSuccess)
	{
		m_btRun.EnableWindow(FALSE);
		m_btStop.EnableWindow(TRUE);
		m_btCancel.EnableWindow(FALSE);

		((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Started realtime data dump"));
		DisplayUpdatedTime();

		// Assign files to streams
		CString sDir, sKRXCode, sFileName;
		CTime t = CTime::GetCurrentTime();
		sDir = t.Format("%Y%m%d");

		if (sDir.Compare(m_sLastDate)) // Prevent opening files multiple times during the same day
		{
			m_sLastDate = sDir;

			// Create folder for today
			sDir = m_sDataPath + _T("\\") + m_sLastDate;
			if (!::PathIsDirectory(sDir))
				::CreateDirectory(sDir, NULL);

			for (unsigned int i = 0; i < m_nKRXCodes; i++)
			{
				sKRXCode.Format(_T("%06d"), m_piKRXCodes[i]);

				// Stream for trade updates
				if (m_pStreams[i].is_open())
					m_pStreams[i].close();

				sFileName = sDir + _T("\\") + sKRXCode + CString(".txt");
				m_pStreams[i].open(sFileName.GetBuffer(), std::ofstream::app);

				// Stream for price table updates
				if (m_pStreams[i + m_ncMaxItems].is_open())
					m_pStreams[i + m_ncMaxItems].close();

				sFileName = sDir + _T("\\") + sKRXCode + CString("t.txt");
				m_pStreams[i + m_ncMaxItems].open(sFileName.GetBuffer(), std::ofstream::app);
			}
		}
	}
	else
	{
		sprintf_s(sDisp, bufSize, _T("KHOpenAPI.SetRealReg: Error #%d"), ret);
		AfxMessageBox(sDisp);
	}

}


void CSaveDataDlg::OnReceiveRealData(LPCTSTR sRealKey, LPCTSTR _sRealType, LPCTSTR sRealData)
{
	// TODO: Add your message handler code here

	CString sRealType (_sRealType);
	const unsigned int bufSize = 1024;
	char pcWriteBuf[bufSize];
	int iKRXCode, data[41], ind;
	
	bool bRelevant = false;

	// KRX code
	iKRXCode = _ttoi(sRealKey);

	if ((ind = KRXCodeToInd(iKRXCode)) != -1)
	{
		if (_T("주식체결") == sRealType)
		{
			// Time
			data[0] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 20));
			// Trade quantity
			data[1] = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 15)));
			// Trade price
			data[2] = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 10)));
			// Sell price 1
			data[3] = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 27)));
			// Buy price 1
			data[4] = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 28)));

			sprintf_s(pcWriteBuf, "%06d\t%d\t%d\t%d\t%d\n", data[0], data[1], data[2], data[3], data[4]);

			m_pStreams[ind].write(pcWriteBuf, strlen(pcWriteBuf));

			bRelevant = true;
		}

		if (_T("주식호가잔량") == sRealType)
		{
			// Time
			data[0] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 21));

			// Price table (price, quantity) (high->low)
			for (unsigned int i = 0; i < 10; i++)
			{
				// Sell price 10 ~ 1 FID(50->41, 70->61)
				data[1  + (i << 1)] = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 50 - i)));
				data[2  + (i << 1)] = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 70 - i)));
				
				// Buy price 1 ~ 10 FID(51->60, 71->80)
				data[21 + (i << 1)] = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 51 + i)));
				data[22 + (i << 1)] = abs(_ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 71 + i)));
			}

			sprintf_s(pcWriteBuf, "%06d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
										"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
										"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
										"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",\
				data[0], data[ 1], data[ 2], data[ 3], data[ 4], data[ 5], data[ 6], data[ 7], data[ 8], data[ 9], data[10],\
						 data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20],\
						 data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30],\
						 data[31], data[32], data[33], data[34], data[35], data[36], data[37], data[38], data[39], data[40]);

			m_pStreams[ind + m_ncMaxItems].write(pcWriteBuf, strlen(pcWriteBuf));

			bRelevant = true;
		}
	}
	
	if (bRelevant)
	{
		// Display updated time every second while events are arriving
		CTime t = CTime::GetCurrentTime();
		if (m_sLastTime.Compare(t.Format("Updated: %Y.%m.%d %H:%M:%S")))
		{
			DisplayUpdatedTime();
			((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Dumping data..."));
		}

		// Timer for auto flush when events stop arriving for more than 3 seconds
		SetTimer(m_ncFlushTimerID, m_ncFlushWaitTimeMillisec, TimerFlushCallback);
	}
}

void CSaveDataDlg::OnBnClickedButtonStop()
{
	// TODO: Add your control notification handler code here

	theApp.m_cKHOpenAPI.SetRealRemove(m_sScrNo_0, _T("ALL"));
	theApp.m_cKHOpenAPI.SetRealRemove(m_sScrNo_1, _T("ALL"));

	m_btStop.EnableWindow(FALSE);

	((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Waiting for remaining events..."));
	DisplayUpdatedTime();

	// Timer for auto flush when events stop arriving for more than 3 seconds
	SetTimer(m_ncFlushTimerID, m_ncFlushWaitTimeMillisec, TimerFlushCallback);
}

void CSaveDataDlg::OnBnClickedCancel()
{
	// TODO: Add your control notification handler code here

	CDialogEx::OnCancel();

	const unsigned int nBufSize=64;
	TCHAR buf[nBufSize];
	RECT rect;
	GetWindowRect(&rect);
	sprintf_s(buf, _T("%d %d"), rect.left, rect.top);
	::WritePrivateProfileString(_T("SAVEDATA"), _T("POS"), buf, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));

	for (unsigned int i = 0; i < m_nKRXCodes; i++)
	{
		if (m_pStreams[i].is_open())
			m_pStreams[i].close();
		if (m_pStreams[i + m_ncMaxItems].is_open())
			m_pStreams[i + m_ncMaxItems].close();
	}

	// This very often causes infinite loop in the background after app closes
	// theApp.m_cKHOpenAPI.CommTerminate();
	// PostQuitMessage(0);

	// Force exit without invoking theApp.m_cKHOpenAPI.CommTerminate() in destructors
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	abort();
}

VOID CALLBACK CSaveDataDlg::TimerFlushCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	for (unsigned int i = 0; i < m_nKRXCodes; i++)
		if (m_pStreams[i].is_open())
			m_pStreams[i].flush();

	((CWnd*)(pCurInstance->GetDlgItem(IDC_STATIC_DISP2)))->SetWindowText(_T("Flushed ofstream"));
	pCurInstance->DisplayUpdatedTime();

	pCurInstance->KillTimer(pCurInstance->m_ncFlushTimerID);
	
	if (pCurInstance->m_btStop.IsWindowEnabled() == FALSE)
	{
		pCurInstance->m_btRun.EnableWindow(TRUE);
		pCurInstance->m_btCancel.EnableWindow(TRUE);
	}

}

void CSaveDataDlg::DisplayUpdatedTime()
{
	// Display time of last event
	CTime t = CTime::GetCurrentTime();
	m_sLastTime = t.Format("Updated: %Y.%m.%d %H:%M:%S");
	((CWnd*)GetDlgItem(IDC_STATIC_DISP_TIME))->SetWindowText(m_sLastTime);
}