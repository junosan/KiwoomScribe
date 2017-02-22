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

// KiwoomScribeDlg.h : header file
//

#pragma once
#include "afxwin.h"

#include <string>
#include <fstream>
#include <map>
#include <vector>

#include "KHOpenAPICtrl.h"

extern CKHOpenAPI openAPI;

// CKiwoomScribeDlg dialog
class CKiwoomScribeDlg : public CDialogEx
{
// Construction
public:
    CKiwoomScribeDlg(CWnd* pParent = NULL);    // standard constructor

// Dialog Data
    enum { IDD = IDD_KIWOOMSCRIBE_DIALOG };

    protected:
    virtual void DoDataExchange(CDataExchange* pDX);            // DDX/DDV support

// My data
    struct Security {
        // used by all (KOSPI, ELW, ETF)
        std::ofstream fs_tr;
        std::ofstream fs_tb;

        // only used by ELW
        struct {
            CString name;
            char type   = '\0';
            int  expiry = 0;
            std::ofstream fs_th;
        } elw;

        // only used by ETF
        struct {
            std::ofstream fs_nav;
        } etf;
    };

    // Map code -> Security
    std::map<std::string, Security> map_KOSPI;
    std::map<std::string, Security> map_ELW;
    std::map<std::string, Security> map_ETF;

    std::ofstream fs_KOSPI200;

    std::vector<CString> scrnos; // for cleanup

    CFont font;
    CString configFile, dataPath;
    CString lastTimeStr, lastTbTime;
    void DisplayUpdatedTime();

    constexpr static UINT_PTR flushTimerID = 1;
    constexpr static UINT     flushWait_ms = 3000;
    static VOID CALLBACK FlushTimerCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime);
    
    static CKiwoomScribeDlg *this_;

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
