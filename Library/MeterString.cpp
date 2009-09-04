/*
  Copyright (C) 2001 Kimmo Pekkola

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
*/

#pragma warning(disable: 4786)
#pragma warning(disable: 4996)

#include "MeterString.h"
#include "Rainmeter.h"
#include "Measure.h"
#include "Error.h"

using namespace Gdiplus;

/*
** CMeterString
**
** The constructor
**
*/
CMeterString::CMeterString(CMeterWindow* meterWindow) : CMeter(meterWindow)
{
	m_Color = RGB(255, 255, 255);
	m_EffectColor = RGB(0, 0, 0);
	m_Effect = EFFECT_NONE;
	m_AutoScale = true;
	m_Align = ALIGN_LEFT;
	m_Font = NULL;
	m_FontFamily = NULL;
	m_Style = NORMAL;
	m_FontSize = 10;
	m_Scale = 1.0;
	m_NoDecimals = true;
	m_Percentual = true;
	m_ClipString = false;
	m_NumOfDecimals = -1;
	m_DimensionsDefined = false;
	m_Angle = 0.0;
}

/*
** ~CMeterString
**
** The destructor
**
*/
CMeterString::~CMeterString()
{
	if(m_Font) delete m_Font;
	if(m_FontFamily) delete m_FontFamily;
}

/*
** GetX
**
** Returns the X-coordinate of the meter
**
*/
int CMeterString::GetX(bool abs)
{
	int x = CMeter::GetX();

	if (!abs) 
	{
		switch(m_Align)
		{
		case ALIGN_CENTER:
			x = x - (m_W / 2);
			break;

		case ALIGN_RIGHT:
			x -= m_W;
			break;

		case ALIGN_LEFT:
			// This is already correct
			break;
		}
	}

	return x;
}


/*
** Initialize
**
** Create the font that is used to draw the text.
**
*/
void CMeterString::Initialize()
{
	CMeter::Initialize();

	if(m_FontFamily) delete m_FontFamily;
	m_FontFamily = new FontFamily(m_FontFace.c_str());
	Status status = m_FontFamily->GetLastStatus();

	//===================================================
	/* Matt King Code */
	// It couldn't find the font family
	// Therefore we look in the privatefontcollection of this meters MeterWindow
	if(Ok != status)
	{
		m_FontFamily = new FontFamily(m_FontFace.c_str(), m_MeterWindow->GetPrivateFontCollection());
		status = m_FontFamily->GetLastStatus();

		// It couldn't find the font family: Log it.
		if(Ok != status)
		{	
			std::wstring error = L"Error: Couldn't load font family: ";
			error += m_FontFace;
			DebugLog(error.c_str());
			
			delete m_FontFamily;
			m_FontFamily = NULL;
		}
		
	}
	/* Matt King  end */
	//===================================================

	FontStyle style = FontStyleRegular;

	switch(m_Style)
	{
	case ITALIC:
		style = FontStyleItalic;
		break;

	case BOLD:
		style = FontStyleBold;
		break;

	case BOLDITALIC:
		style = FontStyleBoldItalic;
		break;
	}

	// Adjust the font size with screen DPI
	HDC dc = GetDC(GetDesktopWindow());
	int dpi = GetDeviceCaps(dc, LOGPIXELSX);
	ReleaseDC(GetDesktopWindow(), dc);

	REAL size = (REAL)m_FontSize * (96.0f / (REAL)dpi);

	if (m_FontFamily)
	{
		if(m_Font) delete m_Font;
		m_Font = new Gdiplus::Font(m_FontFamily, size, style);
	}
	else
	{
		if(m_Font) delete m_Font;
		m_Font = new Gdiplus::Font(FontFamily::GenericSansSerif(), size, style);
	}

	status = m_Font->GetLastStatus();
	if(Ok != status)
	{
	    throw CError(std::wstring(L"Unable to create font: ") + m_FontFace, __LINE__, __FILE__);
	}
}

