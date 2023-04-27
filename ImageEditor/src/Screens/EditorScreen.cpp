#include "EditorScreen.h"

#include <Difu/WindowManagement/WindowManager.h>
#include <Difu/ECS/ECSManager.h>
#include <Difu/ECS/Components.h>
#include <Difu/LayerManagement/Layer.h>
#include <Difu/Utils/Logger.h>

#include <cstdint>
#include <limits>
#include <vector>
#include <map>
#include <algorithm>
#include <cmath>

#include <fmt/core.h>
#include <raylib.h>
#include <raymath.h>
#include <nfd.hpp>

#include "Globals.hpp"
#include "Variables.h"
#include "Utils/ConsoleLog.h"

#include "Layers/AskConfirmLayer.h"
#include "Layers/AskCropFormatLayer.h"

#define CAMERA_SPEED 300

enum SubMenuType
{
	MENU_SAVE,
	MENU_OPEN,
	MENU_QUIT,
	MENU_CROP,
	MENU_BASE_BAR,
	MENU_NONE
};

struct SourceDestinationPair
{
	Rectangle source;
	Rectangle destination;
};

struct ImagePiece
{
	std::vector<SourceDestinationPair> sources_dests;
	Vector2 first_piece_pos;
};


struct MenuItem
{
	bool is_open = false;
	std::string name;
	std::map<SubMenuType, std::string> items;
};

namespace EditorScreen
{
	static Texture2D image;
	static ECS::Entity camera;

	static std::vector<ImagePiece> pieces;
	static Vector2 previous_mouse_pos = {0.0f, 0.0f};
	static int selected_piece = -1;

	static std::pair<int, int> combine_pieces = {-1, -1};
	static bool ask_combine = false;
	
	static int crop_piece = -1;
	static bool ask_crop = false;

	static Layer ask_confirm_layer;
	static Layer ask_crop_format_layer;

	static std::vector<MenuItem> menu;

	static ConsoleLog console_log;
	static void Log(Logger::LogLevel level, std::string message)
	{
		Color color;

		switch (level) 
		{
			case Logger::LogLevel::LOG_LEVEL_DEBUG:
				color = GRAY;
				break;

			case Logger::LogLevel::LOG_LEVEL_INFO:
				color = GREEN;
				break;

			case Logger::LogLevel::LOG_LEVEL_WARNING:
				color = ORANGE;
				break;
		
			case Logger::LogLevel::LOG_LEVEL_ERROR:
				color = RED;
				break;

			default:
				color = VIOLET;
				LOG_WARN("UNREACHABLE: log level is unknown: {}", (int)level);
				break;
		}

		console_log.Print(message, color);
	}

	void Load()
	{
		SetTargetFPS(60);

		camera = ECS::CreateEntity("camera");
		auto& camera_component = camera.AddComponent<Camera2DComponent>();
		camera_component.is_primary = true;
		camera_component.camera.zoom = 1.0f;

		WindowManager::SetBackgroundColor(Colors::BACKGROUND);

		MenuItem file_menu;
		file_menu.name = "File";
		file_menu.items[SubMenuType::MENU_SAVE] = "Save";
		file_menu.items[SubMenuType::MENU_OPEN] = "Open";
		file_menu.items[SubMenuType::MENU_QUIT] = "Quit";

		MenuItem edit_menu;
		edit_menu.name = "Edit";
		edit_menu.items[SubMenuType::MENU_CROP] = "Crop";

		menu.emplace_back(file_menu);
		menu.emplace_back(edit_menu);

		ask_confirm_layer = AskConfirmLayer::GetLayer();
		ask_confirm_layer.Load();

		ask_crop_format_layer = AskCropFormatLayer::GetLayer();
		ask_crop_format_layer.Load();

		console_log.Load({0.0f, 0.0f, 1.0f, 1.0f}, 5.0f);
		Logger::Bind(&Log);

		NFD::Init();
	}

	void Unload()
	{
		UnloadTexture(image);
		ask_confirm_layer.Unload();
		ask_crop_format_layer.Unload();
		console_log.Unload();
		NFD::Quit();
	}

