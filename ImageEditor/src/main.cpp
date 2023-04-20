#include <Difu/ScreenManagement/ScreenManager.h>
#include "Screens/EditorScreen.h"
#include <Difu/WindowManagement/WindowManager.h>

int main()
{
	if (WindowManager::InitWindow("ImageEditor", 800, 480, true))
	{
		ScreenManager::ChangeScreen(EditorScreen::GetScreen());
		WindowManager::RunWindow();
	}
}
