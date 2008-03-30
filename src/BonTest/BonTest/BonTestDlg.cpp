// BonTestDlg.cpp : 実装ファイル
//

#include "stdafx.h"
#include "BonTest.h"
#include "BonTestDlg.h"

#ifdef _DEBUG
#define new DEBUG_NEW
#endif


// アプリケーションのバージョン情報に使われる CAboutDlg ダイアログ

class CAboutDlg : public CDialog
{
public:
	CAboutDlg();

// ダイアログ データ
	enum { IDD = IDD_ABOUTBOX };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);    // DDX/DDV サポート

// 実装
protected:
	DECLARE_MESSAGE_MAP()
};

CAboutDlg::CAboutDlg() : CDialog(CAboutDlg::IDD)
{
}

void CAboutDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
}

BEGIN_MESSAGE_MAP(CAboutDlg, CDialog)
END_MESSAGE_MAP()


// CBonTestDlg ダイアログ
CBonTestDlg::CBonTestDlg(CWnd* pParent /*=NULL*/)
	: CDialog(CBonTestDlg::IDD, pParent)
	, m_iChannel(0)
	, m_iService(0)
{
	m_hIcon = AfxGetApp()->LoadIcon(IDR_MAINFRAME);
	
	// INIキー登録
	m_csRecordPath.RegisterKey(CONFSECT_GENERAL, TEXT("RecordPath"), TEXT("C:\\BonTest.ts"));
}

void CBonTestDlg::DoDataExchange(CDataExchange* pDX)
{
	CDialog::DoDataExchange(pDX);
	//{{AFX_DATA_MAP(CBonSampleDlg)
	DDX_CBIndex(pDX, IDC_CHCOMBO, m_iChannel);
	//}}AFX_DATA_MAP
	DDX_CBIndex(pDX, IDC_SVCOMBO, m_iService);
}

BEGIN_MESSAGE_MAP(CBonTestDlg, CDialog)
	ON_WM_SYSCOMMAND()
	ON_WM_PAINT()
	ON_WM_QUERYDRAGICON()
	//}}AFX_MSG_MAP
	ON_WM_DESTROY()
	ON_WM_TIMER()
	ON_BN_CLICKED(IDC_RESET, &CBonTestDlg::OnBnClickedReset)
	ON_CBN_SELCHANGE(IDC_CHCOMBO, &CBonTestDlg::OnSelChangeChannel)
	ON_CBN_SELCHANGE(IDC_SVCOMBO, &CBonTestDlg::OnSelChangeService)
	ON_BN_CLICKED(IDC_STARTRECORD, &CBonTestDlg::OnBnClickedStartRecord)
	ON_BN_CLICKED(IDC_STOPRECORD, &CBonTestDlg::OnBnClickedStopRecord)
	ON_BN_CLICKED(IDC_BROWSEPATH, &CBonTestDlg::OnBnClickedBrowsePath)
	ON_BN_CLICKED(IDC_OPENFILE, &CBonTestDlg::OnBnClickedOpenFile)
	ON_BN_CLICKED(IDC_PLAY, &CBonTestDlg::OnBnClickedPlay)
	ON_BN_CLICKED(IDC_PAUSE, &CBonTestDlg::OnBnClickedPause)
END_MESSAGE_MAP()


// CBonTestDlg メッセージ ハンドラ

BOOL CBonTestDlg::OnInitDialog()
{
	CDialog::OnInitDialog();

	// "バージョン情報..." メニューをシステム メニューに追加します。

	// IDM_ABOUTBOX は、システム コマンドの範囲内になければなりません。
	ASSERT((IDM_ABOUTBOX & 0xFFF0) == IDM_ABOUTBOX);
	ASSERT(IDM_ABOUTBOX < 0xF000);

	CMenu* pSysMenu = GetSystemMenu(FALSE);
	if (pSysMenu != NULL)
	{
		CString strAboutMenu;
		strAboutMenu.LoadString(IDS_ABOUTBOX);
		if (!strAboutMenu.IsEmpty())
		{
			pSysMenu->AppendMenu(MF_SEPARATOR);
			pSysMenu->AppendMenu(MF_STRING, IDM_ABOUTBOX, strAboutMenu);
		}
	}

	// このダイアログのアイコンを設定します。アプリケーションのメイン ウィンドウがダイアログでない場合、
	//  Framework は、この設定を自動的に行います。
//	SetIcon(m_hIcon, TRUE);			// 大きいアイコンの設定
	SetIcon(m_hIcon, FALSE);		// 小さいアイコンの設定

	// TODO: 初期化をここに追加します。

	// 設定復元
	SetDlgItemText(IDC_RECORDPATH, m_csRecordPath);

	// DTVエンジンオープン
	if(m_DtvEngine.OpenEngine(this, GetDlgItem(IDC_VIEW)->GetSafeHwnd())){
		// プレビュー開始
		m_DtvEngine.EnablePreview();
		}

	UpdateData(FALSE);
	SetTimer(1, 500UL, NULL);

	return TRUE;  // フォーカスをコントロールに設定した場合を除き、TRUE を返します。
}

