#include <WinSock2.h>
#include <shlobj.h>
#include <Shlwapi.h>
#include "ServTreeView.h"
#include "../resource.h"
#include <CommCtrl.h>
#include <LogLib/winsize.h>
#include "MainView.h"
#include "../LogServMgr.h"

using namespace std;

#define MSG_LOGSERV_ADD     (WM_USER + 6011)
#define MSG_LOGSERV_ALTER   (WM_USER + 6012)

CServTreeDlg::CServTreeDlg() {
    mhWnd = NULL;
    mParent = NULL;
    mTreeCtrl = NULL;
}

CServTreeDlg::~CServTreeDlg() {
}

BOOL CServTreeDlg::CreateDlg(HWND hParent) {
    mParent = hParent;
    mhWnd = CreateDialogParamA(NULL, MAKEINTRESOURCEA(IDD_SERV_TREE), hParent, ServTreeDlgProc, (LPARAM)this);
    CLogServMgr::GetInst()->Register(this);
    return TRUE;
}

HWND CServTreeDlg::GetWindow() {
    return mhWnd;
}

BOOL CServTreeDlg::MoveWindow(int x, int y, int cx, int cy) {
    ::MoveWindow(mhWnd, x, y, cx, cy, TRUE);
    return TRUE;
}

HTREEITEM CServTreeDlg::InsertItem(HTREEITEM parent, const mstring &name, const TreeCtrlParam *param) const {
    TVINSERTSTRUCTA st = {0};
    st.hParent = parent;
    st.hInsertAfter = TVI_LAST;
    st.item.mask = TVIF_TEXT | TVIF_PARAM;
    st.item.hItem = NULL;
    st.item.pszText = (LPSTR)name.c_str();
    st.item.cchTextMax = name.size();
    st.item.lParam = (LPARAM)param;
    return (HTREEITEM)SendMessageA(
        mTreeCtrl,
        TVM_INSERTITEMA,
        0,
        (LPARAM)(LPTV_INSERTSTRUCTA)(&st)
        );
}

BOOL CServTreeDlg::SetItemStat(HTREEITEM treeItem, DWORD statMask) const {
    TVITEMEXA item = {0};
    item.mask = TVIF_STATE | TVIF_HANDLE;
    item.hItem = treeItem;
    item.state = statMask;
    item.stateMask = statMask;
    return SendMessageA(
        (HWND)mTreeCtrl,
        TVM_SETITEMA,
        0,
        (LPARAM)(LPTV_INSERTSTRUCTA)(&item)
        );
}

void CServTreeDlg::OnLogServAdd(const LogServDesc *d) {
    mServDesc.push_back(d);

    SendMessageA(mhWnd, MSG_LOGSERV_ADD, (WPARAM)d, 0);
}

void CServTreeDlg::OnLogServSwitch(const LogServDesc *d1, const LogServDesc *d2) {
}

void CServTreeDlg::OnLogServAlter(const LogServDesc *d) {
    SendMessageA(mhWnd, MSG_LOGSERV_ALTER, (WPARAM)d, 0);
}

INT_PTR CServTreeDlg::OnInitDialog(WPARAM wp, LPARAM lp) {
    mTreeCtrl = GetDlgItem(mhWnd, IDC_SERV_TREE);

    CTL_PARAMS arry[] =
    {
        {NULL, mTreeCtrl, 0, 0, 1, 1},
        {IDC_BTN_SERV_CFG, NULL, 0, 1, 0, 0},
        {IDC_BTN_ADD_PATH, NULL, 0.5, 1, 0, 0},
        {IDC_BTN_REFUSH, NULL, 1, 1, 0, 0}
    };
    SetCtlsCoord(mhWnd, arry, sizeof(arry) / sizeof(CTL_PARAMS));
    return 0;
}

mstring CServTreeDlg::ShowFileDlg(const mstring &initDir) const {
    mstring dir;

    char buff[256];
    buff[0] = 0x00;
    BROWSEINFOA bi = {0};
    bi.hwndOwner = mhWnd;
    bi.pidlRoot = NULL;
    bi.pszDisplayName = buff;
    bi.lpszTitle = "选择要监控的对话框";

    bi.ulFlags = BIF_RETURNONLYFSDIRS | BIF_EDITBOX | BIF_NEWDIALOGSTYLE | BIF_NONEWFOLDERBUTTON;
    bi.lpfn = NULL;
    bi.iImage = 0;

    LPITEMIDLIST idl = SHBrowseForFolderA(&bi);

    char dirBuf[256];
    dirBuf[0] = 0x00;
    if (SHGetPathFromIDListA(idl, dirBuf) && INVALID_FILE_ATTRIBUTES != GetFileAttributesA(dirBuf)){
        return dirBuf;
    }
    return "";
}

INT_PTR CServTreeDlg::OnCommand(WPARAM wp, LPARAM lp) {
    WORD id = LOWORD(wp);

    if (IDC_BTN_ADD_PATH == id)
    {
        mstring dir = ShowFileDlg("");

        if (!dir.empty())
        {
            CLogReceiver::GetInst()->AddPathMonitor(dir);
        }
    }
    return 0;
}

