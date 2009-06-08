#ifndef PROGRAM_GUIDE_OPTIONS_H
#define PROGRAM_GUIDE_OPTIONS_H


#include "ProgramGuide.h"
#include "Options.h"


class CProgramGuideOptions : public COptions {
	CProgramGuide *m_pProgramGuide;
	enum { MIN_VIEW_HOURS=1, MAX_VIEW_HOURS=24*7 };
	int m_ViewHours;
	int m_ItemWidth;
	int m_LinesPerHour;
	int m_WheelScrollLines;
	LOGFONT m_Font;
	CProgramGuideToolList m_ToolList;
	void SetDlgItemState();
	void DeleteAllTools();
	static CProgramGuideOptions *GetThis(HWND hDlg);
public:
	CProgramGuideOptions(CProgramGuide *pProgramGuide);
	~CProgramGuideOptions();
	bool Load(LPCTSTR pszFileName);
	bool Save(LPCTSTR pszFileName) const;
	bool GetTimeRange(SYSTEMTIME *pstFirst,SYSTEMTIME *pstLast);
	static BOOL CALLBACK DlgProc(HWND hDlg,UINT uMsg,WPARAM wParam,LPARAM lParam);
};


#endif