void CBonTestDlg::OnSysCommand(UINT nID, LPARAM lParam)
{
	if ((nID & 0xFFF0) == IDM_ABOUTBOX)
	{
		CAboutDlg dlgAbout;
		dlgAbout.DoModal();
	}
	else
	{
		CDialog::OnSysCommand(nID, lParam);
	}
}

// ダイアログに最小化ボタンを追加する場合、アイコンを描画するための
//  下のコードが必要です。ドキュメント/ビュー モデルを使う MFC アプリケーションの場合、
//  これは、Framework によって自動的に設定されます。

void CBonTestDlg::OnPaint()
{
	if (IsIconic())
	{
		CPaintDC dc(this); // 描画のデバイス コンテキスト

		SendMessage(WM_ICONERASEBKGND, reinterpret_cast<WPARAM>(dc.GetSafeHdc()), 0);

		// クライアントの四角形領域内の中央
		int cxIcon = GetSystemMetrics(SM_CXICON);
		int cyIcon = GetSystemMetrics(SM_CYICON);
		CRect rect;
		GetClientRect(&rect);
		int x = (rect.Width() - cxIcon + 1) / 2;
		int y = (rect.Height() - cyIcon + 1) / 2;

		// アイコンの描画
		dc.DrawIcon(x, y, m_hIcon);
	}
	else
	{
		CDialog::OnPaint();
	}
}

// ユーザーが最小化したウィンドウをドラッグしているときに表示するカーソルを取得するために、
//  システムがこの関数を呼び出します。
HCURSOR CBonTestDlg::OnQueryDragIcon()
{
	return static_cast<HCURSOR>(m_hIcon);
}


void CBonTestDlg::OnDestroy()
{
	CDialog::OnDestroy();

	// TODO: ここにメッセージ ハンドラ コードを追加します。

	KillTimer(1);
	
	// DTVエンジンクローズ
	m_DtvEngine.CloseEngine();
}

void CBonTestDlg::OnTimer(UINT_PTR nIDEvent)
{
	// TODO: ここにメッセージ ハンドラ コードを追加するか、既定の処理を呼び出します。
	
	// 受信レベル取得
	CString csText, csTemp;
	csText.Format(TEXT("受信レベル：　%.2fdB"), m_DtvEngine.m_BonSrcDecoder.GetSignalLevel());
	SetDlgItemText(IDC_STATUS, csText);

	if(GetDlgItem(IDC_VIEW)->GetWindow(GW_CHILD)){
		GetDlgItem(IDC_VIEW)->GetWindow(GW_CHILD)->InvalidateRect(NULL);
		}

	// ステータスバー更新
	csText = TEXT("");
	
	csTemp.Format(TEXT("入力： %lu    "), m_DtvEngine.m_TsPacketParser.GetInputPacketCount());
	csText += csTemp;

	csTemp.Format(TEXT("出力： %lu    "), m_DtvEngine.m_TsPacketParser.GetOutputPacketCount());
	csText += csTemp;

	csTemp.Format(TEXT("エラー： %lu    "), m_DtvEngine.m_TsPacketParser.GetErrorPacketCount());
	csText += csTemp;

	csTemp.Format(TEXT("復号漏れ： %lu    "), m_DtvEngine.m_TsDescrambler.GetScramblePacketCount());
	csText += csTemp;

	csTemp.Format(TEXT("録画サイズ： %.2lfMB"), (double)m_DtvEngine.m_FileWriter.GetWriteSize() / (1024.0 * 1024.0));
	csText += csTemp;

	SetDlgItemText(IDC_INFOBAR, csText);

	CDialog::OnTimer(nIDEvent);
}

