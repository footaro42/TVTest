#ifndef TVTEST_IMAGE_H
#define TVTEST_IMAGE_H


struct ImageSaveInfo {
	LPCTSTR pszFileName;
	LPCTSTR pszFormat;
	LPCTSTR pszOption;
	const BITMAPINFO *pbmi;
	const void *pBits;
	LPCTSTR pszComment;
};

typedef BOOL (WINAPI *SaveImageFunc)(const ImageSaveInfo *pInfo);


#endif
