// OpenLieroX

// Line
// Created 5/11/06
// Dark Charlie

// code under LGPL


#ifndef __CLINE_H__DEPRECATED_GUI__
#define __CLINE_H__DEPRECATED_GUI__

namespace DeprecatedGUI {

// Line events
enum {
	LIN_NONE=-1
};


class CLine : public CWidget {
public:
	// Constructor
	CLine(int x1, int y1, int dx, int dy, Uint32 col) {
		iX = x1;
		iY = y1;
		iWidth = dx;
		iHeight = dy;
		iColour = col;
	}


private:
	// Attributes
	Uint32	iColour;

public:
	// Methods

	void	Create(void) { }
	void	Destroy(void) { }

	//These events return an event id, otherwise they return -1
	int		MouseOver(mouse_t *tMouse)			{ return LIN_NONE; }
	int		MouseUp(mouse_t *tMouse, int nDown)		{ return LIN_NONE; }
	int		MouseDown(mouse_t *tMouse, int nDown)	{ return LIN_NONE; }
	int		MouseWheelDown(mouse_t *tMouse)		{ return LIN_NONE; }
	int		MouseWheelUp(mouse_t *tMouse)		{ return LIN_NONE; }
	int		KeyDown(UnicodeChar c, int keysym, const ModifiersState& modstate)	{ return LIN_NONE; }
	int		KeyUp(UnicodeChar c, int keysym, const ModifiersState& modstate)	{ return LIN_NONE; }

	DWORD SendMessage(int iMsg, DWORD Param1, DWORD Param2)	{ 
							return 0;
						}
	DWORD SendMessage(int iMsg, const std::string& sStr, DWORD Param) { return 0; }
	DWORD SendMessage(int iMsg, std::string *sStr, DWORD Param)  { return 0; }

	void	ChangeColour(Uint32 col)			{ iColour = col; }

	// Draw the line
	void	Draw(SDL_Surface * bmpDest) {
		DrawLine(bmpDest, iX, iY, iX + iWidth, iY + iHeight, iColour); 
	}

	void	LoadStyle(void) {}

	static CWidget * WidgetCreator( const std::vector< ScriptVar_t > & p, CGuiLayoutBase * layout, int id, int x, int y, int dx, int dy )
	{
		CWidget * w = new CLine( x, y, x+dx, y+dy, p[0].c );
		layout->Add( w, id, x, y, dx, dy );
		return w;
	};
	
	void	ProcessGuiSkinEvent(int iEvent) {};
};

}; // namespace DeprecatedGUI

#endif  //  __CLINE_H__DEPRECATED_GUI__
