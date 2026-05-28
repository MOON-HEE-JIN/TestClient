#include "CMFCThread.h"

namespace
{
    constexpr int kWindowWidth = 300;
    constexpr int kWindowHeight = 300;

    constexpr UINT ID_LOG_EDIT = 1001;
    constexpr UINT ID_INPUT_EDIT = 1002;

    class CCheatWindow final : public CFrameWnd
    {
    public:
        BOOL CreateCheatWindow()
        {
            return Create(
                nullptr,
                _T("CMFC Cheat Console"),
                WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX,
                CRect(450, 100, 450 + kWindowWidth, 100 + kWindowHeight)
            );
        }

    protected:
        afx_msg int OnCreate(LPCREATESTRUCT lpCreateStruct)
        {
            if (CFrameWnd::OnCreate(lpCreateStruct) == -1)
                return -1;

            CRect rc;
            GetClientRect(&rc);

            const int splitY = (rc.Height() * 9) / 10;

            m_log.Create(
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_MULTILINE | ES_READONLY | WS_VSCROLL,
                CRect(0, 0, rc.Width(), splitY),
                this,
                ID_LOG_EDIT
            );

            m_input.Create(
                WS_CHILD | WS_VISIBLE | WS_BORDER | ES_AUTOHSCROLL,
                CRect(0, splitY, rc.Width(), rc.Height()),
                this,
                ID_INPUT_EDIT
            );

            m_log.SetWindowText(_T("[INFO] Cheat log ready\r\n"));

            PostMessage(WM_APP + 1);
            return 0;
        }

        afx_msg void OnSize(UINT nType, int cx, int cy)
        {
            CFrameWnd::OnSize(nType, cx, cy);

            if (!::IsWindow(m_log.GetSafeHwnd()) || !::IsWindow(m_input.GetSafeHwnd()))
                return;

            const int splitY = (cy * 9) / 10;

            m_log.MoveWindow(0, 0, cx, splitY);
            m_input.MoveWindow(0, splitY, cx, cy - splitY);
        }

        afx_msg void OnDestroy()
        {
            CFrameWnd::OnDestroy();
            PostQuitMessage(0);
        }

        afx_msg LRESULT OnSetInitialFocus(WPARAM, LPARAM)
        {
            if (::IsWindow(m_input.GetSafeHwnd()))
                m_input.SetFocus();

            return 0;
        }

        BOOL PreTranslateMessage(MSG* pMsg) override
        {
            if (pMsg->message == WM_KEYDOWN &&
                pMsg->wParam == VK_RETURN &&
                ::GetFocus() == m_input.GetSafeHwnd())
            {
                CString cmd;
                m_input.GetWindowText(cmd);
                cmd.Trim();

                if (!cmd.IsEmpty())
                {
                    CString oldLog;
                    m_log.GetWindowText(oldLog);

                    oldLog += _T("[CHEAT] ");
                    oldLog += cmd;
                    oldLog += _T("\r\n");

                    m_log.SetWindowText(oldLog);
                    m_input.SetWindowText(_T(""));
                }

                return TRUE;
            }

            return CFrameWnd::PreTranslateMessage(pMsg);
        }

        DECLARE_MESSAGE_MAP()

    private:
        CEdit m_log;
        CEdit m_input;
    };

    BEGIN_MESSAGE_MAP(CCheatWindow, CFrameWnd)
        ON_WM_CREATE()
        ON_WM_SIZE()
        ON_WM_DESTROY()
        ON_MESSAGE(WM_APP + 1, &CCheatWindow::OnSetInitialFocus)
    END_MESSAGE_MAP()


    class CCheatThread final : public CWinThread
    {
        DECLARE_DYNCREATE(CCheatThread)

    public:
        BOOL InitInstance() override
        {
            CCheatWindow* pWnd = new CCheatWindow();

            if (!pWnd->CreateCheatWindow())
            {
                delete pWnd;
                return FALSE;
            }

            m_pMainWnd = pWnd;

            pWnd->ShowWindow(SW_SHOW);
            pWnd->UpdateWindow();

            return TRUE;
        }
    };

    IMPLEMENT_DYNCREATE(CCheatThread, CWinThread)
}

CWinThread* StartMFCThread()
{
    CWinThread* pThread = AfxBeginThread(
        RUNTIME_CLASS(CCheatThread),
        THREAD_PRIORITY_NORMAL,
        0,
        CREATE_SUSPENDED
    );

    if (!pThread)
        return nullptr;

    pThread->m_bAutoDelete = FALSE;
    pThread->ResumeThread();

    return pThread;
}