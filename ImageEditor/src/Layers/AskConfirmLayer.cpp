#include "AskConfirmLayer.h"

#include <Difu/Utils/Logger.h>

#include <raylib.h>
#include <fmt/core.h>

#include "Globals.hpp"
#include "Variables.h"

namespace AskConfirmLayer
{
	static int width, height;
	static Rectangle cancel, confirm;

	static void Load()
	{
	}

	static void Unload()
	{
	}

	static bool Update(float dt)
	{
		(void)dt;

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			Vector2 mouse_pos = GetMousePosition();
			if (CheckCollisionPointRec(mouse_pos, cancel))
			{
				Variables::ask_confirm_dialog_result = false;
				return true;
			}
			else if (CheckCollisionPointRec(mouse_pos, confirm))
			{
				Variables::ask_confirm_dialog_result = true;
				return true;
			}
		}

		return false;
	}

	static void Render()
	{
		Color cancel_color = Colors::BUTTON_NORMAL;
		Color confirm_color = Colors::BUTTON_NORMAL;

		Vector2 mouse_pos = GetMousePosition();

		if (CheckCollisionPointRec(mouse_pos, cancel))
			cancel_color = Colors::BUTTON_HOVER;
		else if (CheckCollisionPointRec(mouse_pos, confirm))
			confirm_color = Colors::BUTTON_HOVER;

		DrawRectangleRec(cancel, cancel_color);
		DrawRectangleRec(confirm, confirm_color);
		
		Vector2 cancel_text_size = MeasureTextEx(GetFontDefault(), "cancel", 30, 3.0f);
		DrawText("cancel", (int)cancel.x + (int)cancel.width / 2 - (int)cancel_text_size.x / 2, (int)cancel.y + (int)cancel.height / 2 - (int)cancel_text_size.y / 2, 30, Colors::BUTTON_TEXT);
		Vector2 confirm_text_size = MeasureTextEx(GetFontDefault(), "confirm", 30, 3.0f);
		DrawText("confirm", (int)confirm.x + (int)confirm.width / 2 - (int)confirm_text_size.x / 2, (int)confirm.y + confirm.height / 2 - (int)confirm_text_size.y / 2, 30, Colors::BUTTON_TEXT);
	}

	static void OnResize(int _width, int _height)
	{
		width = _width;
		height = _height;

		cancel.x = 20.0f;
		cancel.width = 150.0f;
		cancel.height = 40.0f;
		cancel.y = height - 21.0f - 60.0f; 

		confirm.x = width - 170.0f;
		confirm.width = 150.0f;
		confirm.height = 40.0f;
		confirm.y = height - 21.0f - 60.0f;
	}

	Layer GetLayer()
	{
		Layer result;
		
		result.OnLoad = &Load;
		result.OnUnload = &Unload;
		result.OnUpdate = &Update;
		result.OnRender = &Render;
		result.OnResize = &OnResize;

		return result;
	}
}
