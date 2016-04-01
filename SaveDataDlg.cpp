
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
TCHAR			CSaveDataDlg::m_pcProfileBuf[m_ncCodeBufSize];
TCHAR			CSaveDataDlg::m_pcKRXCodes_0[m_ncCodeBufSize];
TCHAR			CSaveDataDlg::m_pcKRXCodes_1[m_ncCodeBufSize];
TCHAR			CSaveDataDlg::m_pcELWCodes_0[m_ncCodeBufSize];
TCHAR			CSaveDataDlg::m_pcELWCodes_1[m_ncCodeBufSize];
int				CSaveDataDlg::m_nCodes;
CString			CSaveDataDlg::m_psCodes[m_ncMaxItems];
std::ofstream	CSaveDataDlg::m_pofsTr[m_ncMaxItems];
std::ofstream	CSaveDataDlg::m_pofsTb[m_ncMaxItems];
std::ofstream	CSaveDataDlg::m_pofsTh[m_ncMaxItems];
std::ofstream	CSaveDataDlg::m_ofsKospi;
CSaveDataDlg *	CSaveDataDlg::pThis;


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
	DDX_Control(pDX, IDC_STATIC_DISP, m_stDisp);
	DDX_Control(pDX, IDC_BUTTON_FETCH, m_btFetch);
}