	void LoadFile(const std::string& filepath)
	{
		UnloadTexture(image);
		pieces.clear();
		image = LoadTexture(filepath.c_str());
		SetTextureFilter(image, TEXTURE_FILTER_BILINEAR);
		SetTextureFilter(image, TEXTURE_FILTER_TRILINEAR);

		Vector2 window_size = WindowManager::GetWindowSize();
		Rectangle image_source = {0.0f, 0.0f, (float)image.width, (float)image.height};
		Rectangle image_dest = {0.0f, 0.0f, (float)image.width, (float)image.height};
		Vector2 image_pos = {window_size.x / 2.0f - image.width / 2.0f, window_size.y / 2.0f - image.height / 2.0f};
		ImagePiece image_piece;
		image_piece.sources_dests.push_back({image_source, image_dest});
		image_piece.first_piece_pos = image_pos;
		pieces.emplace_back(image_piece);

		selected_piece = -1;
		combine_pieces = {-1, -1};
		crop_piece = -1;
	}

	static SubMenuType GetPressedMenuItem()
	{
		Vector2 mouse_pos = GetMousePosition();
		int offset = 0;
		bool clicked_outside = true;
		for (auto& item : menu)
		{
			int item_width = MeasureText(item.name.c_str(), 20);
			if (CheckCollisionPointRec(mouse_pos, {(float)offset, 0.0f, item_width + 10.0f, 20.0f}))
			{
				for (auto& item_again : menu)
					item_again.is_open = false;

				item.is_open = true;
				clicked_outside = false;
			}

			if (item.is_open)
			{
				int max_lenght = 0;
				for (auto& [menu_type, text] : item.items)
				{
					int text_lenght = MeasureText(text.c_str(), 20);
					if (text_lenght > max_lenght)
						max_lenght = text_lenght; 
				}

				int i = 0;
				for (auto& [menu_type, text] : item.items)
				{
					if (CheckCollisionPointRec(mouse_pos, {(float)offset, 20.0f + i * 20.0f + 1.0f, max_lenght + 8.0f, 20.0f}))
					{
						item.is_open = false;
						return menu_type;
					}

					i++;
				}
			}

			offset += item_width + 10;
		}

		if (clicked_outside)
		{
			for (auto& item_again : menu)
				item_again.is_open = false;
		}
		else 
			return SubMenuType::MENU_BASE_BAR;

		return SubMenuType::MENU_NONE;
	}

	static int GetCollidingPieceIndex(Vector2 pos)
	{
		if (ask_combine)
		{
			for (auto& [source, dest] : pieces[combine_pieces.first].sources_dests)
			{
				Rectangle temp_dest = dest;
				temp_dest.x += pieces[combine_pieces.first].first_piece_pos.x;
				temp_dest.y += pieces[combine_pieces.first].first_piece_pos.y;
				if (CheckCollisionPointRec(pos, temp_dest))
					return combine_pieces.first;
			}

			for (auto& [source, dest] : pieces[combine_pieces.second].sources_dests)
			{
				Rectangle temp_dest = dest;
				temp_dest.x += pieces[combine_pieces.second].first_piece_pos.x;
				temp_dest.y += pieces[combine_pieces.second].first_piece_pos.y;
				if (CheckCollisionPointRec(pos, temp_dest))
					return combine_pieces.second;
			}

			return -1;
		}

		for (int i = (int)pieces.size() - 1; i >= 0; i--)
		{
			for (auto& [source, dest]: pieces[i].sources_dests)
			{
				Rectangle temp_dest = dest;
				temp_dest.x += pieces[i].first_piece_pos.x;
				temp_dest.y += pieces[i].first_piece_pos.y;
				if (CheckCollisionPointRec(pos, temp_dest))
					return i;
			}
		}

		return -1;
	}

	// Relative to first_piece_pos
	Rectangle GetBounds(const ImagePiece& piece)
	{
		float top = MAXFLOAT;
		float bottom = -MAXFLOAT;
		float left = MAXFLOAT;
		float right = -MAXFLOAT;
		for (auto& [source, dest]: piece.sources_dests)
		{
			if (dest.x < left)
				left = dest.x;
			if (dest.y < top)
				top = dest.y;
			if (dest.x + dest.width > right)
				right = dest.x + dest.width;
			if (dest.y + dest.height > bottom)
				bottom = dest.y + dest.height;
		}

		return {left, top, right - left, bottom - top};
	}

