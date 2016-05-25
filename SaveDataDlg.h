
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
	virtual void DoDataExchange(CDataExchange* pDX);			// DDX/DDV support

// My data
	int CodeToInd(const CString &sCode);						// returns -1 on non-registered code
	void DisplayUpdatedTime();

	CFont m_font;

	int		m_iExpiryOffset;									// ELW codes with expiry of at least m_iExpiryOffset will be used
	CString m_sDataPath;
	CString m_sLastDate;
	CString m_sLastTime;

	CString m_sScrKRX_0;
	CString m_sScrKRX_1;
	CString m_sScrELW_0;
	CString m_sScrETF_0;
	CString m_sScrKOSPI;

	int m_idxFirstETF;

	static const	int m_ncMaxItems = 400;			
	static const	int m_ncCodeBufSize = (1 << 16);
	static			TCHAR m_pcProfileBuf[m_ncCodeBufSize];
	static			TCHAR m_pcKRXCodes_0[m_ncCodeBufSize];		// String of ;-separated KRX codes for SetRealReg (first 100)
	static			TCHAR m_pcKRXCodes_1[m_ncCodeBufSize];		// String of ;-separated KRX codes for SetRealReg (later 100)
	static			TCHAR m_pcELWCodes_0[m_ncCodeBufSize];		// String of ;-separated ELW codes for SetRealReg (first 100)
	static			TCHAR m_pcETFCodes_0[m_ncCodeBufSize];		// String of ;-separated ETF codes for SetRealReg (first 100)

	static			int m_nCodes;								// Number of codes in "Data\config.ini"
	static			CString m_psCodes[m_ncMaxItems];			// KRX: CODE, ELW: CODE_NAME_EXPIRY (CODE: 6 char alphanumeric)
	static			std::ofstream m_pofsTr[m_ncMaxItems];
	static			std::ofstream m_pofsTb[m_ncMaxItems];
	static			std::ofstream m_pofsTh[m_ncMaxItems];
	static			std::ofstream m_ofsKospi;

	static const	UINT_PTR m_ncFlushTimerID = 1;
	static const	UINT m_ncFlushWaitTimeMillisec = 3000;
	static			VOID CALLBACK TimerFlushCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
	static			CSaveDataDlg *pThis;

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
	CButton m_btCancel;
	CStatic m_stDisp;
	CButton m_btFetch;
	afx_msg void OnBnClickedButtonFetch();
};

inline int CSaveDataDlg::CodeToInd(const CString &sCode)
{
	int ind = -1;

	for (int i = 0; i < m_nCodes; i++)
	{
		if (m_psCodes[i].Mid(0, 6) == sCode.Mid(0, 6))
		{
			ind = i;
			break;
		}
	}

	return ind;
}