BEGIN_MESSAGE_MAP(CSaveDataDlg, CDialogEx)
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CSaveDataDlg::OnBnClickedButtonLogin)
	ON_BN_CLICKED(IDC_BUTTON_RUN, &CSaveDataDlg::OnBnClickedButtonRun)
	ON_BN_CLICKED(IDC_BUTTON_STOP, &CSaveDataDlg::OnBnClickedButtonStop)
	ON_BN_CLICKED(IDCANCEL, &CSaveDataDlg::OnBnClickedCancel)
	ON_BN_CLICKED(IDC_BUTTON_FETCH, &CSaveDataDlg::OnBnClickedButtonFetch)
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

	pThis = this;
	_setmaxstdio(2048); // otherwise limited to ~512 ofstreams

	m_font.CreatePointFont(80, _T("Courier New"));
	m_stDisp.SetFont(&m_font);

	m_btRun.EnableWindow(FALSE);
	m_btStop.EnableWindow(FALSE);
	m_btFetch.EnableWindow(FALSE);

	m_nCodes = 0;
	m_pcKRXCodes_0[0] = _T('\0');
	m_pcKRXCodes_1[0] = _T('\0');
	m_pcELWCodes_0[0] = _T('\0');
	m_pcELWCodes_1[0] = _T('\0');

	TCHAR sTemp[m_ncCodeBufSize];

	// ELW expiry date offset
	::GetPrivateProfileString(_T("SAVEDATA"), _T("ELW_EXPIRY_OFFSET"), _T("5"), sTemp, m_ncCodeBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	m_iExpiryOffset = _ttoi(sTemp);
	sTemp[0] = _T('\0');

	// Parse KRX code list and count # of items
	::GetPrivateProfileString(_T("SAVEDATA"), _T("KRX_CODE"), _T("000660"), m_pcProfileBuf, m_ncCodeBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	int i = 0;
	CString sCode;
	while (AfxExtractSubString(sCode, m_pcProfileBuf, i++, _T(';')))
	{
		int iC;
		for (iC = 0; iC < m_nCodes; iC++)
			if (sCode == m_psCodes[iC])
				break;
		if (iC == m_nCodes) // Avoid duplicate
		{
			if (m_nCodes < 100)
			{
				strcat_s(m_pcKRXCodes_0, sCode);
				strcat_s(m_pcKRXCodes_0, _T(";"));
			}
			else
			{
				strcat_s(m_pcKRXCodes_1, sCode);
				strcat_s(m_pcKRXCodes_1, _T(";"));
			}
			m_psCodes[m_nCodes++] = sCode;
			strcat_s (sTemp, sCode);
			strcat_s (sTemp, _T(" "));
			if ((m_nCodes % 10) == 0)
				strcat_s(sTemp, "\n");
		}
	}

	size_t l;
	l = strlen(m_pcKRXCodes_0);
	if ((l > 0) && (m_pcKRXCodes_0[l - 1] == _T(';'))) m_pcKRXCodes_0[l - 1] = _T('\0');
	l = strlen(m_pcKRXCodes_1);
	if ((l > 0) && (m_pcKRXCodes_1[l - 1] == _T(';'))) m_pcKRXCodes_1[l - 1] = _T('\0');

	// Parse ELW code list and count # of items
	::GetPrivateProfileString(_T("SAVEDATA"), _T("ELW_CODE"), _T("57AC49_한국AC49KOSPI200풋_28"), m_pcProfileBuf, m_ncCodeBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	i = 0;
	int nELWCodes = 0;
	CString sItem;
	while (AfxExtractSubString(sItem, m_pcProfileBuf, i++, _T(';')))
	{
		int idx_1  = 6; // index of first _
		if (sItem.GetLength() <= idx_1 + 1) continue;
		int idx_2  = sItem.Mid(idx_1 + 1).Find(_T('_')); // relative index of second _
		if ((idx_2 == -1) || (sItem.GetLength() <= idx_1 + 1 + idx_2 + 1)) continue;
		int idx_e = idx_1 + 1 + idx_2 + 1; // index where expiry starts

		bool bValid = false;
		if (sItem.Mid(idx_e - 3, 2) == _T("콜")) bValid = true;
		if (sItem.Mid(idx_e - 3, 2) == _T("풋")) bValid = true;					
		int iExpiry = _ttoi(sItem.Mid(idx_e));

		if ((bValid == false) || (iExpiry < m_iExpiryOffset)) continue;

		int iC;
		for (iC = 0; iC < m_nCodes; iC++)
			if (sItem == m_psCodes[iC])
				break;
		if (iC == m_nCodes) // Avoid duplicate
		{
			if (nELWCodes < 100)
			{
				strcat_s(m_pcELWCodes_0, sItem.Mid(0, 6));
				strcat_s(m_pcELWCodes_0, _T(";"));
			}
			else
			{
				strcat_s(m_pcELWCodes_1, sItem.Mid(0, 6));
				strcat_s(m_pcELWCodes_1, _T(";"));
			}
			m_psCodes[m_nCodes++] = sItem;
			nELWCodes++;
			strcat_s (sTemp, sItem.Mid(0, 6));
			strcat_s (sTemp, _T(" "));
			if ((m_nCodes % 10) == 0)
				strcat_s(sTemp, "\n");
		}
	}

	l = strlen(m_pcELWCodes_0);
	if ((l > 0) && (m_pcELWCodes_0[l - 1] == _T(';'))) m_pcELWCodes_0[l - 1] = _T('\0');
	l = strlen(m_pcELWCodes_1);
	if ((l > 0) && (m_pcELWCodes_1[l - 1] == _T(';'))) m_pcELWCodes_1[l - 1] = _T('\0');

	((CWnd*)GetDlgItem(IDC_STATIC_DISP))->SetWindowText(sTemp);
	sItem.Format(_T("%3d KRX + %3d ELW codes\n"), m_nCodes - nELWCodes, nELWCodes);
	((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(sItem);
	DisplayUpdatedTime();

	// Data dump path
	::GetPrivateProfileString(_T("SAVEDATA"), _T("DATA_DIR"), _T("SaveData\\Data"), sTemp, m_ncCodeBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	m_sDataPath = theApp.m_sAppPath + _T("\\") + sTemp;

	// Retrieve last position
	::GetPrivateProfileString(_T("SAVEDATA"), _T("POS"), _T("100 100"), sTemp, m_ncCodeBufSize, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));
	int x, y;
	if (2 == sscanf_s(sTemp, _T("%d %d"), &x, &y))
		SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

	// Used for API communication
	m_sScrKRX_0 = _T("9999");
	m_sScrKRX_1 = _T("9998");
	m_sScrELW_0 = _T("9997");
	m_sScrELW_1 = _T("9996");
	m_sScrKOSPI = _T("9990");

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
	bool bSuccess = true;

	// Set up real time monitor API (KRX codes & FIDs)
	if (bSuccess && ((ret = theApp.m_cKHOpenAPI.SetRealReg(m_sScrKRX_0, m_pcKRXCodes_0, _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80"), _T("0"))) < 0))
		bSuccess = false;

	if (bSuccess && ((ret = theApp.m_cKHOpenAPI.SetRealReg(m_sScrKRX_1, m_pcKRXCodes_1, _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80"), _T("0"))) < 0))
		bSuccess = false;

	if (bSuccess && ((ret = theApp.m_cKHOpenAPI.SetRealReg(m_sScrELW_0, m_pcELWCodes_0, _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80;621;622;623;624;625;626;627;628;629;630;631;632;633;634;635;636;637;638;639;640;670;671;672;673;674;675;676;706"), _T("0"))) < 0))
		bSuccess = false;

	if (bSuccess && ((ret = theApp.m_cKHOpenAPI.SetRealReg(m_sScrELW_1, m_pcELWCodes_1, _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80;621;622;623;624;625;626;627;628;629;630;631;632;633;634;635;636;637;638;639;640;670;671;672;673;674;675;676;706"), _T("0"))) < 0))
		bSuccess = false;

	// Set up KOSPI200 monitor
	theApp.m_cKHOpenAPI.SetInputValue(_T("시장구분"), _T("0"));
	theApp.m_cKHOpenAPI.SetInputValue(_T("업종코드"), _T("201"));
	if (bSuccess && ((ret = theApp.m_cKHOpenAPI.CommRqData(_T("Get_KOSPI200"), _T("OPT20001"), 0, m_sScrKOSPI)) < 0))
		bSuccess = false;

	if (bSuccess)
	{
		m_btRun.EnableWindow(FALSE);
		m_btStop.EnableWindow(TRUE);
		m_btCancel.EnableWindow(FALSE);
		m_btFetch.EnableWindow(FALSE);

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
			if (!::PathIsDirectory(m_sDataPath))
				::CreateDirectory(m_sDataPath, NULL);
			if (!::PathIsDirectory(sDir))
				::CreateDirectory(sDir, NULL);

			// Copy config.ini into sDir
			sFileName = _T("copy ") + theApp.m_sAppPath + _T("\\SaveData\\config.ini") + _T(" ") + sDir;
			system(sFileName);

			for (int i = 0; i < m_nCodes; i++)
			{
				CString sItem = m_psCodes[i]; // ###### (KRX) or ######_ELWNAME_#### (ELW) format
				CString sCode = sItem.Mid(0, 6);

				if (sItem.GetLength() > 6) // ELW
				{
					int idx_n = 0     + sItem           .Find(_T('_')) + 1; // index where name starts
					int idx_e = idx_n + sItem.Mid(idx_n).Find(_T('_')) + 1; // index where expiry starts

					char cType = '\0';
					if (sItem.Mid(idx_e - 3, 2) == _T("콜")) cType = 'c';
					if (sItem.Mid(idx_e - 3, 2) == _T("풋")) cType = 'p';
					int iExpiry = _ttoi(sItem.Mid(idx_e));
					CString sName = sItem.Mid(idx_n, idx_e - idx_n - 1);

					sFileName = sDir + _T("\\") + sCode + CString(_T("i.txt"));
					std::ofstream ofs;
					ofs.open(sFileName.GetBuffer(), std::ofstream::app);
					if (ofs.is_open())
					{
						const size_t szBuf = (1 << 12);
						char pcBuf[szBuf];
						sprintf_s(pcBuf, _T("TYPE=%c\nEXPIRY=%d\nNAME=%s\n"), cType, iExpiry, sName.GetBuffer());
						ofs.write(pcBuf, strlen(pcBuf));
						ofs.close();
					}
				}

				// Stream for trade updates
				if (m_pofsTr[i].is_open()) m_pofsTr[i].close();
				sFileName = sDir + _T("\\") + sCode + CString(_T(".txt"));
				m_pofsTr[i].open(sFileName.GetBuffer(), std::ofstream::app);

				// Stream for table updates
				if (m_pofsTb[i].is_open()) m_pofsTb[i].close();
				sFileName = sDir + _T("\\") + sCode + CString(_T("t.txt"));
				m_pofsTb[i].open(sFileName.GetBuffer(), std::ofstream::app);

				// no theoretical values for KRX
				if (sItem.GetLength() <= 6) continue;
				
				// Stream for theoretical value updates
				if (m_pofsTh[i].is_open()) m_pofsTh[i].close();
				sFileName = sDir + _T("\\") + sCode + CString(_T("g.txt"));
				m_pofsTh[i].open(sFileName.GetBuffer(), std::ofstream::app);
			}

			if (m_ofsKospi.is_open()) m_ofsKospi.close();
			sFileName = sDir + _T("\\") + CString(_T("KOSPI200.txt"));
			m_ofsKospi.open(sFileName.GetBuffer(), std::ofstream::app);
		}
	}
	else
	{
		CString sDisp;
		sDisp.Format(_T("KHOpenAPI.SetRealReg: Error #%d"), ret);
		AfxMessageBox(sDisp);
	}

}


void CSaveDataDlg::OnReceiveRealData(LPCTSTR _sRealKey, LPCTSTR _sRealType, LPCTSTR sRealData)
{
	// TODO: Add your message handler code here
	CString sRealKey (_sRealKey);
	CString sRealType (_sRealType);
	const size_t bufSize = (1 << 12);
	TCHAR pcWriteBuf[bufSize];
	int data[61], ind;
	double pdData[10];
	
	bool bRelevant = false;

	if ((ind = CodeToInd(sRealKey)) != -1)
	{
		if (_T("주식체결") == sRealType)
		{
			// Time
			data[0] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 20));
			// Trade quantity
			data[1] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 15));
			// Trade price
			data[2] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 10));
			// Sell price 1
			data[3] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 27));
			// Buy price 1
			data[4] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 28));

			sprintf_s(pcWriteBuf, "%06d\t%d\t%d\t%d\t%d\n", data[0], data[1], data[2], data[3], data[4]);

			m_pofsTr[ind].write(pcWriteBuf, strlen(pcWriteBuf));

			bRelevant = true;
		}

		if (_T("주식호가잔량") == sRealType)
		{
			// Time
			data[0] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 21));

			if (m_psCodes[ind].GetLength() <= 6) // KRX
			{
				// Price table (price, quantity) (high->low)
				for (int i = 0; i < 10; i++)
				{
					// Sell price 10 ~ 1 FID(50->41, 70->61)
					data[1  + (i << 1)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 50 - i)); // ps10->ps1
					data[2  + (i << 1)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 70 - i)); // qs10->qs1
				
					// Buy price 1 ~ 10 FID(51->60, 71->80)
					data[21 + (i << 1)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 51 + i)); // pb1->pb10
					data[22 + (i << 1)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 71 + i)); // qb1->qb10
				}

				sprintf_s(pcWriteBuf, "%06d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
											"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
											"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
											"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",\
					data[0], data[ 1], data[ 2], data[ 3], data[ 4], data[ 5], data[ 6], data[ 7], data[ 8], data[ 9], data[10],\
							 data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20],\
							 data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30],\
							 data[31], data[32], data[33], data[34], data[35], data[36], data[37], data[38], data[39], data[40]);
			}
			else // ELW
			{
				// Price table (price, quantity) (high->low)
				for (int i = 0; i < 10; i++)
				{
					// Sell price 10 ~ 1 FID(50->41, 70->61, 630->621)
					data[1  + (i * 3)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey,  50 - i)); // ps10->ps1
					data[2  + (i * 3)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey,  70 - i)); // qs10->qs1
					data[3  + (i * 3)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 630 - i)); // ls10->ls1
				
					// Buy price 1 ~ 10 FID(51->60, 71->80, 631->640)
					data[31 + (i * 3)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey,  51 + i)); // pb1->pb10
					data[32 + (i * 3)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey,  71 + i)); // qb1->qb10
					data[33 + (i * 3)] = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 631 + i)); // lb1->lb10
				}

				sprintf_s(pcWriteBuf, "%06d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
											"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
											"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
											"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
											"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t" \
											"%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",\
					data[0], data[ 1], data[ 2], data[ 3], data[ 4], data[ 5], data[ 6], data[ 7], data[ 8], data[ 9], data[10],\
							 data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20],\
							 data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30],\
							 data[31], data[32], data[33], data[34], data[35], data[36], data[37], data[38], data[39], data[40],\
							 data[41], data[42], data[43], data[44], data[45], data[46], data[47], data[48], data[49], data[50],\
							 data[51], data[52], data[53], data[54], data[55], data[56], data[57], data[58], data[59], data[60] );
			}

			m_pofsTb[ind].write(pcWriteBuf, strlen(pcWriteBuf));

			bRelevant = true;
		}

		if ((_T("ELW 이론가") == sRealType) && (m_psCodes[ind].GetLength() > 6))
		{ 
			// Time
			data[0]  = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 20));
			// Current price
			data[1]  = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 10));
			// ELW theoretical price
			pdData[2] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 670)) / 100.0;
			// ELW volatility
			pdData[3] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 671));
			// ELW delta
			pdData[4] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 672)) / 1000000.0;
			// ELW gamma
			pdData[5] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 673)) / 1000000.0;
			// ELW theta
			pdData[6] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 674));
			// ELW vega
			pdData[7] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 675));
			// ELW rho
			pdData[8] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 676));
			// LP volatility
			pdData[9] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 706));

			sprintf_s(pcWriteBuf, "%06d\t%d\t%.2f\t%.2f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.2f\n",\
									  data[0],   data[1], pdData[2], pdData[3], pdData[4],\
									pdData[5], pdData[6], pdData[7], pdData[8], pdData[9]);

			m_pofsTh[ind].write(pcWriteBuf, strlen(pcWriteBuf));

			bRelevant = true;
		}
	}
	else if ((_T("201") == sRealKey) && (_T("업종지수") == sRealType))
	{
		// Time
		data[0]   = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 20));
		// Current price
		pdData[1] = _ttof(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 10));
		// Trade quant
		data[2]   = _ttoi(theApp.m_cKHOpenAPI.GetCommRealData(sRealKey, 15));

		sprintf_s(pcWriteBuf, "%06d\t%.2f\t%d\n", data[0], pdData[1], data[2]);

		m_ofsKospi.write(pcWriteBuf, strlen(pcWriteBuf));

		bRelevant = true;
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

	theApp.m_cKHOpenAPI.SetRealRemove(m_sScrKRX_0, _T("ALL"));
	theApp.m_cKHOpenAPI.SetRealRemove(m_sScrKRX_1, _T("ALL"));
	theApp.m_cKHOpenAPI.SetRealRemove(m_sScrELW_0, _T("ALL"));
	theApp.m_cKHOpenAPI.SetRealRemove(m_sScrELW_1, _T("ALL"));
	theApp.m_cKHOpenAPI.SetRealRemove(m_sScrKOSPI, _T("ALL"));

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

	const size_t nBufSize=64;
	TCHAR buf[nBufSize];
	RECT rect;
	GetWindowRect(&rect);
	sprintf_s(buf, _T("%d %d"), rect.left, rect.top);
	::WritePrivateProfileString(_T("SAVEDATA"), _T("POS"), buf, theApp.m_sAppPath + _T("\\SaveData\\config.ini"));

	for (int i = 0; i < m_nCodes; i++)
	{
		if (m_pofsTr[i].is_open()) m_pofsTr[i].close();
		if (m_pofsTb[i].is_open()) m_pofsTb[i].close();
		if (m_pofsTh[i].is_open()) m_pofsTh[i].close();
	}
	if (m_ofsKospi.is_open()) m_ofsKospi.close();

	// This very often causes infinite loop in the background after app closes
	// theApp.m_cKHOpenAPI.CommTerminate();
	// PostQuitMessage(0);

	// Force exit without invoking theApp.m_cKHOpenAPI.CommTerminate() in destructors
	SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
	abort();
}

