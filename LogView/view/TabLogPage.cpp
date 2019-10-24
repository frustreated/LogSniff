#include "TabLogPage.h"
#include "../resource.h"
#include "../../LogLib/winsize.h"
#include "../../LogLib/LogUtil.h"
#include <assert.h>
#include "MainView.h"

using namespace std;

#define MSG_FILTER_RETURN       (WM_USER + 2011)

std::map<HWND, CTabLogPage *> CTabLogPage::msProcCache;
RLocker *CTabLogPage::msLocker = NULL;

CTabLogPage::CTabLogPage() {
    if (NULL == msLocker)
    {
        msLocker = new RLocker();
    }
}

CTabLogPage::~CTabLogPage() {
}

void CTabLogPage::MoveWindow(int x, int y, int cx, int cy) const {
    ::MoveWindow(mHwnd, x, y, cx, cy, TRUE);
}

void CTabLogPage::AppendLog(const mstring &label, const mstring &content) {
    if (label == LABEL_DBG_CONTENT)
    {
        mSyntaxView.PushLog(content);
    } else if (label == LABEL_LOG_CONTENT)
    {
        mSyntaxView.PushLog(content);
    }
}

void CTabLogPage::ClearLog() {
    mSyntaxView.ClearCache();
    mSyntaxView.ClearLogView();
}

INT_PTR CTabLogPage::OnInitDialog(WPARAM wp, LPARAM lp) {
    HWND hwnd = GetHandle();
    mFltCtrl = GetDlgItem(hwnd, IDC_COM_FILTER);
    mCkRegular = GetDlgItem(hwnd, IDC_CK_REGULAR);

    COMBOBOXINFO info = { sizeof(COMBOBOXINFO) };
    GetComboBoxInfo(mFltCtrl, &info);
    mFltEdit = info.hwndItem;

    RECT rtClient = {0};
    GetClientRect(hwnd, &rtClient);
    RECT rtFlt = {0};
    GetWindowRect(mFltCtrl, &rtFlt);
    MapWindowPoints(NULL, hwnd, (LPPOINT)&rtFlt, 2);

    int x = rtFlt.left;
    int y = rtFlt.bottom + 5;
    int width = (rtClient.right - rtClient.left) - 2 * x;
    int high = (rtClient.bottom - rtClient.top) - y - 5;
    mSyntaxView.CreateLogView(hwnd, x, y, width, high);
    mSyntaxView.InitLogBase();

    CTL_PARAMS arry[] = {
        {IDC_COM_FILTER, NULL, 0, 0, 1, 0},
        {IDC_CK_REGULAR, NULL, 1, 0, 0, 0},
        {0, mSyntaxView.GetWindow(), 0, 0, 1, 1}
    };
    SetCtlsCoord(hwnd, arry, RTL_NUMBER_OF(arry));
    return 0;
}

INT_PTR CTabLogPage::OnFilterReturn(WPARAM wp, LPARAM lp) {
    mSyntaxView.SetFilter(GetWindowStrA(mFltEdit));
    return 0;
}

INT_PTR CTabLogPage::OnClose(WPARAM wp, LPARAM lp) {
    return 0;
}

INT_PTR CTabLogPage::MessageProc(UINT msg, WPARAM wp, LPARAM lp) {
    if (WM_INITDIALOG == msg)
    {
        return OnInitDialog(wp, lp);
    }
    else if (MSG_FILTER_RETURN == msg)
    {
        return OnFilterReturn(wp, lp);
    }
    else if (WM_CLOSE == msg)
    {
        return OnClose(wp, lp);
    }
    return 0;
}

INT_PTR CTabLogPage::GetMsgHook(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    if (hwnd == mFltEdit && WM_KEYDOWN == msg)
    {
        if (wp == VK_RETURN)
        {
            SendMessageW(mHwnd, MSG_FILTER_RETURN, 0, 0);
        }
    } else if (WM_RBUTTONDOWN == msg && (hwnd == mSyntaxView.GetWindow()))
    {
        OnPopupMenu();
    }
    return 0;
}