
// RealtimeDlg.h : header file
//

#pragma once
#include "afxwin.h"


// CRealtimeDlg dialog
class CRealtimeDlg : public CDialogEx
{
// Construction
public:
	CRealtimeDlg(CWnd* pParent = NULL);	// standard constructor

// Dialog Data
	enum { IDD = IDD_REALTIME_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV support

// My data
	unsigned int KRXCodeToInd(unsigned int iKRXCode);

	CString m_sScrNo;
	CString m_sLastDate;

	static const unsigned int m_ncMaxItems = 100;	// Max number of codes allowed in real-time API
	static const unsigned int m_ncKRXCodesBufSize = 1024;
	static TCHAR m_sKRXCodes[m_ncKRXCodesBufSize];			// String of ;-separated KRX codes from config.ini
	static unsigned int m_nKRXCodes;						// Number of codes
	static unsigned int m_piKRXCodes[m_ncMaxItems];		// Array of codes, parsed
	static std::ofstream m_pStreams[m_ncMaxItems * 2];			// Array of output file streams (trade & table)


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
	static CRealtimeDlg *pCurInstance;
	CButton m_btCancel;
};

inline unsigned int CRealtimeDlg::KRXCodeToInd(unsigned int iKRXCode)
{
	unsigned int ind = -1;

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