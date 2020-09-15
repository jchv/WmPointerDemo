#pragma once

#include <windows.h>
#include <fmt/core.h>

namespace {

#define ENUM_STR_START(name) static std::string name##_STR(int val) { switch (val) { default: return fmt::format("{}", val)
#define ENUM_STR_END() } }
#define ENUM_STR_KEY(key) case key: return #key

ENUM_STR_START(PDC);
ENUM_STR_KEY(PDC_ARRIVAL);
ENUM_STR_KEY(PDC_REMOVAL);
ENUM_STR_KEY(PDC_ORIENTATION_0);
ENUM_STR_KEY(PDC_ORIENTATION_90);
ENUM_STR_KEY(PDC_ORIENTATION_180);
ENUM_STR_KEY(PDC_ORIENTATION_270);
ENUM_STR_KEY(PDC_MODE_DEFAULT);
ENUM_STR_KEY(PDC_MODE_CENTERED);
ENUM_STR_KEY(PDC_MAPPING_CHANGE);
ENUM_STR_KEY(PDC_RESOLUTION);
ENUM_STR_KEY(PDC_ORIGIN);
ENUM_STR_KEY(PDC_MODE_ASPECTRATIOPRESERVED);
ENUM_STR_END();

#undef ENUM_STR_KEY
#undef ENUM_STR_END
#undef ENUM_STR_START

constexpr std::string_view Btn(bool down)
{
	return down ? "X" : " ";
}

std::string PointerState(WPARAM wParam)
{
	const bool newPtr = IS_POINTER_NEW_WPARAM(wParam);
	const bool inRange = IS_POINTER_INRANGE_WPARAM(wParam);
	const bool inContact = IS_POINTER_INCONTACT_WPARAM(wParam);
	const bool isPrimary = IS_POINTER_PRIMARY_WPARAM(wParam);
	const bool b1 = IS_POINTER_FIRSTBUTTON_WPARAM(wParam);
	const bool b2 = IS_POINTER_SECONDBUTTON_WPARAM(wParam);
	const bool b3 = IS_POINTER_THIRDBUTTON_WPARAM(wParam);
	const bool b4 = IS_POINTER_FOURTHBUTTON_WPARAM(wParam);
	const bool b5 = IS_POINTER_FIFTHBUTTON_WPARAM(wParam);
	return fmt::format("{} {} {} {} [{}|{}|{}|{}|{}]", newPtr ? "NEW" : "!NEW", inRange ? "INRANGE" : "!INRANGE", inContact ? "INCONTACT" : "!INCONTACT", isPrimary ? "PRIMARY" : "!PRIMARY", Btn(b1), Btn(b2), Btn(b3), Btn(b4), Btn(b5));
}

std::string MouseState(WPARAM wParam)
{
	const bool l = LOWORD(wParam) & MK_LBUTTON;
	const bool r = LOWORD(wParam) & MK_RBUTTON;
	const bool shift = LOWORD(wParam) & MK_SHIFT;
	const bool ctrl = LOWORD(wParam) & MK_CONTROL;
	const bool m = LOWORD(wParam) & MK_MBUTTON;
	const bool x1 = LOWORD(wParam) & MK_XBUTTON1;
	const bool x2 = LOWORD(wParam) & MK_XBUTTON2;
	return fmt::format("{} {} [{}|{}|{}|{}|{}]", shift ? "SHIFT" : "!SHIFT", ctrl ? "CTRL" : "!CTRL", Btn(l), Btn(r), Btn(m), Btn(x1), Btn(x2));
}

}