INT_PTR CServTreeDlg::OnNotify(WPARAM wp, LPARAM lp) {
    NMHDR *headr = (NMHDR *)lp;

    HTREEITEM sel = 0;
    TVITEM itm = {0};
    const TreeCtrlParam *desc = NULL;
    if (headr->code == NM_DBLCLK)
    {
        sel = TreeView_GetSelection(mTreeCtrl);

        itm.mask = TVIF_PARAM;
        itm.hItem = sel;
        TreeView_GetItem(mTreeCtrl, &itm);

        desc = (const TreeCtrlParam *)itm.lParam;
        int d = 123;
    } else if (headr->code == NM_RCLICK) {
        sel = TreeView_GetSelection(mTreeCtrl);

        itm.mask = TVIF_PARAM;
        itm.hItem = sel;
        TreeView_GetItem(mTreeCtrl, &itm);

        desc = (const TreeCtrlParam *)itm.lParam;
        if (desc && desc->mNodeType == em_tree_local_root_node)
        {
            int dd = 12345;
        }
    }
    return 0;
}

INT_PTR CServTreeDlg::OnServAddedInternal(const LogServDesc *desc) {
    if (desc->mLogServType == em_log_serv_local)
    {
        TreeCtrlParam *root = new TreeCtrlParam();
        root->mNodeType = em_tree_local_root_node;
        root->mServDesc = desc;
        HTREEITEM t1 = InsertItem(NULL, "本地服务", root);
    }
    return 0;
}

CServTreeDlg::TreeRootCache *CServTreeDlg::GetCacheFromDesc(const LogServDesc *desc) const {
    size_t i = 0;
    for (i = 0 ; i < mTreeCache.size() ; i++)
    {
        if (mTreeCache[i]->mServDesc == desc)
        {
            return mTreeCache[i];
        }
    }
    return NULL;
}

bool CServTreeDlg::IsServInCache(const LogServDesc *desc) const {
    for (vector<TreeRootCache *>::const_iterator it = mTreeCache.begin() ; it != mTreeCache.end() ; it++)
    {
        TreeRootCache *ptr = *it;

        if (ptr->mServDesc == desc)
        {
            return true;
        }
    }
    return false;
}

bool CServTreeDlg::InsertServToCache(const LogServDesc *desc, HTREEITEM hFileLog, HTREEITEM hFileSet) {
    if (IsServInCache(desc))
    {
        return false;
    }

    TreeRootCache *cache = new TreeRootCache();
    cache->mServDesc = desc;
    cache->mFileLogItem = hFileLog;
    cache->mFileSetItem = hFileSet;

    mTreeCache.push_back(cache);
    return true;
}

void CServTreeDlg::OnServTreeUpdate(const LogServDesc *desc) {
}

INT_PTR CServTreeDlg::OnServAlterInternal(const LogServDesc *desc) {
    OnServTreeUpdate(desc);
    return 0;
}

void CServTreeDlg::DeleteChildByName(HTREEITEM parent, const mstring &name) {
    HTREEITEM tmp = (HTREEITEM)SendMessageA(mTreeCtrl, TVM_GETNEXTITEM, TVGN_CHILD, (LPARAM)parent);

    list<HTREEITEM> delSet;
    while (NULL != tmp) {
        TVITEMA itm = {0};
        char buff[256] = {0};

        itm.mask = (TVIF_HANDLE | TVIF_TEXT);
        itm.pszText = buff;
        itm.cchTextMax = sizeof(buff);

        SendMessageA(mTreeCtrl, TVM_GETITEMA, (WPARAM)mTreeCtrl, (LPARAM)&itm);
        if (name == itm.pszText)
        {
            delSet.push_back(tmp);
        }

        tmp = (HTREEITEM)SendMessageA(mTreeCtrl, TVM_GETNEXTITEM, TVGN_NEXT, (LPARAM)tmp);
    }

    for (list<HTREEITEM>::const_iterator it = delSet.begin() ; it != delSet.end() ; it++)
    {
        SendMessageA(mTreeCtrl, TVM_DELETEITEM, 0, (LPARAM)*it);
    }
}

void CServTreeDlg::OnNewLogFiles(const list<mstring> &fileSet) {
}

INT_PTR CServTreeDlg::OnClose(WPARAM wp, LPARAM lp) {
    EndDialog(mhWnd, 0);
    return 0;
}

void CServTreeDlg::InsertPathToFileTree(const mstring &path) {
}

void CServTreeDlg::DeletePathFromFileTree(const mstring &path) {
}

INT_PTR CServTreeDlg::ServTreeDlgProc(HWND hdlg, UINT msg, WPARAM wp, LPARAM lp) {
    static CServTreeDlg *sPtr = NULL;

    if (msg == WM_INITDIALOG)
    {
        sPtr = (CServTreeDlg *)lp;
        sPtr->mhWnd = hdlg;
    }

    if (!sPtr)
    {
        return 0;
    }

    switch (msg)
    {
    case WM_INITDIALOG:
        sPtr->OnInitDialog(wp, lp);
        break;
    case WM_COMMAND:
        sPtr->OnCommand(wp, lp);
        break;
    case WM_NOTIFY:
        sPtr->OnNotify(wp, lp);
        break;
    case MSG_LOGSERV_ADD:
        sPtr->OnServAddedInternal((const LogServDesc *)wp);
        break;
    case MSG_LOGSERV_ALTER:
        sPtr->OnServAlterInternal((const LogServDesc *)wp);
        break;
    case WM_CLOSE:
        sPtr->OnClose(wp, lp);
        break;
    }
    return 0;
}