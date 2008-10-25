#include <windows.h>
#include <tchar.h>
#include "libpng/png.h"
#include "TVTest_Image.h"
#include "PNG.h"
#include "ImageUtil.h"




static void WriteData(png_structp pPNG,png_bytep pbData,png_size_t Length)
{
	HANDLE hFile;
	DWORD dwWrite;

	hFile=(HANDLE)png_get_io_ptr(pPNG);
	if (!WriteFile(hFile,pbData,Length,&dwWrite,NULL) || dwWrite!=Length)
		png_error(pPNG,"Write Error");
}


static void FlushData(png_structp pPNG)
{
	HANDLE hFile;

	hFile=(HANDLE)png_get_io_ptr(pPNG);
	FlushFileBuffers(hFile);
}


bool SavePNGFile(const ImageSaveInfo *pInfo)
{
	HANDLE hFile;
	int Width,Height,BitsPerPixel;
	png_structp pPNG;
	png_infop pPNGInfo;
	int i,nPasses,y;
	int nSrcRowBytes;
	png_bytep pbRow;
	BYTE *pBuff=NULL;

	hFile=CreateFile(pInfo->pszFileName,GENERIC_WRITE,0,NULL,CREATE_ALWAYS,
												FILE_ATTRIBUTE_NORMAL,NULL);
	if (hFile==INVALID_HANDLE_VALUE)
		return false;
	pPNG=png_create_write_struct(PNG_LIBPNG_VER_STRING,NULL,NULL,NULL);
	if (pPNG==NULL) {
		CloseHandle(hFile);
		return false;
	}
	pPNGInfo=png_create_info_struct(pPNG);
	if (pPNGInfo==NULL){
		png_destroy_write_struct(&pPNG,NULL);
		CloseHandle(hFile);
		return false;
	}
	if (setjmp(pPNG->jmpbuf)) {
		png_destroy_write_struct(&pPNG,&pPNGInfo);
		if (pBuff!=NULL)
			delete [] pBuff;
		CloseHandle(hFile);
		return false;
	}
	png_set_write_fn(pPNG,hFile,WriteData,FlushData);
	png_set_compression_level(pPNG,_ttoi(pInfo->pszOption));
	Width=pInfo->pbmi->bmiHeader.biWidth;
	Height=abs(pInfo->pbmi->bmiHeader.biHeight);
	BitsPerPixel=pInfo->pbmi->bmiHeader.biBitCount;
	png_set_IHDR(pPNG,pPNGInfo,Width,Height,
		BitsPerPixel<8?BitsPerPixel:8,
		BitsPerPixel<=8?PNG_COLOR_TYPE_PALETTE:PNG_COLOR_TYPE_RGB,
		//fInterlace?PNG_INTERLACE_NONE:PNG_INTERLACE_ADAM7,
		PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_DEFAULT,PNG_FILTER_TYPE_DEFAULT);
	if (BitsPerPixel<=8) {
		int nColors=1<<BitsPerPixel;
		png_color PNGPalette[256];
		const RGBQUAD *prgb;

		prgb=pInfo->pbmi->bmiColors;
		for (i=0;i<nColors;i++) {
			PNGPalette[i].red=prgb->rgbRed;
			PNGPalette[i].green=prgb->rgbGreen;
			PNGPalette[i].blue=prgb->rgbBlue;
			prgb++;
		}
		png_set_PLTE(pPNG,pPNGInfo,PNGPalette,nColors);
	}
	if (pInfo->pszComment!=NULL) {
		png_text PNGText;

#ifndef UNICODE
		PNGText.text=pInfo->pszComment;
#else
		int Length=WideCharToMultiByte(CP_ACP,0,pInfo->pszComment,-1,NULL,0,NULL,NULL);
		PNGText.text=new char[Length];
		WideCharToMultiByte(CP_ACP,0,pInfo->pszComment,-1,PNGText.text,Length,NULL,NULL);
#endif
		PNGText.key="Comment";
		PNGText.compression=PNG_TEXT_COMPRESSION_NONE;
		png_set_text(pPNG,pPNGInfo,&PNGText,1);
#ifdef UNICODE
		delete [] PNGText.text;
#endif
	}
	png_write_info(pPNG,pPNGInfo);
	if (pPNGInfo->color_type==PNG_COLOR_TYPE_RGB)
		png_set_bgr(pPNG);
	if (pPNGInfo->interlace_type!=0)
		nPasses=png_set_interlace_handling(pPNG);
	else
		nPasses=1;
	nSrcRowBytes=DIB_ROW_BYTES(Width,BitsPerPixel);
	if (BitsPerPixel==32)
		pBuff=new BYTE[Width*3];
	for (i=0;i<nPasses;i++) {
		for (y=0;y<Height;y++) {
			pbRow=(png_bytep)pInfo->pBits+
				(pInfo->pbmi->bmiHeader.biHeight>0?(Height-1-y):y)*nSrcRowBytes;
			if (pBuff!=NULL) {
				int x;
				const BYTE *p;
				BYTE *q;

				p=pbRow;
				q=pBuff;
				for (x=0;x<Width;x++) {
					*q++=p[0];
					*q++=p[1];
					*q++=p[2];
					p+=4;
				}
				pbRow=pBuff;
			}
			png_write_rows(pPNG,&pbRow,1);
		}
	}
	if (pBuff!=NULL) {
		delete [] pBuff;
		pBuff=NULL;
	}
	png_write_end(pPNG,pPNGInfo);
	png_destroy_write_struct(&pPNG,&pPNGInfo);
	CloseHandle(hFile);
	return true;
}
