
// RealtimeDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CSaveDataDlg dialog
class CSaveDataDlg : public CDialogEx
{
// Construction
public:
	CSaveDataDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_SAVEDATA_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// My data
	int KRXCodeToInd(unsigned int iKRXCode);				// returns -1 on error

	CString m_sDataPath;
	CString m_sScrNo_0, m_sScrNo_1;
	CString m_sLastDate, m_sLastTime;

	static const unsigned int m_ncMaxItems = 200;			// Max number of codes allowed in real-time API
	static const unsigned int m_ncKRXCodesBufSize = 4096;
	static TCHAR m_sKRXCodes_0[m_ncKRXCodesBufSize];			// String of ;-separated KRX codes from config.ini
	static TCHAR m_sKRXCodes_1[m_ncKRXCodesBufSize];			// String of ;-separated KRX codes from config.ini
	static unsigned int m_nKRXCodes;						// Number of codes in "Data\config.ini"
	static unsigned int m_piKRXCodes[m_ncMaxItems];			// Array of codes, parsed
	static std::ofstream m_pStreams[m_ncMaxItems * 2];		// Array of output file streams (trade & table)

	void DisplayUpdatedTime();

	static const UINT_PTR m_ncFlushTimerID = 1;
	static const UINT m_ncFlushWaitTimeMillisec = 3000;


// Implementation
protected:
	HICON m_hIcon;

	// Generated message map functions
	virtual BOOL OnInitDialog();
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
public:
	afx_msg void OnBnClickedButtonLogin();
	DECLARE_EVENTSINK_MAP()
	void OnEventConnect(long nErrCode);
	afx_msg void OnBnClickedButtonRun();
	void OnReceiveRealData(LPCTSTR sRealKey, LPCTSTR sRealType, LPCTSTR sRealData);
	CButton m_btLogin;
	CButton m_btRun;
	afx_msg void OnBnClickedButtonStop();
	CButton m_btStop;
	afx_msg void OnBnClickedCancel();

	static VOID CALLBACK TimerFlushCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static CSaveDataDlg *pCurInstance;
	CButton m_btCancel;
};

inline int CSaveDataDlg::KRXCodeToInd(unsigned int iKRXCode)
{
	int ind = -1;

	for (unsigned int i = 0; i < m_nKRXCodes; i++)
	{
		if (m_piKRXCodes[i] == iKRXCode)
		{
			ind = i;
			break;
		}
	}

	return ind;
}