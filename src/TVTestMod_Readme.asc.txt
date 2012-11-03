-----BEGIN PGP SIGNED MESSAGE-----
Hash: SHA1

TvtPlayの倍速再生時TSドロップ部分で長時間フリーズしないように+起動時フリーズしな
いようにしたTVTest
2012-09-26
By TvtPlay作者

■概要
添付のTVTestMod.zip(2sen/hdus/up0637)やTVTestModBuild.zipのつづきです。同梱の
TVTest.exeをオリジナルのTVTest0.8.0のものと置き換えて利用できます。
TVTestModBuild.zipはデッドロック対策と引きかえに(たぶんシリアライズ区間が極端に
長いために)特にシングルコア環境で動作が重くなる現象がみられたため、
AacDecFilter.cppをさらに修正しました。またベースをTVTest0.8.0に変更しました。
TVTest0.7.23が近々公開停止されそうな気がするので、添付のTVTestModBuild.zipはバイ
ナリを削除しています。

ビルド方法は後述「TVTest0.8.0ビルド手順」を参照してください。
DBCTRADO(http://dbctrado.allalla.com/)からTVTest0.8.0のソースコードを取得し、
diff_src.zipの対応するファイルと置き換えれば同等のものができます。なお、同梱の
TVTest(orig).exeは対照実験のためdiff_src.zipのAacDecFilter.h/.cppのみを置き換え
ずにビルドしたもの、つまりオリジナルのTVTest.exeと同じ動作をするはずのものです。

以下のいずれかの場合に公開を停止する予定です:
1.TVTest0.8.0の公開が停止された(GPLを満たさないため)
2.同梱のTVTest.exeに致命的な不具合がみつかった
3.その他(需要がなくなったなど)

■TVTest0.8.0ビルド手順
DBCTRADOの推奨外のビルド方法なので、この手順でビルドしたTVTestでのみ発生したエラ
ーなどをDBCTRADOの中の人に報告するのはやめてください。

[用意+インストールするもの]
・Visual C++ 2010 Express(以下VC++)
・Windows SDK for Windows 7 (「DirectShow BaseClasses」のビルドのため)
・faad2-2.7.zip (FAAD2 Source http://www.audiocoding.com からDL)
・TVTest_0.8.0.7zにあるTVTest_0.8.0_Src.7z
[手順]
※各プロジェクトをビルドするときは、プロジェクトのプロパティの「ランタイムライブ
  ラリ」の設定を一致させること(でないとLNK2005エラーを起こす)
1. 【「FAAD2」(libfaad.lib)をビルド】
   "faad2-2.7.zip"を展開、"frontend\faad.sln"をVC++で開いて(途中、変換ウィザード
   が出る)ビルドする
2. 【「DirectShow BaseClasses」(strmbasd.lib strmbase.lib)をビルド】
   デフォルトでは "Program Files\Microsoft SDKs\Windows\v7.1\Samples\multimedia\
   directshow\baseclasses" フォルダにある"baseclasses.sln"を開いてビルドする
3. "TVTest_0.8.0.7z"とその中身"TVTest_0.8.0_Src.7z"を展開する
4. "TVTest.sln"をVC++で開く
5. TVTestプロジェクトのプロパティを開き、上記"baseclasses"フォルダを「追加のイン
   クルードディレクトリ」に加える。また、同フォルダにある"Debug"または"Release"
   フォルダ、および"libfaad.lib"が生成されたフォルダを「追加のライブラリディレク
   トリ」に加える
6. "VMR9Renderless.cpp"はATLのCComPtrを使っているので、15行目あたりに下記コード
   を加え、5行目 #include <atlbase.h> をコメントアウト

#ifndef __ATLBASE_H__
template <class T> class CComPtr
{
public:
	CComPtr() : p(NULL) {}
	CComPtr(T *lp) : p(lp) { if (p) p->AddRef(); }
	CComPtr(const CComPtr<T> &other) { p = other.p; if (p) p->AddRef(); }
	~CComPtr() { Release(); }
	T *operator=(T *lp) { Attach(lp); if (p) p->AddRef(); return p; }
	T *operator=(const CComPtr<T> &other) { return *this = other.p; }
	bool operator==(T *lp) const { return p == lp; }
	operator T*() const { return p; }
	T **operator&() { _ASSERT(!p); return &p; }
	T *operator->() const { _ASSERT(p); return p; }
	void Release() { if (p) p->Release(); p = NULL; }
	void Attach(T *lp) { Release(); p = lp; }
	HRESULT CopyTo(T **lpp) { _ASSERT(lpp); if (!lpp) return E_POINTER; *lpp = p; if (p) p->AddRef(); return S_OK; }
	T *p;
};
#endif

7. TVTestプロジェクトについては以上でビルド成功するはず。TVH264も同様にプロパテ
   ィを修正すればビルドできるはず

■SHA-1
f0230c72c40e0f4fd895d20d4e1717317bbe308f *TVTestMod(up0637)(obsolete).zip
e5ffc009e403f5c18c8b435ff2dffb7b331a92a4 *TVTestModBuild(obsolete).zip
b69313ed513f315d2f3f7eb65c0d8c83b4c7574b *diff_src.zip
445444c7050e71b2c47e7131c8ee194bc44084c0 *TVTest(orig).exe
c43eee908cf305fb4dca7b7a83cbfe4b3283ebc1 *TVTest.exe
-----BEGIN PGP SIGNATURE-----
Version: GnuPG v1.4.12 (MingW32)

iQEcBAEBAgAGBQJQYs5/AAoJEOCRC1JRujjtfsQH/1c2X1smbTXUgy0z2adhyWiH
cpfiRsFcXGD0h3kqHvWKNV0XwrIo9QeMx7MhY86ixoD3dtYtbIVlk3a16jZeQ9la
PA1c2SxF0ovmZFNiPucp+NsjKzpxJi3TYj9zzlbKAnc09uiDAcetNA9YGAlw+s57
BXpe3lJXbCJJN96X0P0324eNYKlIR6Ufs6GHvZZjm3y0sqttf06czLoz10tokXNZ
PEZ3VkmmxpMKsVE+ZCB/m8fYxUNNI84cNBF863t31NoDGyDyImP1lo05JRDXk7Tz
G4mPPK2a3YENZ14v5V3yZj+E/fdNLiGhPO4g0GG4UK1K0gZTFLiEFcVdHcSoQF0=
=vRi7
-----END PGP SIGNATURE-----
