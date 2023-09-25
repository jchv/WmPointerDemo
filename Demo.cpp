#include "Base.h"
#include "Print.h"
#include <unordered_map>
#include <thread>

class MainWindow final : public BaseWindow<MainWindow>
{
public:
	PCTSTR ClassName() const { return TEXT("WmPointerDemo"); }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void InjectEvents() const;
	void Log(std::string_view Line) const;
	bool Throttle(UINT uMsg);
	void UpdateDPIDependentResources();

protected:
	HWND m_hwndEdit = nullptr;
	HWND m_hwndTerse = nullptr;
	HWND m_hwndThrottle = nullptr;
	HWND m_hwndMotionEnabled = nullptr;
	HWND m_hwndRetZeroOnWMPointer = nullptr;
	HWND m_hwndCallPromoteMouseInPointer = nullptr;
	HWND m_hwndInject = nullptr;

	int m_dpi = 96;
	bool m_terse = true;
	bool m_throttle = false;
	bool m_motionEnabled = false;
	bool m_returnZeroOnWMPointer = true;
	bool m_callPromoteMouseInPointer = false;
	std::unordered_map<UINT, ULONGLONG> m_msgTickMap{};
	std::unordered_map<UINT, int> m_throttleCount{};

	constexpr static int IDC_TEXTLOG = 100;
	constexpr static int IDC_TERSE = 101;
	constexpr static int IDC_THROTTLE = 102;
	constexpr static int IDC_MOTIONENABLE = 103;
	constexpr static int IDC_RETZPTR = 104;
	constexpr static int IDC_CALLPROMOTE = 105;
	constexpr static int IDC_INJECT = 106;
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(pCmdLine);

	EnableMouseInPointer(TRUE);
	
	MainWindow win{};
	if (!win.Create(TEXT("WmPointer Demo"), WS_OVERLAPPEDWINDOW, 0, CW_USEDEFAULT, CW_USEDEFAULT, 800, 600))
	{
		MessageBox(HWND_DESKTOP, TEXT("Unable to create main window?"), TEXT("Fatal Error"), MB_OK | MB_ICONSTOP);
		return 0;
	}
	ShowWindow(win.Window(), nCmdShow);

	MSG msg{};
	while (GetMessage(&msg, nullptr, 0, 0))
	{
		TranslateMessage(&msg);
		DispatchMessage(&msg);
	}