	void DrawPiece(const ImagePiece& piece, bool selected, bool serialize = false)
	{
		Camera2D ecs_camera = ECS::GetPrimaryCamera();
		Rectangle bounds = GetBounds(piece);

		for (auto [source, dest]: piece.sources_dests)
		{
			if (serialize)
			{
				dest.x -= bounds.x;
				dest.y -= bounds.y;
			}
			else
			{
				dest.x += piece.first_piece_pos.x;
				dest.y += piece.first_piece_pos.y;
				
				Rectangle outline = dest;
				float offset = 1.0f / ecs_camera.zoom;
				outline.x -= offset;
				outline.y -= offset;
				outline.width += 2.0f * offset;
				outline.height += 2.0f * offset;
				DrawRectangleRec(outline, selected ? Colors::SELECTED_PIECE_OUTLINE : Colors::PIECE_OUTLINE);
			}
			DrawTexturePro(image, source, dest, {0.0f, 0.0f}, 0.0f, WHITE);
		}
	}

	bool HandleMenu()
	{
		SubMenuType clicked_item = GetPressedMenuItem();
		switch (clicked_item)
		{
			case SubMenuType::MENU_SAVE:
				{
					if (selected_piece < 0)
					{
						Logger::Warn("No piece selected");
						break;
					}

					if (!IsTextureReady(image))
					{
						Logger::Warn("No image loaded");
						break;
					}

					NFD::UniquePath out_path;
					nfdfilteritem_t filter_item[1] = {{"Image File", "png"}};
					std::string path = "image.png";
					nfdresult_t result = NFD::SaveDialog(out_path, filter_item, 1, nullptr, path.c_str());

					if (result == NFD_OKAY)
					{
						Rectangle piece_bounds = GetBounds(pieces[selected_piece]);
						RenderTexture out_texture = LoadRenderTexture((int)piece_bounds.width, (int)piece_bounds.height);

						BeginTextureMode(out_texture);
						DrawPiece(pieces[selected_piece], false, true);
						EndTextureMode();

						path = out_path.get();
						Image out_image = LoadImageFromTexture(out_texture.texture);
						ImageFlipVertical(&out_image);
						ExportImage(out_image, path.c_str());
						UnloadImage(out_image);
						UnloadRenderTexture(out_texture);

						Logger::Info("Piece saved successfully as '{}'", path);
					}
					else if (result != NFD_CANCEL)
					{
        				LOG_ERROR(NFD::GetError());
					}
				}
				break;

			case SubMenuType::MENU_OPEN:
				{

					NFD::UniquePath out_path;
					std::string path = "";
					nfdfilteritem_t filter_item[1] = {{"Image File", "png"}};
					nfdresult_t result = NFD::OpenDialog(out_path, filter_item, 1);
					if (result == NFD_OKAY)
					{
						path = out_path.get();
						LoadFile(path);
					}
					else if (result != NFD_CANCEL)
					{
        				LOG_ERROR(NFD::GetError());
					}
				}
				break;

			case SubMenuType::MENU_QUIT:
				WindowManager::CloseWindow();
				break;

			case SubMenuType::MENU_CROP:
				{
					if (pieces.size() < 1)
					{
						Logger::Warn("No image loaded");
						break;
					}

					if (selected_piece < 0)
					{
						Logger::Warn("No piece selected");
						break;
					}
					ask_crop = true;
					crop_piece = selected_piece;
				}
				break;

			case SubMenuType::MENU_BASE_BAR:
				break;

			case SubMenuType::MENU_NONE:
				return false;

			default:
				LOG_WARN("UNIMPLEMENTED HandleMenu() SubMenuType {}", (int)clicked_item);
				break;
		}

		return true;
	}

	float GetCollidingArea(Rectangle rect_1, Rectangle rect_2)
	{
		Rectangle collision = GetCollisionRec(rect_1, rect_2);
		return collision.width * collision.height;
	}

