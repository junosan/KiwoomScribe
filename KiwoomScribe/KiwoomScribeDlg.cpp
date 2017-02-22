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

// KiwoomScribeDlg.cpp : implementation file
//

#include "stdafx.h"
#include "KiwoomScribe.h"
#include "KiwoomScribeDlg.h"
#include "afxdialogex.h"

#include <string>

#ifdef _DEBUG
#define new DEBUG_NEW
#endif

// The one and only instance
CKHOpenAPI openAPI;

// Static variable
CKiwoomScribeDlg* CKiwoomScribeDlg::this_ = nullptr;

// CKiwoomScribeDlg dialog

CKiwoomScribeDlg::CKiwoomScribeDlg(CWnd* pParent /*=NULL*/)
    : CDialogEx(CKiwoomScribeDlg::IDD, pParent)
{
    m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
}

void CKiwoomScribeDlg::DoDataExchange(CDataExchange* pDX)
{
    CDialogEx::DoDataExchange(pDX);
    DDX_Control(pDX, IDC_KHOPENAPICTRL1, openAPI);
    DDX_Control(pDX, IDC_BUTTON_LOGIN, m_btLogin);
    DDX_Control(pDX, IDC_BUTTON_RUN, m_btRun);
    DDX_Control(pDX, IDC_BUTTON_STOP, m_btStop);
    DDX_Control(pDX, IDCANCEL, m_btCancel);
    DDX_Control(pDX, IDC_STATIC_DISP, m_stDisp);
    DDX_Control(pDX, IDC_BUTTON_FETCH, m_btFetch);
}

BEGIN_MESSAGE_MAP(CKiwoomScribeDlg, CDialogEx)
    ON_WM_PAINT()
    ON_WM_QUERYDRAGICON()
    ON_BN_CLICKED(IDC_BUTTON_LOGIN, &CKiwoomScribeDlg::OnBnClickedButtonLogin)
    ON_BN_CLICKED(IDC_BUTTON_RUN, &CKiwoomScribeDlg::OnBnClickedButtonRun)
    ON_BN_CLICKED(IDC_BUTTON_STOP, &CKiwoomScribeDlg::OnBnClickedButtonStop)
    ON_BN_CLICKED(IDCANCEL, &CKiwoomScribeDlg::OnBnClickedCancel)
    ON_BN_CLICKED(IDC_BUTTON_FETCH, &CKiwoomScribeDlg::OnBnClickedButtonFetch)
END_MESSAGE_MAP()


// CKiwoomScribeDlg message handlers