/*
** ReadConfig
**
** Read the meter-specific configs from the ini-file.
**
*/
void CMeterString::ReadConfig(const WCHAR* section)
{
	WCHAR tmpName[256];

	// Store the current font values so we know if the font needs to be updated
	std::wstring oldFontFace = m_FontFace;
	int oldFontSize = m_FontSize;
	TEXTSTYLE oldStyle = m_Style;

	// Read common configs
	CMeter::ReadConfig(section);

	CConfigParser& parser = m_MeterWindow->GetParser();

	// Check for extra measures
	int i = 2;
	bool loop = true;
	do 
	{
		swprintf(tmpName, L"MeasureName%i", i);
		std::wstring measure = parser.ReadString(section, tmpName, L"");
		if (!measure.empty())
		{
			m_MeasureNames.push_back(measure);
		}
		else
		{
			loop = false;
		}
		i++;
	} while(loop);
	
	
	m_Color = parser.ReadColor(section, L"FontColor", parser.ReadColor(m_StyleName.c_str(), L"FontColor", Color::Black));
	m_EffectColor = parser.ReadColor(section, L"FontEffectColor", parser.ReadColor(m_StyleName.c_str(), L"FontEffectColor", Color::Black));

	m_Prefix = parser.ReadString(section, L"Prefix", parser.ReadString(m_StyleName.c_str(), L"Prefix", L"").c_str(),true,true);
	m_Postfix = parser.ReadString(section, L"Postfix", parser.ReadString(m_StyleName.c_str(), L"Postfix", L"").c_str(),true,true);
	m_Text = parser.ReadString(section, L"Text", parser.ReadString(m_StyleName.c_str(), L"Text", L"").c_str(),true,true);

	m_Percentual = 0!=parser.ReadInt(section, L"Percentual", 0!=parser.ReadInt(m_StyleName.c_str(), L"Percentual", 0));
	m_AutoScale = 0!=parser.ReadInt(section, L"AutoScale", 0!=parser.ReadInt(m_StyleName.c_str(), L"AutoScale", 0));
	m_ClipString = 0!=parser.ReadInt(section, L"ClipString", 0!=parser.ReadInt(m_StyleName.c_str(), L"ClipString", 0));

	m_FontSize = parser.ReadFormula(section, L"FontSize", parser.ReadFormula(m_StyleName.c_str(), L"FontSize", 10));

	m_NumOfDecimals = parser.ReadInt(section, L"NumOfDecimals", parser.ReadInt(m_StyleName.c_str(), L"NumOfDecimals", -1));

	m_Angle = (Gdiplus::REAL)parser.ReadFloat(section, L"Angle", (Gdiplus::REAL)parser.ReadFloat(m_StyleName.c_str(), L"Angle",0.0));

	std::wstring scale;
	scale = parser.ReadString(section, L"Scale", parser.ReadString(m_StyleName.c_str(), L"Scale", L"1").c_str(),true,true);

	if (wcschr(scale.c_str(), '.') == NULL)
	{
		m_NoDecimals = true;
	}
	else
	{
		m_NoDecimals = false;
	}
	m_Scale = wcstod(scale.c_str(), NULL);

	m_FontFace = parser.ReadString(section, L"FontFace", parser.ReadString(m_StyleName.c_str(), L"FontFace", L"Arial").c_str(),true,true);


	std::wstring align;
	align = parser.ReadString(section, L"StringAlign", parser.ReadString(m_StyleName.c_str(), L"StringAlign", L"LEFT").c_str(),true,true);

	if(_wcsicmp(align.c_str(), L"LEFT") == 0)
	{
		m_Align = ALIGN_LEFT;
	}
	else if(_wcsicmp(align.c_str(), L"RIGHT") == 0)
	{
		m_Align = ALIGN_RIGHT;
	}
	else if(_wcsicmp(align.c_str(), L"CENTER") == 0)
	{
		m_Align = ALIGN_CENTER;
	}
	else
	{
        throw CError(std::wstring(L"No such StringAlign: ") + align, __LINE__, __FILE__);
	}

	std::wstring style;
	style = parser.ReadString(section, L"StringStyle", parser.ReadString(m_StyleName.c_str(), L"StringStyle", L"NORMAL").c_str(),true,true);
	
	if(_wcsicmp(style.c_str(), L"NORMAL") == 0)
	{
		m_Style = NORMAL;
	}
	else if(_wcsicmp(style.c_str(), L"BOLD") == 0)
	{
		m_Style = BOLD;
	}
	else if(_wcsicmp(style.c_str(), L"ITALIC") == 0)
	{
		m_Style = ITALIC;
	}
	else if(_wcsicmp(style.c_str(), L"BOLDITALIC") == 0)
	{
		m_Style = BOLDITALIC;
	}
	else
	{
        throw CError(std::wstring(L"No such StringStyle: ") + style, __LINE__, __FILE__);
	}

	std::wstring effect;
	effect = parser.ReadString(section, L"StringEffect", parser.ReadString(m_StyleName.c_str(), L"StringEffect", L"NONE").c_str(),true,true);
	
	if(_wcsicmp(effect.c_str(), L"NONE") == 0)
	{
		m_Effect = EFFECT_NONE;
	}
	else if(_wcsicmp(effect.c_str(), L"SHADOW") == 0)
	{
		m_Effect = EFFECT_SHADOW;
	}
	else if(_wcsicmp(effect.c_str(), L"BORDER") == 0)
	{
		m_Effect = EFFECT_BORDER;
	}
	else
	{
        throw CError(std::wstring(L"No such StringEffect: ") + effect, __LINE__, __FILE__);
	}

	if (-1 != parser.ReadInt(section, L"W", -1) && -1 != parser.ReadInt(section, L"H", -1))
	{
		m_DimensionsDefined = true;
	}

	if (m_Initialized &&
		(oldFontFace != m_FontFace ||
		oldFontSize != m_FontSize ||
		oldStyle != m_Style))
	{
		Initialize();	// Recreate the font
	}
}

