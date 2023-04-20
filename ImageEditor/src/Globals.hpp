// This file is auto generated, DO NOT MODIFY (pwease uwu)
// PLATFORM: RAYLIB
// SOURCE: ImageEditor/Globals.txt
#pragma once

#ifndef RL_COLOR_TYPE
	// Color, 4 components, R8G8B8A8 (32bit)
	typedef struct Color
	{
		unsigned char r;        // Color red value
		unsigned char g;        // Color green value
		unsigned char b;        // Color blue value
		unsigned char a;        // Color alpha value
	} Color;
	#define RL_COLOR_TYPE
#endif

#ifndef PI
	#define PI 3.14159265358979323846f
#endif
#ifndef E
	#define E 2.71828182845904523536f
#endif

namespace Colors
{
	const Color BACKGROUND = {80, 80, 80, 255};
	const Color MENU_BACKGROUND = {250, 244, 237, 255};
	const Color MENU_OUTLINE = {40, 105, 131, 255};
	const Color MENU_HOVER = {215, 130, 126, 255};
	const Color MENU_TEXT = {87, 82, 121, 255};
	const Color MENU_TEXT_HOVER = {250, 244, 237, 255};
	const Color PIECE_OUTLINE = {164, 98, 255, 255};
	const Color SELECTED_PIECE_OUTLINE = {180, 161, 255, 255};
} // namespace Colors