BOOL CKiwoomScribeDlg::OnInitDialog()
{
    CDialogEx::OnInitDialog();

    // Set the icon for this dialog.  The framework does this automatically
    //  when the application's main window is not a dialog
    SetIcon(m_hIcon, TRUE);  // Set big icon
    SetIcon(m_hIcon, FALSE); // Set small icon

    // TODO: Add extra initialization here

    verify(this_ == nullptr); // allow only one instance
    this_ = this;

    _setmaxstdio(2048); // otherwise limited to ~512 ofstreams
    verify(2048 == _getmaxstdio());

    font.CreatePointFont(80, _T("Courier New"));
    m_stDisp.SetFont(&font); // font needs to be non-temporary

    m_btRun.EnableWindow(FALSE);
    m_btStop.EnableWindow(FALSE);
    m_btFetch.EnableWindow(FALSE);

    constexpr static int buf_size = 1 << 16;
    static TCHAR buf[buf_size];

    configFile = theApp.path + _T("\\config.ini");
    CString substr;

    // Parse KRX code list
    ::GetPrivateProfileString(_T("SAVEDATA"), _T("KRX_CODE"), _T("000660"), buf, buf_size, configFile);
    for (int i = 0; AfxExtractSubString(substr, buf, i, _T(';')); ++i) {
        verify(substr.GetLength() == 6);
        map_KOSPI.insert(std::make_pair(std::string(substr), Security())); // duplicate rejected automatically
    }

    // ELW expiry date offset
    ::GetPrivateProfileString(_T("SAVEDATA"), _T("ELW_EXPIRY_OFFSET"), _T("5"), buf, buf_size, configFile);
    int expiryOffset = _ttoi(buf);

    // Parse ELW code list
    ::GetPrivateProfileString(_T("SAVEDATA"), _T("ELW_CODE"), _T("57AC49_한국AC49KOSPI200풋_28"), buf, buf_size, configFile);
    for (int i = 0; AfxExtractSubString(substr, buf, i, _T(';')); ++i)
    {
        int idx_1 = 6; // index of first _
        if (substr.GetLength() <= idx_1 + 1) continue;
        int idx_2 = substr.Mid(idx_1 + 1).Find(_T('_')); // relative index of second _
        if (idx_2 == -1 ||
            substr.GetLength() <= idx_1 + 1 + idx_2 + 1) continue;
        int idx_e = idx_1 + 1 + idx_2 + 1; // index where expiry starts

        CString name = substr.Mid(idx_1 + 1, idx_2);
        char type = '\0';
        if (substr.Mid(idx_e - 3, 2) == _T("콜")) type = 'c';
        if (substr.Mid(idx_e - 3, 2) == _T("풋")) type = 'p';                    
        int expiry = _ttoi(substr.Mid(idx_e));

        if (type == '\0' || expiry < expiryOffset) continue;

        auto it_success = map_ELW.insert(std::make_pair(std::string(substr.Mid(0, 6)), Security())); // duplicate rejected automatically
        if (it_success.second == true) {
            it_success.first->second.elw.name   = name;
            it_success.first->second.elw.type   = type;
            it_success.first->second.elw.expiry = expiry;
        }
    }

    // Parse ETF code list
    ::GetPrivateProfileString(_T("SAVEDATA"), _T("ETF_CODE"), _T("122630"), buf, buf_size, configFile);
    for (int i = 0; AfxExtractSubString(substr, buf, i, _T(';')); ++i) {
        verify(substr.GetLength() == 6);
        map_ETF.insert(std::make_pair(std::string(substr), Security())); // duplicate rejected automatically
    }

    CString code_list;

    auto AppendSecurity = [&](const TCHAR* security, const std::map<std::string, Security> &map) {
        substr.Format(_T("%s (%d)\n"), security, map.size());
        code_list += substr;

        const int n_newline = 20;
        int n = 0;
        for (auto it = std::begin(map), end = std::end(map); it != end; ++it) {
            code_list += it->first.c_str();
            if (++n < n_newline)
                code_list += _T(" ");
            else {
                code_list += _T("\n");
                n = 0;
            }
        }
        if (n != 0) code_list += _T("\n");
    };

    AppendSecurity(_T("KOSPI"), map_KOSPI);
    AppendSecurity(_T("ELW")  , map_ELW  );
    AppendSecurity(_T("ETF")  , map_ETF  );

    ((CWnd*)GetDlgItem(IDC_STATIC_DISP))->SetWindowText(code_list);
    lastTbTime = _T("08:00:00");
    DisplayUpdatedTime();

    // Data dump path = DATA_DIR\YYYYMMDD
    ::GetPrivateProfileString(_T("SAVEDATA"), _T("DATA_DIR"), _T("C:"), buf, buf_size, configFile);
    {
        size_t len = strlen(buf);
        verify(len > 0);
        if (buf[len - 1] == '\\') buf[len - 1] = '\0';
    }
    dataPath = CString(buf) + _T("\\") + CTime::GetCurrentTime().Format("%Y%m%d");

    // Retrieve last position
    ::GetPrivateProfileString(_T("SAVEDATA"), _T("POS"), _T("100 100"), buf, buf_size, configFile);
    int x, y;
    if (2 == sscanf_s(buf, _T("%d %d"), &x, &y))
        SetWindowPos(NULL, x, y, 0, 0, SWP_NOSIZE | SWP_NOZORDER);

    return TRUE;  // return TRUE  unless you set the focus to a control
}

