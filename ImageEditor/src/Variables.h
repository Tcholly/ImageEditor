#pragma once

struct AskCropFormatDialogResult
{
	unsigned int x, y;
};

namespace Variables
{
	extern bool ask_confirm_dialog_result;
	extern AskCropFormatDialogResult ask_crop_dialog_result;
}