	// Binds the pieces position-wise but keeps them separated
	Vector2 BindPieces(ImagePiece& first, ImagePiece& second)
	{
		Rectangle first_bounds = GetBounds(first);	
		Rectangle second_bounds = GetBounds(second);

		first_bounds.x += first.first_piece_pos.x;
		first_bounds.y += first.first_piece_pos.y;

		second_bounds.x += second.first_piece_pos.x;
		second_bounds.y += second.first_piece_pos.y;

		bool should_bind = true;
		if (IsKeyDown(KEY_LEFT_SHIFT) || IsKeyDown(KEY_RIGHT_SHIFT))
		{
			std::map<float, Rectangle> snap_rects;

			Rectangle rect_tl = second_bounds;
			rect_tl.x = first_bounds.x;
			rect_tl.y = first_bounds.y - rect_tl.height;

			Rectangle rect_tr = second_bounds;
			rect_tr.x = first_bounds.x + first_bounds.width - rect_tr.width;
			rect_tr.y = first_bounds.y - rect_tr.height;

			Rectangle rect_rt = second_bounds;
			rect_rt.x = first_bounds.x + first_bounds.width;
			rect_rt.y = first_bounds.y;

			Rectangle rect_rb = second_bounds;
			rect_rb.x = first_bounds.x + first_bounds.width;
			rect_rb.y = first_bounds.y + first_bounds.height - rect_rb.height;

			Rectangle rect_br = second_bounds;
			rect_br.x = first_bounds.x + first_bounds.width - rect_br.width;
			rect_br.y = first_bounds.y + first_bounds.height;

			Rectangle rect_bl = second_bounds;
			rect_bl.x = first_bounds.x;
			rect_bl.y = first_bounds.y + first_bounds.height;

			Rectangle rect_lb = second_bounds;
			rect_lb.x = first_bounds.x - rect_lb.width;
			rect_lb.y = first_bounds.y + first_bounds.height - rect_lb.height;

			Rectangle rect_lt = second_bounds;
			rect_lt.x = first_bounds.x - rect_lt.width;
			rect_lt.y = first_bounds.y;

			snap_rects[GetCollidingArea(second_bounds, rect_tl)] = rect_tl;
			snap_rects[GetCollidingArea(second_bounds, rect_tr)] = rect_tr;
			snap_rects[GetCollidingArea(second_bounds, rect_rt)] = rect_rt;
			snap_rects[GetCollidingArea(second_bounds, rect_rb)] = rect_rb;
			snap_rects[GetCollidingArea(second_bounds, rect_br)] = rect_br;
			snap_rects[GetCollidingArea(second_bounds, rect_bl)] = rect_bl;
			snap_rects[GetCollidingArea(second_bounds, rect_lb)] = rect_lb;
			snap_rects[GetCollidingArea(second_bounds, rect_lt)] = rect_lt;

			float max_area = 0.0f;
			Rectangle final_rect;
			for (auto& [area, rect] : snap_rects)
			{
				if (area > max_area)
				{
					max_area = area;
					final_rect = rect;
				}
			}

			if (max_area > 0.0f)
			{
				Vector2 offset = Vector2Subtract({final_rect.x, final_rect.y}, {second_bounds.x, second_bounds.y});

				second.first_piece_pos.x += offset.x;
				second.first_piece_pos.y += offset.y;

				should_bind = false;
			}
		}
		else if (should_bind)
		{
			if (second_bounds.x > first_bounds.x + first_bounds.width)
				second.first_piece_pos.x -= second_bounds.x - first_bounds.x - first_bounds.width;
			if (second_bounds.y > first_bounds.y + first_bounds.height)
				second.first_piece_pos.y -= second_bounds.y - first_bounds.y - first_bounds.height;
			if (second_bounds.x + second_bounds.width < first_bounds.x)
				second.first_piece_pos.x -= second_bounds.x + second_bounds.width - first_bounds.x;
			if (second_bounds.y + second_bounds.height < first_bounds.y)
				second.first_piece_pos.y -= second_bounds.y + second_bounds.height - first_bounds.y;
		}

		Vector2 result;
		result.x = second.first_piece_pos.x - first.first_piece_pos.x;
		result.y = second.first_piece_pos.y - first.first_piece_pos.y;
		return result;
	}

	// @param contact_point A vector relative to the position of the first piece where to attach the second piece
	static void CombinePieces(uint32_t first, uint32_t second, Vector2 offset)
	{
		if (first >= pieces.size() || second >= pieces.size())
			return;

		for (auto source_dest_pair : pieces[second].sources_dests)
		{
			source_dest_pair.destination.x += offset.x;
			source_dest_pair.destination.y += offset.y;

			pieces[first].sources_dests.emplace_back(source_dest_pair);
		}

		pieces.erase(pieces.begin() + second);
	}