/*
** Update
**
** Updates the value(s) from the measures.
**
*/



/*
** Update
**
** Updates the value(s) from the measures.
**
*/
bool CMeterString::Update()
{
	if (CMeter::Update())
	{
		std::vector<std::wstring> stringValues;

		int decimals = (m_NoDecimals && !m_AutoScale) ? 0 : 1;
		if (m_NumOfDecimals != -1) decimals = m_NumOfDecimals;

		if (m_Measure) stringValues.push_back(m_Measure->GetStringValue(m_AutoScale, m_Scale, decimals, m_Percentual));

		// Get the values for the other measures
		for (size_t i = 0; i < m_Measures.size(); i++)
		{
			stringValues.push_back(m_Measures[i]->GetStringValue(m_AutoScale, m_Scale, decimals, m_Percentual));
		}

		// Create the text
		m_String = m_Prefix;
		if (m_Text.empty())
		{
			if (stringValues.size() > 0)
			{
				m_String += stringValues[0]; 
			}
		}
		else
		{
			WCHAR buffer[256];
			// Create the actual text (i.e. replace %1, %2, .. with the measure texts)
			std::wstring tmpText = m_Text;

			for (size_t i = 0; i < stringValues.size(); i++)
			{
				wsprintf(buffer, L"%%%i", i + 1);

				size_t start = 0;
				size_t pos = std::wstring::npos;

				do 
				{
					pos = tmpText.find(buffer, start);
					if (pos != std::wstring::npos)
					{
						tmpText.replace(tmpText.begin() + pos, tmpText.begin() + pos + wcslen(buffer), stringValues[i]);
						start = pos + stringValues[i].length();
					}
				} while(pos != std::wstring::npos);
			}

			m_String += tmpText;
		}
		m_String += m_Postfix;

		if (!m_DimensionsDefined)
		{
			// Calculate the text size
			RectF rect;
			Graphics graphics(m_MeterWindow->GetDoubleBuffer());
			DrawString(graphics, &rect);
			m_W = (int)rect.Width;
			m_H = (int)rect.Height;
		}

		return true;
	}
	return false;
}