	return 0;
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#define MESSAGE_CASE(prefix, message) \
	case prefix##_##message: \
		if (this->m_throttle && this->Throttle(prefix##_##message)) { break; } \
		if (!this->m_terse) this->Log(""); \
		this->Log(fmt::format(FMT_STRING(#prefix "_" #message "(wParam: {:#010x}, lParam: {:#010x})"), wParam, lParam)); \
		if (this->m_throttleCount[prefix##_##message] > 0) \
		{ \
			this->Log(fmt::format(FMT_STRING("; (throttled {} previous " #prefix "_" #message " messages)"), this->m_throttleCount[prefix##_##message])); \
			this->m_throttleCount.erase(prefix##_##message); \
		}

#define LOG_DERIVED(derived, desc) \
	if (!this->m_terse) this->Log(fmt::format(FMT_STRING("; - " #derived " = {}  " desc), derived))

	if (m_callPromoteMouseInPointer)
	{
		reinterpret_cast<int(__stdcall*)(int)>(GetProcAddress(GetModuleHandle(TEXT("win32u")), "NtUserPromoteMouseInPointer"))(0);
		reinterpret_cast<int(__stdcall*)(int, int)>(GetProcAddress(GetModuleHandle(TEXT("win32u")), "NtUserPromotePointer"))(GET_POINTERID_WPARAM(wParam), MAKELONG(0, 0));
		reinterpret_cast<int(__stdcall*)(int, int)>(GetProcAddress(GetModuleHandle(TEXT("win32u")), "NtUserPromotePointer"))(GET_POINTERID_WPARAM(wParam), MAKELONG(1, 1));
	}

	if (!m_motionEnabled)
	{
		switch (uMsg)
		{
		case WM_POINTERUPDATE:
			if (m_returnZeroOnWMPointer)
			{
				return 0;
			}
			// fallthrough
		case WM_NCPOINTERUPDATE:
		case WM_MOUSEMOVE:
			return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
		}
	}
	
	switch (uMsg)
	{
	case WM_CREATE:
		{
			RegisterPointerDeviceNotifications(m_hwnd, TRUE);
			RegisterTouchWindow(m_hwnd, 0);
			RegisterTouchHitTestingWindow(m_hwnd, TOUCH_HIT_TESTING_CLIENT);

			m_dpi = GetDpiForWindow(m_hwnd);

			RECT wndRect;
			GetWindowRect(m_hwnd, &wndRect);
			MoveWindow(
				m_hwnd,
				wndRect.left,
				wndRect.top,
				MulDiv(wndRect.right - wndRect.left, m_dpi, USER_DEFAULT_SCREEN_DPI),
				MulDiv(wndRect.bottom - wndRect.top, m_dpi, USER_DEFAULT_SCREEN_DPI),
				TRUE
			);
			
			m_hwndEdit = CreateWindowEx(
				0, 
				TEXT("EDIT"),
				nullptr,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY | WS_DISABLED | ES_AUTOVSCROLL,
				0, 
				0, 
				0, 
				0,
				m_hwnd,
				reinterpret_cast<HMENU>(IDC_TEXTLOG),
				reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)),
				nullptr
			);

			SendMessage(m_hwndEdit, EM_SETLIMITTEXT, 0, 0);

			m_hwndTerse = CreateWindowEx(
				0,
				TEXT("BUTTON"),
				TEXT("Terse"),
				WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
				0,
				0,
				0,
				0,
				m_hwnd,
				reinterpret_cast<HMENU>(IDC_TERSE),
				reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)),
				nullptr
			);
			Button_SetCheck(m_hwndTerse, m_terse);

			m_hwndThrottle = CreateWindowEx(
				0,
				TEXT("BUTTON"),
				TEXT("Throttle"),
				WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
				0,
				0,
				0,
				0,
				m_hwnd,
				reinterpret_cast<HMENU>(IDC_THROTTLE),
				reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)),
				nullptr
			);
			Button_SetCheck(m_hwndThrottle, m_throttle);

			m_hwndMotionEnabled = CreateWindowEx(
				0, 
				TEXT("BUTTON"), 
				TEXT("Motion Events"), 
				WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
				0,
				0,
				0,
				0,
				m_hwnd,
				reinterpret_cast<HMENU>(IDC_MOTIONENABLE),
				reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)), 
				nullptr
			);
			Button_SetCheck(m_hwndMotionEnabled, m_motionEnabled);

			m_hwndRetZeroOnWMPointer = CreateWindowEx(
				0,
				TEXT("BUTTON"),
				TEXT("Ret 0 on WM_POINTER*"),
				WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
				0,
				0,
				0,
				0,
				m_hwnd,
				reinterpret_cast<HMENU>(IDC_RETZPTR),
				reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)),
				nullptr
			);
			Button_SetCheck(m_hwndRetZeroOnWMPointer, m_returnZeroOnWMPointer);

			m_hwndCallPromoteMouseInPointer = CreateWindowEx(
				0,
				TEXT("BUTTON"),
				TEXT("Call NtUserPromoteMouseInPointer"),
				WS_CHILD | WS_VISIBLE | BS_CHECKBOX,
				0,
				0,
				0,
				0,
				m_hwnd,
				reinterpret_cast<HMENU>(IDC_CALLPROMOTE),
				reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)),
				nullptr
			);
			Button_SetCheck(m_hwndCallPromoteMouseInPointer, m_callPromoteMouseInPointer);

			m_hwndInject = CreateWindowEx(
				0,
				TEXT("BUTTON"),
				TEXT("Inject"),
				WS_CHILD | WS_VISIBLE,
				0,
				0,
				0,
				0,
				m_hwnd,
				reinterpret_cast<HMENU>(IDC_INJECT),
				reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)),
				nullptr
			);

			UpdateDPIDependentResources();
		}
		return 0;

	case WM_SIZE:
		{
			const auto w = LOWORD(lParam), h = HIWORD(lParam);
			const auto logH = h / 2;
			
			MoveWindow(m_hwndEdit, 0, 0, w, logH, TRUE);

			const auto numChecks = 6;
			const auto checkW = w / numChecks;
			const auto checkH = MulDiv(40, m_dpi, USER_DEFAULT_SCREEN_DPI);
			MoveWindow(m_hwndTerse, checkW * 0, logH, checkW, checkH, TRUE);
			MoveWindow(m_hwndThrottle, checkW * 1, logH, checkW, checkH, TRUE);
			MoveWindow(m_hwndMotionEnabled, checkW * 2, logH, checkW, checkH, TRUE);
			MoveWindow(m_hwndRetZeroOnWMPointer, checkW * 3, logH, checkW, checkH, TRUE);
			MoveWindow(m_hwndCallPromoteMouseInPointer, checkW * 4, logH, w - checkW * (numChecks - 1), checkH, TRUE);
			MoveWindow(m_hwndInject, checkW * 5, logH, w - checkW * (numChecks - 1), checkH, TRUE);
	}
		return 0;

	case WM_DPICHANGED:
		{
			m_dpi = HIWORD(wParam);
			UpdateDPIDependentResources();

			RECT* const newRect = reinterpret_cast<RECT*>(lParam);
			SetWindowPos(
				m_hwnd,
				nullptr,
				newRect->left,
				newRect->top,
				newRect->right - newRect->left,
				newRect->bottom - newRect->top,
				SWP_NOZORDER | SWP_NOACTIVATE
			);
			break;
		}

	case WM_DESTROY:
		{
			PostQuitMessage(0);
		}
		return 0;

	case WM_PAINT:
		{
			PAINTSTRUCT ps;
			HDC hdc = BeginPaint(m_hwnd, &ps);
			FillRect(hdc, &ps.rcPaint, reinterpret_cast<HBRUSH>((COLOR_WINDOWFRAME)));
			EndPaint(m_hwnd, &ps);
		}
		return 0;

	case WM_KEYUP:
		switch (wParam)
		{
		case '1':
			{
				HSYNTHETICPOINTERDEVICE d = CreateSyntheticPointerDevice(PT_PEN, 1, POINTER_FEEDBACK_DEFAULT);
				Log(fmt::format(FMT_STRING("Created synthetic pointer {}"), reinterpret_cast<void*>(d)));

				POINTER_TYPE_INFO t{};
				t.type = PT_PEN;
				t.penInfo.pointerInfo.pointerType = PT_PEN;
				t.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_PRIMARY;
				t.penInfo.pointerInfo.ptPixelLocation.x = 10;
				t.penInfo.pointerInfo.ptPixelLocation.y = 10;
				t.penInfo.penFlags = PEN_FLAG_NONE;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_PRIMARY;
				t.penInfo.pointerInfo.ptPixelLocation.x = 20;
				t.penInfo.pointerInfo.ptPixelLocation.y = 20;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_INCONTACT | POINTER_FLAG_FIRSTBUTTON | POINTER_FLAG_PRIMARY;
				t.penInfo.pointerInfo.ButtonChangeType = POINTER_CHANGE_FIRSTBUTTON_DOWN;
				t.penInfo.pointerInfo.ptPixelLocation.x = 50;
				t.penInfo.pointerInfo.ptPixelLocation.y = 50;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x080;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 55;
				t.penInfo.pointerInfo.ptPixelLocation.y = 55;
				t.penInfo.pointerInfo.ButtonChangeType = POINTER_CHANGE_NONE;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x100;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 58;
				t.penInfo.pointerInfo.ptPixelLocation.y = 58;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x180;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 60;
				t.penInfo.pointerInfo.ptPixelLocation.y = 60;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x200;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 70;
				t.penInfo.pointerInfo.ptPixelLocation.y = 70;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x280;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 80;
				t.penInfo.pointerInfo.ptPixelLocation.y = 80;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x300;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 90;
				t.penInfo.pointerInfo.ptPixelLocation.y = 90;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x380;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 100;
				t.penInfo.pointerInfo.ptPixelLocation.y = 100;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x400;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 110;
				t.penInfo.pointerInfo.ptPixelLocation.y = 110;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x380;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 120;
				t.penInfo.pointerInfo.ptPixelLocation.y = 120;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x300;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 130;
				t.penInfo.pointerInfo.ptPixelLocation.y = 130;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x280;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 140;
				t.penInfo.pointerInfo.ptPixelLocation.y = 140;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x200;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 150;
				t.penInfo.pointerInfo.ptPixelLocation.y = 150;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x180;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 160;
				t.penInfo.pointerInfo.ptPixelLocation.y = 160;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x100;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.ptPixelLocation.x = 170;
				t.penInfo.pointerInfo.ptPixelLocation.y = 170;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0x080;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				t.penInfo.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE | POINTER_FLAG_PRIMARY;
				t.penInfo.pointerInfo.ButtonChangeType = POINTER_CHANGE_FIRSTBUTTON_UP;
				t.penInfo.pointerInfo.ptPixelLocation.x = 180;
				t.penInfo.pointerInfo.ptPixelLocation.y = 180;
				t.penInfo.penFlags = PEN_FLAG_NONE;
				t.penInfo.penMask = PEN_MASK_PRESSURE;
				t.penInfo.pressure = 0;
				InjectSyntheticPointerInput(d, &t, 1);

				Sleep(100);

				DestroySyntheticPointerDevice(d);
				Log(fmt::format(FMT_STRING("Destroyed synthetic pointer {}"), reinterpret_cast<void*>(d)));
			}
		}
		break;

	case WM_COMMAND:
		switch (wParam)
		{
		case IDC_TERSE:
			m_terse = !m_terse;
			Button_SetCheck(m_hwndTerse, m_terse);
			break;
		case IDC_THROTTLE:
			m_throttle = !m_throttle;
			Button_SetCheck(m_hwndThrottle, m_throttle);
			break;
		case IDC_MOTIONENABLE:
			m_motionEnabled = !m_motionEnabled;
			Button_SetCheck(m_hwndMotionEnabled, m_motionEnabled);
			break;
		case IDC_RETZPTR:
			m_returnZeroOnWMPointer = !m_returnZeroOnWMPointer;
			Button_SetCheck(m_hwndRetZeroOnWMPointer, m_returnZeroOnWMPointer);
			break;
		case IDC_CALLPROMOTE:
			m_callPromoteMouseInPointer = !m_callPromoteMouseInPointer;
			Button_SetCheck(m_hwndCallPromoteMouseInPointer, m_callPromoteMouseInPointer);
			break;
		case IDC_INJECT:
			InjectEvents();
			break;
		}
		return 0;

	MESSAGE_CASE(DM, POINTERHITTEST)
		break;

	MESSAGE_CASE(WM, NCPOINTERDOWN)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(HIWORD(wParam), "hit test value");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM, NCPOINTERUP)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(HIWORD(wParam), "hit test value");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM, NCPOINTERUPDATE)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(HIWORD(wParam), "hit test value");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM, POINTERACTIVATE)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(HIWORD(wParam), "hit test value");
			LOG_DERIVED(lParam, "window being activated");
		}
		break;

	MESSAGE_CASE(WM, POINTERCAPTURECHANGED)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(lParam, "window capturing pointer");
		}
		break;

	MESSAGE_CASE(WM, POINTERDEVICECHANGE)
		{
			LOG_DERIVED(PDC_STR(wParam), "pointer identifier");
		}
		break;

	MESSAGE_CASE(WM, POINTERDEVICEINRANGE)
		break;

	MESSAGE_CASE(WM, POINTERDEVICEOUTOFRANGE)
		break;

	MESSAGE_CASE(WM, POINTERDOWN)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(PointerState(wParam), "pointer state");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM, POINTERENTER)
		break;

	MESSAGE_CASE(WM, POINTERLEAVE)
		break;

	MESSAGE_CASE(WM, POINTERROUTEDAWAY)
		break;

	MESSAGE_CASE(WM, POINTERROUTEDRELEASED)
		break;

	MESSAGE_CASE(WM, POINTERROUTEDTO)
		break;

	MESSAGE_CASE(WM, POINTERUP)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(PointerState(wParam), "pointer state");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM, POINTERUPDATE)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(PointerState(wParam), "pointer state");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM, POINTERWHEEL)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(GET_WHEEL_DELTA_WPARAM(wParam), "wheel delta");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM, POINTERHWHEEL)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(GET_WHEEL_DELTA_WPARAM(wParam), "wheel delta");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM, TOUCHHITTESTING)
		if (wParam != 0) {
			const auto* info = reinterpret_cast<TOUCH_HIT_TESTING_INPUT*>(wParam);
			LOG_DERIVED(info->pointerId, "pointer identifier");
			LOG_DERIVED(info->point.x, "x coordinate");
			LOG_DERIVED(info->point.y, "y coordinate");
			LOG_DERIVED(info->orientation, "orientation");
		}
		return 0;

	MESSAGE_CASE(WM, MOUSEMOVE)
		{
			LOG_DERIVED(MouseState(wParam), "pointer state");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		return 0;

	MESSAGE_CASE(WM, MOUSEWHEEL)
		{
			LOG_DERIVED(MouseState(wParam), "pointer state");
			LOG_DERIVED(static_cast<SHORT> HIWORD(wParam), "wheel delta");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		return 0;

	MESSAGE_CASE(WM, MOUSELEAVE)
		return 0;

	MESSAGE_CASE(WM, MOUSEACTIVATE)
		{
			LOG_DERIVED(wParam, "top level window");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		// Do *not* eat the event, so we can see that too.
		return MA_ACTIVATE;

#define MESSAGE_CASE_BUTTONEVENT(EVENT) \
	MESSAGE_CASE(WM, EVENT) \
		{ \
			LOG_DERIVED(MouseState(wParam), "pointer state"); \
			LOG_DERIVED(HIWORD(wParam), "x button"); \
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate"); \
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate"); \
		} \
		return 0

#define MESSAGE_CASE_BUTTONEVENTS(btn) \
	MESSAGE_CASE_BUTTONEVENT(btn##BUTTONDBLCLK); \
	MESSAGE_CASE_BUTTONEVENT(btn##BUTTONDOWN); \
	MESSAGE_CASE_BUTTONEVENT(btn##BUTTONUP) \

	MESSAGE_CASE_BUTTONEVENTS(R);
	MESSAGE_CASE_BUTTONEVENTS(L);
	MESSAGE_CASE_BUTTONEVENTS(M);
	MESSAGE_CASE_BUTTONEVENTS(X);
		
	default:
		break;
	}

#undef LOG_DERIVED
#undef MESSAGE_CASE

	if (m_returnZeroOnWMPointer)
	{
		switch (uMsg)
		{
		case WM_POINTERACTIVATE:
		case WM_POINTERCAPTURECHANGED:
		case WM_POINTERDEVICECHANGE:
		case WM_POINTERDEVICEINRANGE:
		case WM_POINTERDEVICEOUTOFRANGE:
		case WM_POINTERDOWN:
		case WM_POINTERENTER:
		case WM_POINTERLEAVE:
		case WM_POINTERROUTEDAWAY:
		case WM_POINTERROUTEDRELEASED:
		case WM_POINTERROUTEDTO:
		case WM_POINTERUP:
		case WM_POINTERUPDATE:
		case WM_POINTERWHEEL:
		case WM_POINTERHWHEEL:
			return 0;
		default:
			break;
		}
	}

	return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
}

typedef void (__stdcall * NtUserInitializePointerDeviceInjectionFn)(
	POINTER_INPUT_TYPE       pointerType,
	ULONG                    maxCount,
	ULONG                    unknown,
	POINTER_FEEDBACK_MODE    mode,
	HSYNTHETICPOINTERDEVICE* out
);

typedef void(__stdcall* NtUserInjectPointerInputFn)(
	HSYNTHETICPOINTERDEVICE  device,
	const POINTER_TYPE_INFO* pointerInfo,
	UINT32                   count
);

typedef void(__stdcall* NtUserRemoveInjectionDeviceFn)(
	HSYNTHETICPOINTERDEVICE device
);

//NtUserInitializePointerDeviceInjection(pointerType, maxCount, 0, mode, &result);
void MainWindow::InjectEvents() const
{
	std::thread t{ [&]() {
		HSYNTHETICPOINTERDEVICE device{};
		auto NtUserInitializePointerDeviceInjection = reinterpret_cast<NtUserInitializePointerDeviceInjectionFn>(GetProcAddress(GetModuleHandle(TEXT("win32u")), "NtUserInitializePointerDeviceInjection"));
		auto NtUserInjectPointerInput = reinterpret_cast<NtUserInjectPointerInputFn>(GetProcAddress(GetModuleHandle(TEXT("win32u")), "NtUserInjectPointerInput"));
		auto NtUserRemoveInjectionDevice = reinterpret_cast<NtUserRemoveInjectionDeviceFn>(GetProcAddress(GetModuleHandle(TEXT("win32u")), "NtUserRemoveInjectionDevice"));

		NtUserInitializePointerDeviceInjection(PT_PEN, 1, 0, POINTER_FEEDBACK_DEFAULT, &device);
		for (int i = 0; i < 50; i++) {
			POINTER_TYPE_INFO inputInfo[1];
			inputInfo[0].type = PT_PEN;
			inputInfo[0].penInfo.pointerInfo.pointerType = PT_PEN;
			inputInfo[0].penInfo.pointerInfo.pointerId = 0;
			inputInfo[0].penInfo.pointerInfo.frameId = 0;
			inputInfo[0].penInfo.pointerInfo.pointerFlags = POINTER_FLAG_INRANGE;
			inputInfo[0].penInfo.penMask = PEN_MASK_PRESSURE | PEN_MASK_TILT_X | PEN_MASK_TILT_Y;
			inputInfo[0].penInfo.pointerInfo.ptPixelLocation.x = 100 + i * 5;
			inputInfo[0].penInfo.pointerInfo.ptPixelLocation.y = 100 + i * 5;
			inputInfo[0].penInfo.pressure = 0;
			inputInfo[0].penInfo.tiltX = 15;
			inputInfo[0].penInfo.tiltY = -26;
			inputInfo[0].penInfo.pointerInfo.dwTime = 0;
			inputInfo[0].penInfo.pointerInfo.PerformanceCount = 0;
			NtUserInjectPointerInput(device, inputInfo, 1);
			Sleep(10);
		}
		NtUserRemoveInjectionDevice(device);

		MessageBox(m_hwnd, TEXT("Done"), TEXT("Inject"), MB_OK | MB_ICONINFORMATION);
	} };
	t.detach();
}

void MainWindow::Log(std::string_view Line) const
{
	SendMessage(m_hwndEdit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(ToWinString(Line).c_str()));
	SendMessage(m_hwndEdit, EM_REPLACESEL, TRUE, reinterpret_cast<LPARAM>(TEXT("\r\n")));
}

bool MainWindow::Throttle(UINT uMsg)
{
	const auto currentTicks = GetTickCount64();
	if (currentTicks - m_msgTickMap[uMsg] < g_ThrottleMilliseconds)
	{
		this->m_throttleCount[uMsg]++;
		return true;
	}
	m_msgTickMap[uMsg] = currentTicks;
	return false;
}

void MainWindow::UpdateDPIDependentResources()
{
	const int fontSize = MulDiv(18, m_dpi, USER_DEFAULT_SCREEN_DPI);

	HFONT hFontConsolas = CreateFont(fontSize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT("Consolas"));
	SendMessage(m_hwndEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFontConsolas), MAKELPARAM(TRUE, 0));

	HFONT hFontCalibri = CreateFont(fontSize, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT("Calibri"));
	SendMessage(m_hwndTerse, WM_SETFONT, reinterpret_cast<WPARAM>(hFontCalibri), MAKELPARAM(TRUE, 0));
	SendMessage(m_hwndThrottle, WM_SETFONT, reinterpret_cast<WPARAM>(hFontCalibri), MAKELPARAM(TRUE, 0));
	SendMessage(m_hwndMotionEnabled, WM_SETFONT, reinterpret_cast<WPARAM>(hFontCalibri), MAKELPARAM(TRUE, 0));
	SendMessage(m_hwndRetZeroOnWMPointer, WM_SETFONT, reinterpret_cast<WPARAM>(hFontCalibri), MAKELPARAM(TRUE, 0));
	SendMessage(m_hwndCallPromoteMouseInPointer, WM_SETFONT, reinterpret_cast<WPARAM>(hFontCalibri), MAKELPARAM(TRUE, 0));
	SendMessage(m_hwndInject, WM_SETFONT, reinterpret_cast<WPARAM>(hFontCalibri), MAKELPARAM(TRUE, 0));
}
