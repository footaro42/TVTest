#ifndef THEME_H
#define THEME_H


namespace Theme {

	enum GradientType {
		GRADIENT_NORMAL,
		GRADIENT_GLOSSY
	};

	enum GradientDirection {
		DIRECTION_HORZ,
		DIRECTION_VERT
	};

	struct GradientInfo {
		GradientType Type;
		GradientDirection Direction;
		COLORREF Color1;
		COLORREF Color2;

		GradientInfo()
			: Type(GRADIENT_NORMAL)
			, Direction(DIRECTION_HORZ)
			, Color1(RGB(0,0,0))
			, Color2(RGB(0,0,0))
		{
		}
		GradientInfo(GradientType type,GradientDirection dir,COLORREF color1,COLORREF color2)
			: Type(type)
			, Direction(dir)
			, Color1(color1)
			, Color2(color2)
		{
		}
	};

	bool FillGradient(HDC hdc,const RECT *pRect,const GradientInfo *pInfo);

	enum BorderType {
		BORDER_FLAT,
		BORDER_SUNKEN,
		BORDER_RAISED
	};

	bool DrawBorder(HDC hdc,const RECT *pRect,BorderType Type);
}


#endif