/*
** Draw
**
** Draws the meter on the double buffer
**
*/
bool CMeterString::Draw(Graphics& graphics)
{
	if(!CMeter::Draw(graphics)) return false;

	return DrawString(graphics, NULL);
}

/*
** DrawString
**
** Draws the string or calculates it's size
**
*/
bool CMeterString::DrawString(Graphics& graphics, RectF* rect)
{
	StringFormat stringFormat;
	
	if (m_AntiAlias)
	{
		graphics.SetTextRenderingHint(TextRenderingHintAntiAlias);
	}
	else
	{
		graphics.SetTextRenderingHint(TextRenderingHintSingleBitPerPixelGridFit);
	}

	switch(m_Align)
	{
	case ALIGN_CENTER:
		stringFormat.SetAlignment(StringAlignmentCenter);
		break;

	case ALIGN_RIGHT:
		stringFormat.SetAlignment(StringAlignmentFar);
		break;

	case ALIGN_LEFT:
		stringFormat.SetAlignment(StringAlignmentNear);
		break;
	}

	REAL x = (REAL)GetX();
	REAL y = (REAL)GetY();

	if (rect)
	{
		PointF pos(x, y);
		graphics.MeasureString(m_String.c_str(), -1, m_Font, pos, rect);
	}
	else
	{
		RectF rc((REAL)x, (REAL)y, (REAL)m_W, (REAL)m_H);

		if (m_ClipString)
		{
			stringFormat.SetTrimming(StringTrimmingEllipsisCharacter);
		}
		else
		{
			stringFormat.SetTrimming(StringTrimmingNone);
			stringFormat.SetFormatFlags(StringFormatFlagsNoClip | StringFormatFlagsNoWrap);
		}

		REAL angle = m_Angle * 180.0f / 3.14159265f;		// Convert to degrees
		graphics.TranslateTransform((Gdiplus::REAL)CMeter::GetX(), y);
		graphics.RotateTransform(angle);
		graphics.TranslateTransform(-(Gdiplus::REAL)CMeter::GetX(), -y);

		if (m_Effect == EFFECT_SHADOW)
		{
			SolidBrush solidBrush(m_EffectColor);
			RectF rcEffect(rc);
			rcEffect.Offset(1, 1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
		}
		else if (m_Effect == EFFECT_BORDER)
		{
			SolidBrush solidBrush(m_EffectColor);
			RectF rcEffect(rc);
			rcEffect.Offset(0, 1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(1, -1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(-1, -1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
			rcEffect.Offset(-1, 1);
			graphics.DrawString(m_String.c_str(), -1, m_Font, rcEffect, &stringFormat, &solidBrush);
		}
		
		SolidBrush solidBrush(m_Color);
		graphics.DrawString(m_String.c_str(), -1, m_Font, rc, &stringFormat, &solidBrush);

		graphics.ResetTransform();
	}

	return true;
}

/*
** BindMeasure
**
** Overridden method. The string meters need not to be bound on anything
**
*/
void CMeterString::BindMeasure(std::list<CMeasure*>& measures)
{
	if (m_MeasureName.empty()) return;	// Allow NULL measure binding

	CMeter::BindMeasure(measures);

	std::vector<std::wstring>::iterator j = m_MeasureNames.begin();
	for (; j != m_MeasureNames.end(); j++)
	{
		// Go through the list and check it there is a secondary measures for us
		std::list<CMeasure*>::iterator i = measures.begin();
		for( ; i != measures.end(); i++)
		{
			if(_wcsicmp((*i)->GetName(), (*j).c_str()) == 0)
			{
				m_Measures.push_back(*i);
				break;
			}
		}

		if (i == measures.end())
		{
	        throw CError(std::wstring(L"The meter [") + m_Name + L"] cannot be bound with [" + (*j) + L"]!", __LINE__, __FILE__);
		}
	}
}
