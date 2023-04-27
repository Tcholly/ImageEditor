#pragma once

#include <vector>
#include <string>
#include <raylib.h>

struct ConsoleLogMessage
{
	std::string message;
	Color text_color;
	float life;
};

class ConsoleLog
{
public:
	ConsoleLog();

	void Load(Rectangle destination, float message_lifetime);
	void Unload();

	void Print(const std::string& value, Color text_color);

	void Update(float dt);
	void Render(bool bottom_is_latest = false, bool stick_right = false);

	void SetMessageColor(Color color);
	void SetMessageLifetime(float lifetime);
	void SetDestinationBounds(Rectangle bounds);

private:
	std::vector<ConsoleLogMessage> content;
	float message_lifetime = 5.0f;
	Rectangle destination = {0.0f, 0.0f, 100.0f, 100.0f};
	RenderTexture2D output_texture;
};