// If you add a minimize button to your dialog, you will need the code below
//  to draw the icon.  For MFC applications using the document/view model,
//  this is automatically done for you by the framework.

void CKiwoomScribeDlg::OnPaint()
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
HCURSOR CKiwoomScribeDlg::OnQueryDragIcon()
{
    return static_cast<HCURSOR>(m_hIcon);
}



void CKiwoomScribeDlg::OnBnClickedButtonLogin()
{
    // TODO: Add your control notification handler code here

    if (openAPI.CommConnect() < 0)
    {
        AfxMessageBox(_T("OpenAPI::CommConnect: Login failed"));
    }

}

BEGIN_EVENTSINK_MAP(CKiwoomScribeDlg, CDialogEx)
    ON_EVENT(CKiwoomScribeDlg, IDC_KHOPENAPICTRL1, 5, CKiwoomScribeDlg::OnEventConnect, VTS_I4)
    ON_EVENT(CKiwoomScribeDlg, IDC_KHOPENAPICTRL1, 2, CKiwoomScribeDlg::OnReceiveRealData, VTS_BSTR VTS_BSTR VTS_BSTR)
END_EVENTSINK_MAP()


void CKiwoomScribeDlg::OnEventConnect(long nErrCode)
{
    // TODO: Add your message handler code here
    
    if (nErrCode < 0)
    {
        CString str;
        str.Format(_T("OpenAPI::OnEventConnect: Error %d"), nErrCode);
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


void CKiwoomScribeDlg::OnBnClickedButtonRun()
{
    // TODO: Add your control notification handler code here

    long retsum = 0;

    // starts from scrno_ini, then decreases by 1 every 100 items (OpenAPI limits 100 items per request per scrno)
    auto SetupRealtime = [&](int scrno_ini, const std::map<std::string, Security> &map, const CString &FID_list) {
        int scrno = scrno_ini;
        CString codes, scrno_str;
        constexpr int n_bin = 100;
        int n = 0;
        for (auto it = std::begin(map), end = std::end(map); it != end; ++it) {
            codes += it->first.c_str();
            auto temp_it = it;
            if (++n == n_bin || ++temp_it == end) {
                scrno_str.Format(_T("%d"), scrno);
                scrnos.push_back(scrno_str);
                retsum += openAPI.SetRealReg(scrno_str, codes, FID_list, _T("0"));
                codes.Empty();
                n = 0;
                --scrno;
            } else
                codes += _T(";");
        }
    };

    CString FID_KOSPI = _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80");
    CString FID_ELW   = _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80;621;622;623;624;625;626;627;628;629;630;631;632;633;634;635;636;637;638;639;640;670;671;672;673;674;675;676;706");
    CString FID_ETF   = _T("10;15;20;21;27;28;41;42;43;44;45;46;47;48;49;50;51;52;53;54;55;56;57;58;59;60;61;62;63;64;65;66;67;68;69;70;71;72;73;74;75;76;77;78;79;80;36;39;265;266");

    SetupRealtime(9990, map_KOSPI, FID_KOSPI);
    SetupRealtime(9980, map_ELW  , FID_ELW  );
    SetupRealtime(9970, map_ETF  , FID_ETF  );

    // Set up KOSPI200 monitor
    openAPI.SetInputValue(_T("시장구분"), _T("0"));
    openAPI.SetInputValue(_T("업종코드"), _T("201"));

    constexpr TCHAR* scrno_KOSPI200 = _T("9999");
    scrnos.push_back(scrno_KOSPI200);
    retsum += openAPI.CommRqData(_T("Get_KOSPI200"), _T("OPT20001"), 0, scrno_KOSPI200);

    if (retsum == 0) // no error
    {
        m_btRun.EnableWindow(FALSE);
        m_btStop.EnableWindow(TRUE);
        m_btCancel.EnableWindow(FALSE);
        m_btFetch.EnableWindow(FALSE);

        ((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Started realtime data dump"));
        DisplayUpdatedTime();

        // Create directories
        if (FALSE == ::PathIsDirectory(dataPath)) {
            BOOL ret = ::CreateDirectory(dataPath, NULL);
            verify(ret != 0); // 0 is the value specified in MSDN; triggers if DATA_DIR is a non-existent directory
        }

        CString etfPath = dataPath + _T("\\ETF");
        if (FALSE == ::PathIsDirectory(etfPath)) {
            BOOL ret = ::CreateDirectory(etfPath, NULL);
            verify(ret != 0);
        }

        // Copy config.ini to dataPath
        system(_T("copy ") + configFile + _T(" ") + dataPath);

        // tr & tb data ofstreams
        auto OpenAllTrTb = [&](std::map<std::string, Security> &map, const CString &path) {
            for (auto it = std::begin(map), end = std::end(map); it != end; ++it) {
                std::string filename = path + _T("\\") + it->first.c_str() + _T(".txt");
                it->second.fs_tr.open(filename, std::ofstream::app);
                filename = path + _T("\\") + it->first.c_str() + _T("t.txt");
                it->second.fs_tb.open(filename, std::ofstream::app);
            }
        };

        OpenAllTrTb(map_KOSPI, dataPath);
        OpenAllTrTb(map_ELW  , dataPath);
        OpenAllTrTb(map_ETF  , etfPath );

        // ELW-specific ofstreams
        for (auto it = std::begin(map_ELW), end = std::end(map_ELW); it != end; ++it) {
            std::string filename = dataPath + _T("\\") + it->first.c_str() + _T("i.txt");
            {
                std::ofstream ofs_i(filename, std::ofstream::app);
                ofs_i << "TYPE="   << it->second.elw.type   << '\n'
                      << "EXPIRY=" << it->second.elw.expiry << '\n'
                      << "NAME="   << it->second.elw.name   << '\n';
            } // ofs_i closed here
            filename = dataPath + _T("\\") + it->first.c_str() + _T("g.txt");
            it->second.elw.fs_th.open(filename, std::ofstream::app);
        }

        // ETF-specific ofstreams
        for (auto it = std::begin(map_ETF), end = std::end(map_ETF); it != end; ++it) {
            std::string filename = etfPath + _T("\\") + it->first.c_str() + _T("n.txt");
            it->second.etf.fs_nav.open(filename, std::ofstream::app);
        }

        // KOSPI200 ofstream
        std::string filename = dataPath + _T("\\KOSPI200.txt");
        fs_KOSPI200.open(filename, std::ofstream::app);
    }
    else
    {
        CString sDisp;
        sDisp.Format(_T("OpenAPI::SetRealReg: Error sum %d"), retsum);
        AfxMessageBox(sDisp);
    }

}


void CKiwoomScribeDlg::OnReceiveRealData(LPCTSTR sRealKey_, LPCTSTR sRealType_, LPCTSTR sRealData)
{
    constexpr static size_t buf_size = 1 << 16;
    static TCHAR buf[buf_size];

    CString sRealKey (sRealKey_ );
    CString sRealType(sRealType_);
    std::string code(sRealKey);

    int data[61];
    double pdData[10];
    
    bool relevant = false;

    auto it = map_KOSPI.find(code);
    if (it == std::end(map_KOSPI)) it = map_ELW.find(code);
    if (it == std::end(map_ELW  )) it = map_ETF.find(code);

    // it == std::end(map_ETF) iff none of the maps have code
    if (it != std::end(map_ETF))
    {
        if (_T("주식체결") == sRealType)
        {
            // Time
            data[0] = _ttoi(openAPI.GetCommRealData(sRealKey, 20)); // note: atoi doesn't throw but returns 0 on error
            // Trade quantity
            data[1] = _ttoi(openAPI.GetCommRealData(sRealKey, 15));
            // Trade price
            data[2] = _ttoi(openAPI.GetCommRealData(sRealKey, 10));
            // Sell price 1
            data[3] = _ttoi(openAPI.GetCommRealData(sRealKey, 27));
            // Buy price 1
            data[4] = _ttoi(openAPI.GetCommRealData(sRealKey, 28));

            sprintf_s(buf, "%06d\t%d\t%d\t%d\t%d\n", data[0], data[1], data[2], data[3], data[4]);

            it->second.fs_tr.write(buf, strlen(buf));

            relevant = true;
        }

        if (_T("주식호가잔량") == sRealType)
        {
            // Time
            CString time = openAPI.GetCommRealData(sRealKey, 21);
            data[0] = _ttoi(time);

            time.Insert(4, _T(':'));
            time.Insert(2, _T(':'));

            if (time.Compare(lastTbTime) > 0) // don't update with earlier data mixed in with new data
                lastTbTime = time;

            if (it->second.elw.type == '\0') // KOSPI or ETF
            {
                // Price table (price, quantity) (high->low)
                for (int i = 0; i < 10; i++)
                {
                    // Sell price 10 ~ 1 FID(50->41, 70->61)
                    data[1 + (i << 1)] = _ttoi(openAPI.GetCommRealData(sRealKey, 50 - i)); // ps10->ps1
                    data[2 + (i << 1)] = _ttoi(openAPI.GetCommRealData(sRealKey, 70 - i)); // qs10->qs1

                    // Buy price 1 ~ 10 FID(51->60, 71->80)
                    data[21 + (i << 1)] = _ttoi(openAPI.GetCommRealData(sRealKey, 51 + i)); // pb1->pb10
                    data[22 + (i << 1)] = _ttoi(openAPI.GetCommRealData(sRealKey, 71 + i)); // qb1->qb10
                }

                sprintf_s(buf, "%06d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t"\
                                     "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t"\
                                     "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t"\
                                     "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                    data[ 0], data[ 1], data[ 2], data[ 3], data[ 4], data[ 5], data[ 6], data[ 7], data[ 8], data[ 9], data[10],
                              data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20],
                              data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30],
                              data[31], data[32], data[33], data[34], data[35], data[36], data[37], data[38], data[39], data[40]);
            }
            else // ELW
            {
                // Price table (price, quantity) (high->low)
                for (int i = 0; i < 10; i++)
                {
                    // Sell price 10 ~ 1 FID(50->41, 70->61, 630->621)
                    data[1 + (i * 3)] = _ttoi(openAPI.GetCommRealData(sRealKey, 50 - i)); // ps10->ps1
                    data[2 + (i * 3)] = _ttoi(openAPI.GetCommRealData(sRealKey, 70 - i)); // qs10->qs1
                    data[3 + (i * 3)] = _ttoi(openAPI.GetCommRealData(sRealKey, 630 - i)); // ls10->ls1

                    // Buy price 1 ~ 10 FID(51->60, 71->80, 631->640)
                    data[31 + (i * 3)] = _ttoi(openAPI.GetCommRealData(sRealKey, 51 + i)); // pb1->pb10
                    data[32 + (i * 3)] = _ttoi(openAPI.GetCommRealData(sRealKey, 71 + i)); // qb1->qb10
                    data[33 + (i * 3)] = _ttoi(openAPI.GetCommRealData(sRealKey, 631 + i)); // lb1->lb10
                }

                sprintf_s(buf, "%06d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t"\
                                     "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t"\
                                     "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t"\
                                     "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t"\
                                     "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t"\
                                     "%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\t%d\n",
                    data[ 0], data[ 1], data[ 2], data[ 3], data[ 4], data[ 5], data[ 6], data[ 7], data[ 8], data[ 9], data[10],
                              data[11], data[12], data[13], data[14], data[15], data[16], data[17], data[18], data[19], data[20],
                              data[21], data[22], data[23], data[24], data[25], data[26], data[27], data[28], data[29], data[30],
                              data[31], data[32], data[33], data[34], data[35], data[36], data[37], data[38], data[39], data[40],
                              data[41], data[42], data[43], data[44], data[45], data[46], data[47], data[48], data[49], data[50],
                              data[51], data[52], data[53], data[54], data[55], data[56], data[57], data[58], data[59], data[60]);
            }

            it->second.fs_tb.write(buf, strlen(buf));

            relevant = true;
        }

        if (_T("ELW 이론가") == sRealType && it->second.elw.type != '\0')
        {
            // Time
            data[0] = _ttoi(openAPI.GetCommRealData(sRealKey, 20));
            // Current price
            data[1] = _ttoi(openAPI.GetCommRealData(sRealKey, 10));
            // ELW theoretical price
            pdData[2] = _ttof(openAPI.GetCommRealData(sRealKey, 670)) / 100.0;
            // ELW volatility
            pdData[3] = _ttof(openAPI.GetCommRealData(sRealKey, 671));
            // ELW delta
            pdData[4] = _ttof(openAPI.GetCommRealData(sRealKey, 672)) / 1000000.0;
            // ELW gamma
            pdData[5] = _ttof(openAPI.GetCommRealData(sRealKey, 673)) / 1000000.0;
            // ELW theta
            pdData[6] = _ttof(openAPI.GetCommRealData(sRealKey, 674));
            // ELW vega
            pdData[7] = _ttof(openAPI.GetCommRealData(sRealKey, 675));
            // ELW rho
            pdData[8] = _ttof(openAPI.GetCommRealData(sRealKey, 676));
            // LP volatility
            pdData[9] = _ttof(openAPI.GetCommRealData(sRealKey, 706));

            sprintf_s(buf, "%06d\t%d\t%.2f\t%.2f\t%.6f\t%.6f\t%.6f\t%.6f\t%.6f\t%.2f\n",
                  data[0],   data[1], pdData[2], pdData[3], pdData[4],
                pdData[5], pdData[6], pdData[7], pdData[8], pdData[9]);
            
            it->second.elw.fs_th.write(buf, strlen(buf));

            relevant = true;
        }

        if (_T("ETF NAV") == sRealType && it->second.etf.fs_nav.is_open())
        {
            std::string sTime  = openAPI.GetCommRealData(sRealKey,  20).Trim(); // time
            std::string sCurPr = openAPI.GetCommRealData(sRealKey,  10).Trim(); // current price
            std::string sNAV   = openAPI.GetCommRealData(sRealKey,  36).Trim(); // NAV
            std::string sErr   = openAPI.GetCommRealData(sRealKey,  39).Trim(); // track error
            std::string sIdxR  = openAPI.GetCommRealData(sRealKey, 265).Trim(); // NAV/index deviation
            std::string sETFR  = openAPI.GetCommRealData(sRealKey, 266).Trim(); // NAV/ETF deviation

            sprintf_s(buf, "%s\t%s\t%s\t%s\t%s\t%s\n", \
                sTime.c_str(), sCurPr.c_str(), sNAV.c_str(), sErr.c_str(), sIdxR.c_str(), sETFR.c_str());

            it->second.etf.fs_nav.write(buf, strlen(buf));

            relevant = true;
        }
    }
    
    if (_T("201") == sRealKey && _T("업종지수") == sRealType)
    {
        // Time
        data[0]   = _ttoi(openAPI.GetCommRealData(sRealKey, 20));
        // Current price
        pdData[1] = _ttof(openAPI.GetCommRealData(sRealKey, 10));
        // Trade quant
        data[2]   = _ttoi(openAPI.GetCommRealData(sRealKey, 15));

        sprintf_s(buf, "%06d\t%.2f\t%d\n", data[0], pdData[1], data[2]);

        fs_KOSPI200.write(buf, strlen(buf));

        relevant = true;
    }
    
    if (relevant)
    {
        // Display updated time every second while events are arriving
        CString curTimeStr = CTime::GetCurrentTime().Format("%H:%M:%S/");
        curTimeStr.Append(lastTbTime);
        if (0 != lastTimeStr.Compare(curTimeStr))
        {
            DisplayUpdatedTime();
            ((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Dumping data..."));
        }

        // Timer for auto flush when events stop arriving for more than 3 seconds
        SetTimer(flushTimerID, flushWait_ms, FlushTimerCallback);
    }
}

void CKiwoomScribeDlg::OnBnClickedButtonStop()
{
    for (const auto &scrno : scrnos) {
        openAPI.SetRealRemove(scrno, _T("ALL"));
    }

    m_btStop.EnableWindow(FALSE);

    ((CWnd*)GetDlgItem(IDC_STATIC_DISP2))->SetWindowText(_T("Waiting for remaining events..."));
    DisplayUpdatedTime();

    // Timer for auto flush when events stop arriving for more than 3 seconds
    SetTimer(flushTimerID, flushWait_ms, FlushTimerCallback);
}

void CKiwoomScribeDlg::OnBnClickedCancel()
{
    // TODO: Add your control notification handler code here

    CDialogEx::OnCancel();

    RECT rect;
    GetWindowRect(&rect);
    CString str;
    str.Format(_T("%d %d"), rect.left, rect.top);
    ::WritePrivateProfileString(_T("SAVEDATA"), _T("POS"), str, configFile);

    // ofstreams closed automatically in destructors

    // This very often causes infinite loop in the background after app closes
    // openAPI.CommTerminate();
    // PostQuitMessage(0);

    // Force exit without invoking openAPI.CommTerminate() in destructors
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX);
    abort();
}

VOID CALLBACK CKiwoomScribeDlg::FlushTimerCallback(HWND hwnd, UINT uMsg, UINT_PTR idEvent, DWORD dwTime)
{
    auto FlushAll = [](std::map<std::string, Security> &map) {
        for (auto it = std::begin(map), end = std::end(map); it != end; ++it) {
            if (it->second.    fs_tr .is_open()) it->second.    fs_tr .flush();
            if (it->second.    fs_tb .is_open()) it->second.    fs_tb .flush();
            if (it->second.elw.fs_th .is_open()) it->second.elw.fs_th .flush();
            if (it->second.etf.fs_nav.is_open()) it->second.etf.fs_nav.flush();
        }
    };

    FlushAll(this_->map_KOSPI);
    FlushAll(this_->map_ELW  );
    FlushAll(this_->map_ETF  );

    if (this_->fs_KOSPI200.is_open()) this_->fs_KOSPI200.flush();

    ((CWnd*)(this_->GetDlgItem(IDC_STATIC_DISP2)))->SetWindowText(_T("Flushed ofstreams"));
    this_->DisplayUpdatedTime();

    this_->KillTimer(this_->flushTimerID);
    
    if (this_->m_btStop.IsWindowEnabled() == FALSE)
    {
        this_->m_btRun.EnableWindow(TRUE);
        this_->m_btCancel.EnableWindow(TRUE);
        this_->m_btFetch.EnableWindow(TRUE);
    }

}

void CKiwoomScribeDlg::DisplayUpdatedTime()
{
    // Display time of last event
    lastTimeStr = CTime::GetCurrentTime().Format("%H:%M:%S/");
    lastTimeStr.Append(lastTbTime);
    ((CWnd*)GetDlgItem(IDC_STATIC_DISP_TIME))->SetWindowText(lastTimeStr);
}

void CKiwoomScribeDlg::OnBnClickedButtonFetch()
{
    system(theApp.path + _T("\\FetchCode\\FetchCode.exe"));
}