	// Crop pieces
	// @return true if crop is successfull, false if crop has failed
	static bool CropPiece(uint32_t piece_index, int x_times, int y_times)
	{
		if (x_times < 1 || y_times < 1)
		{
			Logger::Error("Crop failed: values must be both greater than 0: x = {}, y = {}", x_times, y_times);
			return false;
		}

		if (x_times == 1 && y_times == 1)
		{
			Logger::Warn("Crop not done: values are both 1 so it is useless to crop");
			return false;
		}

		Rectangle piece_bounds = GetBounds(pieces[piece_index]);
		piece_bounds.x += pieces[piece_index].first_piece_pos.x;
		piece_bounds.y += pieces[piece_index].first_piece_pos.y;
		Vector2 new_piece_size = {piece_bounds.width / x_times, piece_bounds.height / y_times};
		for (int y = 0; y < y_times; y++)
		{
			for (int x = 0; x < x_times; x++)
			{
				Rectangle new_piece_bounds = {piece_bounds.x + x * new_piece_size.x, piece_bounds.y + y * new_piece_size.y, new_piece_size.x, new_piece_size.y};
				ImagePiece new_piece;
				new_piece.first_piece_pos.x = new_piece_bounds.x;
				new_piece.first_piece_pos.y = new_piece_bounds.y;

				bool found_collision = false;
				for (auto [source, dest] : pieces[piece_index].sources_dests)
				{
					dest.x += pieces[piece_index].first_piece_pos.x;
					dest.y += pieces[piece_index].first_piece_pos.y;
					if (CheckCollisionRecs(dest, new_piece_bounds))
					{
						found_collision = true;
						SourceDestinationPair new_source_dest_pair;

						Rectangle collision_area = GetCollisionRec(dest, new_piece_bounds);

						new_source_dest_pair.source.x = source.x + collision_area.x - dest.x;
						new_source_dest_pair.source.y = source.y + collision_area.y - dest.y;
						new_source_dest_pair.source.width = collision_area.width;
						new_source_dest_pair.source.height = collision_area.height;

						new_source_dest_pair.destination.x = collision_area.x - new_piece_bounds.x;
						new_source_dest_pair.destination.y = collision_area.y - new_piece_bounds.y;
						new_source_dest_pair.destination.width= collision_area.width;
						new_source_dest_pair.destination.height = collision_area.height;

						new_piece.sources_dests.emplace_back(new_source_dest_pair);
					}
				}
				if (found_collision)
					pieces.emplace_back(new_piece);
			}
		}

		return true;
	}

	void Update(float dt)
	{
		if (IsFileDropped())
		{
			FilePathList dropped_files = LoadDroppedFiles();
			LoadFile(dropped_files.paths[0]);
			UnloadDroppedFiles(dropped_files);
		}

		auto& camera_component = camera.GetComponent<Camera2DComponent>();
		camera_component.camera.zoom += GetMouseWheelMove() * camera_component.camera.zoom * 0.1f;
		camera_component.camera.zoom = std::clamp(camera_component.camera.zoom, 0.01f, MAXFLOAT);

		Vector2 direction = {0.0f, 0.0f};
		if (IsKeyDown(KEY_A))
			direction.x -= 1.0f;
		if (IsKeyDown(KEY_D))
			direction.x += 1.0f;
		if (IsKeyDown(KEY_W))
			direction.y -= 1.0f;
		if (IsKeyDown(KEY_S))
			direction.y += 1.0f;

		if (direction.x != 0.0f || direction.y != 0.0f)
			direction = Vector2Normalize(direction);

		camera_component.camera.target.x += direction.x * CAMERA_SPEED * dt / camera_component.camera.zoom;
		camera_component.camera.target.y += direction.y * CAMERA_SPEED * dt / camera_component.camera.zoom;

		Vector2 mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera_component.camera);
		bool is_dialog = ask_combine || ask_crop;

		if (IsMouseButtonPressed(MOUSE_BUTTON_LEFT))
		{
			if (!HandleMenu())
				selected_piece = GetCollidingPieceIndex(mouse_pos);
		}

		if (IsMouseButtonReleased(MOUSE_BUTTON_RIGHT) && !is_dialog)
		{
			if (combine_pieces.first == -1)
				combine_pieces.first = GetCollidingPieceIndex(mouse_pos);
			else
			{
				combine_pieces.second = GetCollidingPieceIndex(mouse_pos);
				if (combine_pieces.second > -1)
					ask_combine = true;
			}
		}

