#include "ConsoleLog.h"

ConsoleLog::ConsoleLog()
{
}

void ConsoleLog::Load(Rectangle _destination, float _message_lifetime)
{
	message_lifetime = _message_lifetime;
	destination = _destination;
	output_texture = LoadRenderTexture(destination.width, destination.height);
}

void ConsoleLog::Unload()
{
	UnloadRenderTexture(output_texture);
}

void ConsoleLog::Print(const std::string &value, Color text_color)
{
	ConsoleLogMessage to_add;
	to_add.message = value;
	to_add.text_color = text_color;
	to_add.life = message_lifetime;
	content.emplace_back(to_add);
}

void ConsoleLog::Update(float dt)
{
	if (content.size() > 0)
	{
		for (int i = content.size() - 1; i >= 0; i--)
		{
			content[i].life -= dt;
			if (content[i].life < 0.0f)
				content.erase(content.begin() + i);
		}
	}
}

void ConsoleLog::Render(bool bottom_is_latest, bool stick_left)
{
	if (content.size() < 1)
		return;

	int beginY = 0;
	int increment = 20;
	if (bottom_is_latest)
	{
		beginY = destination.height - increment;
		increment *= -1;
	}

	BeginTextureMode(output_texture);
	ClearBackground({0, 0, 0, 0});
	for (int i = content.size() - 1; i >= 0; i--)
	{
		int x_pos = 0;
		if (stick_left)
		{
			int text_lenght = MeasureText(content[i].message.c_str(), 20);
			x_pos = destination.width - text_lenght;
		}
		DrawText(content[i].message.c_str(), x_pos, beginY + increment * i, 20, content[i].text_color);	
	}
	EndTextureMode();

	DrawTexturePro(output_texture.texture, {0.0f, 0.0f, destination.width, -destination.height}, destination, {0.0f, 0.0f}, 0.0f, WHITE);
}

void ConsoleLog::SetMessageLifetime(float lifetime)
{
	message_lifetime = lifetime;
}

void ConsoleLog::SetDestinationBounds(Rectangle bounds)
{
	destination = bounds;
	UnloadRenderTexture(output_texture);
	output_texture = LoadRenderTexture(destination.width, destination.height);
}
