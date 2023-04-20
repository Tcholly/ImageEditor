#include "AskCropFormatLayer.h"

#include <raylib.h>
#include <fmt/core.h>

#include "Variables.h"

namespace AskCropFormatLayer
{
	int width, height;
	Rectangle plus_x, plus_y, minus_x, minus_y;

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
			if (CheckCollisionPointRec(mouse_pos, plus_x))
				Variables::ask_crop_dialog_result.x += 1;
			else if (CheckCollisionPointRec(mouse_pos, minus_x) && Variables::ask_crop_dialog_result.x != 0)
				Variables::ask_crop_dialog_result.x -= 1;
			else if (CheckCollisionPointRec(mouse_pos, plus_y))
				Variables::ask_crop_dialog_result.y += 1;
			else if (CheckCollisionPointRec(mouse_pos, minus_y) && Variables::ask_crop_dialog_result.y != 0)
				Variables::ask_crop_dialog_result.y -= 1;
		}

		return false;
	}

	static void Render()
	{
		DrawRectangle(0, 0, width, height, RED);

		Color plus_x_color = BLUE;
		Color minus_x_color = BLUE;
		Color plus_y_color = BLUE;
		Color minus_y_color = BLUE;

		Vector2 mouse_pos = GetMousePosition();
		if (CheckCollisionPointRec(mouse_pos, plus_x))
			plus_x_color = GRAY;
		else if (CheckCollisionPointRec(mouse_pos, minus_x))
			minus_x_color = GRAY;
		else if (CheckCollisionPointRec(mouse_pos, plus_y))
			plus_y_color = GRAY;
		else if (CheckCollisionPointRec(mouse_pos, minus_y))
			minus_y_color = GRAY;

		DrawRectangleRec(plus_x, plus_x_color);
		DrawRectangleRec(minus_x, minus_x_color);
		DrawRectangleRec(plus_y, plus_y_color);
		DrawRectangleRec(minus_y, minus_y_color);

		Vector2 plus_text_size = MeasureTextEx(GetFontDefault(), "+", 30, 3.0f);
		Vector2 minus_text_size = MeasureTextEx(GetFontDefault(), "-", 30, 3.0f);

		DrawText("+", (int)plus_x.x + (int)plus_x.width / 2 - (int)plus_text_size.x / 2, (int)plus_x.y + (int)plus_x.height / 2 - (int)plus_text_size.y / 2, 30, WHITE);
		DrawText("-", (int)minus_x.x + (int)minus_x.width / 2 - (int)minus_text_size.x / 2, (int)minus_x.y + (int)minus_x.height / 2 - (int)minus_text_size.y / 2, 30, WHITE);

		DrawText("+", (int)plus_y.x + (int)plus_y.width / 2 - (int)plus_text_size.x / 2, (int)plus_y.y + (int)plus_y.height / 2 - (int)plus_text_size.y / 2, 30, WHITE);
		DrawText("-", (int)minus_y.x + (int)minus_y.width / 2 - (int)minus_text_size.x / 2, (int)minus_y.y + (int)minus_y.height / 2 - (int)minus_text_size.y / 2, 30, WHITE);

		std::string x_text = fmt::format("{}", Variables::ask_crop_dialog_result.x);
		Vector2 x_text_size = MeasureTextEx(GetFontDefault(), x_text.c_str(), 30, 3.0f);
		DrawText(x_text.c_str(), width / 2 - (int)x_text_size.x / 2, (int)plus_x.y + (int)plus_x.height / 2 - (int)x_text_size.y / 2, 30, WHITE);

		std::string y_text = fmt::format("{}", Variables::ask_crop_dialog_result.y);
		Vector2 y_text_size = MeasureTextEx(GetFontDefault(), y_text.c_str(), 30, 3.0f);
		DrawText(y_text.c_str(), width / 2 - (int)y_text_size.x / 2, (int)plus_y.y + (int)plus_y.height / 2 - (int)y_text_size.y / 2, 30, WHITE);
	}

	static void OnResize(int _width, int _height)
	{
		width = _width;
		height = _height;

		plus_x.height = 30;
		plus_x.width = width / 5.0f;
		plus_x.x = 3.0f * width / 5.0f; 
		plus_x.y = (height / 5.0f) * 1.5f - 15.0f;

		minus_x.height = 30;
		minus_x.width = width / 5.0f;
		minus_x.x = width / 5.0f; 
		minus_x.y = (height / 5.0f) * 1.5f - 15.0f;

		plus_y.height = 30;
		plus_y.width = width / 5.0f;
		plus_y.x = 3.0f * width / 5.0f; 
		plus_y.y = (height / 5.0f) * 3.5f - 15.0f;

		minus_y.height = 30;
		minus_y.width = width / 5.0f;
		minus_y.x = width / 5.0f; 
		minus_y.y = (height / 5.0f) * 3.5f - 15.0f;
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