		if (IsMouseButtonDown(MOUSE_BUTTON_LEFT))
		{
			if (!ask_crop || !CheckCollisionPointRec(GetMousePosition(), {(float)ask_crop_format_layer.x, (float)ask_crop_format_layer.y, (float)ask_crop_format_layer.width, (float)ask_crop_format_layer.height}))
			{
				if (selected_piece == -1)
					selected_piece = GetCollidingPieceIndex(mouse_pos);

				Vector2 mouse_delta;
				mouse_delta.x = mouse_pos.x - previous_mouse_pos.x;
				mouse_delta.y = mouse_pos.y - previous_mouse_pos.y;
				if (selected_piece > -1)
				{
					pieces[selected_piece].first_piece_pos.x += mouse_delta.x;
					pieces[selected_piece].first_piece_pos.y += mouse_delta.y;
				}
				else 
				{
					camera_component.camera.target.x -= mouse_delta.x;
					camera_component.camera.target.y -= mouse_delta.y;
					mouse_pos = GetScreenToWorld2D(GetMousePosition(), camera_component.camera);
				}
			}
		}

		// Dialogs ---------------------------------------------------------
		if (ask_combine)
		{
			Vector2 offset = BindPieces(pieces[combine_pieces.first], pieces[combine_pieces.second]);
			if (ask_confirm_layer.Update(dt))
			{
				ask_combine = false;
				if (Variables::ask_confirm_dialog_result)
					CombinePieces(combine_pieces.first, combine_pieces.second, offset);
				combine_pieces = {-1, -1};
			}
		}

		if (ask_crop)
		{
			bool ask_crop_layer_result = ask_crop_format_layer.Update(dt);
			(void)ask_crop_layer_result;
			if (ask_confirm_layer.Update(dt))
			{
				ask_crop = false;
				if (Variables::ask_confirm_dialog_result)
				{
					if (Variables::ask_crop_dialog_result.x == 0 || Variables::ask_crop_dialog_result.y == 0)
					{
						Logger::Warn("Please crop using values that make sense (not x = {} and y = {})", Variables::ask_crop_dialog_result.x, Variables::ask_crop_dialog_result.y);
					}
					else
					{
						if (CropPiece(crop_piece, Variables::ask_crop_dialog_result.x, Variables::ask_crop_dialog_result.y))
							pieces.erase(pieces.begin() + crop_piece);
					}
					Variables::ask_crop_dialog_result = {1, 1};
				}
				crop_piece = -1;
			}
		}