VOID CALLBACK CSaveDataDlg::TimerFlushCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
	for (int i = 0; i < m_nCodes; i++)
	{
		if (m_pofsTr[i].is_open()) m_pofsTr[i].flush();
		if (m_pofsTb[i].is_open()) m_pofsTb[i].flush();
		if (m_pofsTh[i].is_open()) m_pofsTh[i].flush();
	}
	if (m_ofsKospi.is_open()) m_ofsKospi.flush();

	((CWnd*)(pThis->GetDlgItem(IDC_STATIC_DISP2)))->SetWindowText(_T("Flushed ofstreams"));
	pThis->DisplayUpdatedTime();

	pThis->KillTimer(pThis->m_ncFlushTimerID);
	
	if (pThis->m_btStop.IsWindowEnabled() == FALSE)
	{
		pThis->m_btRun.EnableWindow(TRUE);
		pThis->m_btCancel.EnableWindow(TRUE);
		pThis->m_btFetch.EnableWindow(TRUE);
	}

}

void CSaveDataDlg::DisplayUpdatedTime()
{
	// Display time of last event
	CTime t = CTime::GetCurrentTime();
	m_sLastTime = t.Format("Updated: %Y.%m.%d %H:%M:%S");
	((CWnd*)GetDlgItem(IDC_STATIC_DISP_TIME))->SetWindowText(m_sLastTime);
}

void CSaveDataDlg::OnBnClickedButtonFetch()
{
	// TODO: Add your control notification handler code here

	CString sCmd = theApp.m_sAppPath + _T("\\PAXNetELW\\PAXNetELW.exe");
	system(sCmd);
}