void CBonTestDlg::OnBnClickedReset()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	m_DtvEngine.ResetEngine();
}

void CBonTestDlg::OnSelChangeChannel()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	UpdateData(TRUE);

	if(!m_DtvEngine.SetChannel(0U, (WORD)m_iChannel + 13U)){
		::AfxMessageBox(TEXT("チャンネルの切替に失敗しました。"));
		}
		
	CComboBox *pServiceCombo = static_cast<CComboBox *>(GetDlgItem(IDC_SVCOMBO));
	pServiceCombo->ResetContent();
	pServiceCombo->AddString(TEXT("チャンネル変更中..."));
	pServiceCombo->SetCurSel(0);
}

void CBonTestDlg::OnSelChangeService()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	UpdateData(TRUE);

	if(!m_DtvEngine.SetService(m_iService)){
		::AfxMessageBox(TEXT("サービスの切替に失敗しました。"));
		}
}

const DWORD CBonTestDlg::OnDtvEngineEvent(CDtvEngine *pEngine, const DWORD dwEventID, PVOID pParam)
{
	// 完全に暫定
	CProgManager *pProgManager = static_cast<CProgManager *>(pParam);
	CComboBox *pServiceCombo = static_cast<CComboBox *>(GetDlgItem(IDC_SVCOMBO));
	
	pServiceCombo->ResetContent();
	TCHAR szServiceName[1024];
	
	// サービス名更新
	for(WORD wIndex = 0U ; wIndex < pProgManager->GetServiceNum() ; wIndex++){
		if(pProgManager->GetServiceName(szServiceName, wIndex)){
			pServiceCombo->AddString(szServiceName);		
			}
		else{
			pServiceCombo->AddString(TEXT("サービス名取得中..."));
			}
		}	

	pServiceCombo->SetCurSel(pEngine->GetService());

	return 0UL;
}

void CBonTestDlg::OnBnClickedStartRecord()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	
	// ファイル書き込み開始
	if(!m_DtvEngine.m_FileWriter.OpenFile(m_csRecordPath)){
		::AfxMessageBox(TEXT("ファイルのオープンに失敗しました。"));
		return;		
		}
		
	// コントロール状態変更
	GetDlgItem(IDC_STARTRECORD)->EnableWindow(FALSE);
	GetDlgItem(IDC_STOPRECORD)->EnableWindow(TRUE);
}

void CBonTestDlg::OnBnClickedStopRecord()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	
	// ファイル書き込み終了
	m_DtvEngine.m_FileWriter.CloseFile();
	
	// コントロール状態変更
	GetDlgItem(IDC_STARTRECORD)->EnableWindow(TRUE);
	GetDlgItem(IDC_STOPRECORD)->EnableWindow(FALSE);
}

void CBonTestDlg::OnBnClickedBrowsePath()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	
	// ファイル選択
	CFileDialog Dlg(FALSE, TEXT("ts"), m_csRecordPath, OFN_HIDEREADONLY | OFN_OVERWRITEPROMPT, TEXT("MPEG2-TS (*.ts)|*.ts||"));
	if(Dlg.DoModal() != IDOK)return;
	
	// コントロール更新
	m_csRecordPath = Dlg.GetPathName();
	SetDlgItemText(IDC_RECORDPATH, m_csRecordPath);
}

void CBonTestDlg::OnBnClickedOpenFile()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	CFileDialog Dlg(TRUE, TEXT("ts"), NULL, OFN_HIDEREADONLY | OFN_FILEMUSTEXIST, TEXT("TSファイル (*.ts)|*.ts||"));
	if(Dlg.DoModal() != IDOK)return;

	if(!m_DtvEngine.PlayFile(Dlg.GetPathName())){
		::AfxMessageBox(TEXT("ファイルのオープンに失敗しました。"));
		}
}

void CBonTestDlg::OnBnClickedPlay()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	
}

void CBonTestDlg::OnBnClickedPause()
{
	// TODO: ここにコントロール通知ハンドラ コードを追加します。
	m_DtvEngine.StopFile();
}