		previous_mouse_pos = mouse_pos;
		console_log.Update(dt);
	}

	void DrawMenu()
	{
		int offset = 0;
		for (auto& item : menu)
		{
			Vector2 mouse_pos = GetMousePosition();
			int item_width = MeasureText(item.name.c_str(), 20);
			bool hover = false;
			if (CheckCollisionPointRec(mouse_pos, {(float)offset, 0.0f, item_width + 10.0f, 20.0f}))
			{
				DrawRectangle(offset, 0, item_width + 10, 20, Colors::MENU_HOVER);
				hover = true;
			}

			DrawText(item.name.c_str(), offset + 5, 1, 20, hover ? Colors::MENU_TEXT_HOVER : Colors::MENU_TEXT);

			if (item.is_open)
			{
				int max_lenght = 0;
				for (auto& [item_type, text] : item.items)
				{
					int text_lenght = MeasureText(text.c_str(), 20);
					if (text_lenght > max_lenght)
						max_lenght = text_lenght; 
				}
				DrawRectangle(offset, 20, max_lenght + 10, 20 * item.items.size(), Colors::MENU_BACKGROUND);
				DrawRectangleLines(offset, 20, max_lenght + 10, 20 * item.items.size() + 1, Colors::MENU_OUTLINE);

				int i = 0;
				hover = false;
				for (auto& [item_type, text] : item.items)
				{
					if (CheckCollisionPointRec(mouse_pos, {(float)offset, 20.0f + i * 20.0f + 1.0f, max_lenght + 8.0f, 20.0f}))
					{
						DrawRectangle(offset + 1, 20 + i * 20 + 1, max_lenght + 8, 19, Colors::MENU_HOVER);
						hover = true;
					}
					DrawText(text.c_str(), offset + 5, 20 + i * 20 + 1, 20, hover ? Colors::MENU_TEXT_HOVER : Colors::MENU_TEXT);
					hover = false;
					i++;
				}
			}

			offset += item_width + 10;
		}
	}

	void Render()
	{
		Vector2 window_size = WindowManager::GetWindowSize();
		auto& camera_component = camera.GetComponent<Camera2DComponent>();
		BeginMode2D(camera_component.camera);

		DrawRectanglePro({window_size.x / 2.0f, window_size.y / 2.0f, 10.0f, 10.0f}, {5.0f, 5.0f}, 0.0f, RED);
		if (IsTextureReady(image))
		{
			if (ask_combine)
			{
				DrawPiece(pieces[combine_pieces.first], false);
				DrawPiece(pieces[combine_pieces.second], false);
			}
			else if (ask_crop)
			{
				DrawPiece(pieces[crop_piece], false);

				Rectangle piece_bounds = GetBounds(pieces[crop_piece]);
				if (Variables::ask_crop_dialog_result.x > 0)
				{
					int x_step = piece_bounds.width / Variables::ask_crop_dialog_result.x;
					for (int x = 0; x < (int)Variables::ask_crop_dialog_result.x + 1; x++)
						DrawLine(piece_bounds.x + pieces[crop_piece].first_piece_pos.x + x * x_step, piece_bounds.y + pieces[crop_piece].first_piece_pos.y, piece_bounds.x + pieces[crop_piece].first_piece_pos.x + x * x_step, piece_bounds.y + pieces[crop_piece].first_piece_pos.y + piece_bounds.height, RED);
				}
				if (Variables::ask_crop_dialog_result.y > 0)
				{
					int y_step = piece_bounds.height / Variables::ask_crop_dialog_result.y;
					for (int y = 0; y < (int)Variables::ask_crop_dialog_result.y + 1; y++)
						DrawLine(piece_bounds.x + pieces[crop_piece].first_piece_pos.x, piece_bounds.y + pieces[crop_piece].first_piece_pos.y + y * y_step, piece_bounds.x + pieces[crop_piece].first_piece_pos.x + piece_bounds.width, piece_bounds.y + pieces[crop_piece].first_piece_pos.y + y * y_step, RED);
				}
			}
			else
				for (int i = 0; i < (int)pieces.size(); i++)
					DrawPiece(pieces[i], i == selected_piece);
		}

		EndMode2D();

		// Dialogs -------------------------------------------------------------
		if (ask_crop)
		{
			ask_crop_format_layer.Render();
			ask_confirm_layer.Render();
		}
		if (ask_combine)
			ask_confirm_layer.Render();

		// Menu ----------------------------------------------------------------
		DrawRectangle(0, 0, GetScreenWidth(), 20, Colors::MENU_BACKGROUND);
		DrawRectangle(0, 20, GetScreenWidth(), 1, Colors::MENU_OUTLINE);
		DrawMenu();	

		DrawRectangle(0, GetScreenHeight() - 20, GetScreenWidth(), 20, Colors::MENU_BACKGROUND);
		DrawRectangle(0, GetScreenHeight() - 21, GetScreenWidth(), 1, Colors::MENU_OUTLINE);

		std::string image_info = fmt::format("Size: {} x {}", image.width, image.height); 
		int image_info_width = MeasureText(image_info.c_str(), 20);
		DrawText(image_info.c_str(), GetScreenWidth() / 2.0f - image_info_width / 2.0f, GetScreenHeight() - 20, 20, Colors::MENU_TEXT);

		std::string zoom_info = fmt::format("{}%", (int)(camera_component.camera.zoom * 100));
		int zoom_info_width = MeasureText(zoom_info.c_str(), 20);
		DrawText(zoom_info.c_str(), window_size.x - zoom_info_width - 5, window_size.y - 20, 20, Colors::MENU_TEXT);

		console_log.Render(false, true);

		DrawFPS(5, GetScreenHeight() - 21 - 30);

	}

	void OnResize(int width, int height)
	{
		auto& camera_component = camera.GetComponent<Camera2DComponent>();
		camera_component.camera.target = {width / 2.0f, height / 2.0f};
		camera_component.camera.offset = {width / 2.0f, height / 2.0f};

		ask_confirm_layer.Resize(width, height);
		ask_confirm_layer.x = 0;
		ask_confirm_layer.y = 0;

		ask_crop_format_layer.x = 20;
		ask_crop_format_layer.y = 100;
		ask_crop_format_layer.Resize(240, 200);

		console_log.SetDestinationBounds({10.0f, 25.0f, width - 20.0f, height - 50.0f});
	}

	Screen GetScreen()
	{
		Screen result;

		result.LoadFunction = &Load;
		result.UnloadFunction = &Unload;
		result.UpdateFunction = &Update;
		result.RenderFunction = &Render;
		result.OnResize = &OnResize;

		return result;
	}
}
