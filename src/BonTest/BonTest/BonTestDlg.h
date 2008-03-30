// BonTestDlg.h : ヘッダー ファイル
//

#pragma once

#include "DtvEngine.h"
#include "ConfigData.h"


// CBonTestDlg ダイアログ
class CBonTestDlg : public CDialog, public CDtvEngineHandler
{
// コンストラクション
public:
	CBonTestDlg(CWnd* pParent = NULL);	// 標準コンストラクタ

// ダイアログ データ
	enum { IDD = IDD_BONTEST_DIALOG };

	protected:
	virtual void DoDataExchange(CDataExchange* pDX);	// DDX/DDV サポート

// 実装
protected:
	// 生成された、メッセージ割り当て関数
	virtual BOOL OnInitDialog();
	afx_msg void OnSysCommand(UINT nID, LPARAM lParam);
	afx_msg void OnPaint();
	afx_msg HCURSOR OnQueryDragIcon();
	DECLARE_MESSAGE_MAP()
	
	const DWORD OnDtvEngineEvent(CDtvEngine *pEngine, const DWORD dwEventID, PVOID pParam);
	
	HICON m_hIcon;
	CDtvEngine m_DtvEngine;
	CConfigString m_csRecordPath;
	
public:
	afx_msg void OnDestroy();
	afx_msg void OnTimer(UINT_PTR nIDEvent);
	afx_msg void OnBnClickedReset();
	afx_msg void OnSelChangeChannel();
	afx_msg void OnSelChangeService();

	int m_iChannel;
	int m_iService;
	afx_msg void OnBnClickedStartRecord();
	afx_msg void OnBnClickedStopRecord();
	afx_msg void OnBnClickedBrowsePath();
	afx_msg void OnBnClickedOpenFile();
	afx_msg void OnBnClickedPlay();
	afx_msg void OnBnClickedPause();
};
