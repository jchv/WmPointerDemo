#include "Base.h"
#include "Print.h"

class MainWindow final : public BaseWindow<MainWindow>
{
public:
	PCTSTR ClassName() const { return TEXT("WmPointerDemo"); }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Log(std::string_view Line) const;
	bool Throttle(UINT uMsg);
	void UpdateDPIDependentResources();

protected:
	HWND m_hwndEdit = nullptr;
	HWND m_hwndTerse = nullptr;
	HWND m_hwndThrottle = nullptr;
	HWND m_hwndMotionEnabled = nullptr;
	HWND m_hwndRetZeroOnWMPointer = nullptr;

	int m_dpi = 96;
	bool m_terse = true;
	bool m_throttle = false;
	bool m_motionEnabled = false;
	bool m_returnZeroOnWMPointer = true;
	std::unordered_map<UINT, ULONGLONG> m_msgTickMap{};
	std::unordered_map<UINT, int> m_throttleCount{};

	constexpr static int IDC_TEXTLOG = 100;
	constexpr static int IDC_TERSE = 101;
	constexpr static int IDC_THROTTLE = 102;
	constexpr static int IDC_MOTIONENABLE = 103;
	constexpr static int IDC_RETZPTR = 104;
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
		this->Log(fmt::format(#prefix "_" #message "(wParam: {}, lParam: {})", wParam, lParam)); \
		if (this->m_throttleCount[prefix##_##message] > 0) \
		{ \
			this->Log(fmt::format("; (throttled {} previous " #prefix "_" #message " messages)", this->m_throttleCount[prefix##_##message])); \
			this->m_throttleCount.erase(prefix##_##message); \
		}

#define LOG_DERIVED(derived, desc) \
	if (!this->m_terse) this->Log(fmt::format("; - " #derived " = {}  " desc, derived))

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

			UpdateDPIDependentResources();
		}
		return 0;

	case WM_SIZE:
		{
			const auto w = LOWORD(lParam), h = HIWORD(lParam);
			const auto logH = h / 2;
			
			MoveWindow(m_hwndEdit, 0, 0, w, logH, TRUE);

			const auto numChecks = 4;
			const auto checkW = w / numChecks;
			const auto checkH = MulDiv(40, m_dpi, USER_DEFAULT_SCREEN_DPI);
			MoveWindow(m_hwndTerse, checkW * 0, logH, checkW, checkH, TRUE);
			MoveWindow(m_hwndThrottle, checkW * 1, logH, checkW, checkH, TRUE);
			MoveWindow(m_hwndMotionEnabled, checkW * 2, logH, checkW, checkH, TRUE);
			MoveWindow(m_hwndRetZeroOnWMPointer, checkW * 3, logH, w - (checkW * (numChecks - 1)), checkH, TRUE);
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
		{
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
}
