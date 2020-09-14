#include <windows.h>
#include <windowsx.h>
#include <fmt/core.h>
#include <string>

constexpr int g_ThrottleMilliseconds = 500;

#ifdef UNICODE
#include <locale>

std::wstring ToWinString(std::string_view utf8)
{
	const int utf8Length = static_cast<int>(utf8.length());
	std::wstring utf16;
	if (utf8.empty())
	{
		return utf16;
	}
	constexpr DWORD kFlags = MB_ERR_INVALID_CHARS;
	const int utf16Length = ::MultiByteToWideChar(
		CP_UTF8,
		kFlags,
		utf8.data(),
		utf8Length,
		nullptr,
		0
	);
	if (utf16Length == 0)
	{
		return utf16;
	}
	utf16.resize(utf16Length);
	int result = ::MultiByteToWideChar(
		CP_UTF8,
		kFlags,
		utf8.data(),
		utf8Length,
		&utf16[0],
		utf16Length
	);
	return utf16;
}

#else

#define ToWinString(x) x

#endif

template <typename DerivedType>
class BaseWindow
{
public:
	virtual ~BaseWindow() = default;

	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
	{
		DerivedType* pThis = nullptr;
		if (uMsg == WM_NCCREATE)
		{
			auto* pCreate = reinterpret_cast<CREATESTRUCT*>(lParam);
			pThis = static_cast<DerivedType*>(pCreate->lpCreateParams);
			SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pThis));
			pThis->m_hwnd = hwnd;
		}
		else
		{
			pThis = reinterpret_cast<DerivedType*>(GetWindowLongPtr(hwnd, GWLP_USERDATA));
		}
		if (pThis)
		{
			return pThis->HandleMessage(uMsg, wParam, lParam);
		}
		return DefWindowProc(hwnd, uMsg, wParam, lParam);
	}

	BaseWindow() : m_hwnd(nullptr)
	{
	}

	BOOL Create(
		PCTSTR lpWindowName,
		DWORD dwStyle,
		DWORD dwExStyle = 0,
		int x = CW_USEDEFAULT,
		int y = CW_USEDEFAULT,
		int nWidth = CW_USEDEFAULT,
		int nHeight = CW_USEDEFAULT,
		HWND hWndParent = nullptr,
		HMENU hMenu = nullptr
	)
	{
		WNDCLASS wc = {0};

		wc.lpfnWndProc = DerivedType::WindowProc;
		wc.hInstance = GetModuleHandle(nullptr);
		wc.lpszClassName = ClassName();
		wc.hCursor = LoadCursor(nullptr, IDC_ARROW);

		RegisterClass(&wc);

		m_hwnd = CreateWindowEx(
			dwExStyle, ClassName(), lpWindowName, dwStyle, x, y,
			nWidth, nHeight, hWndParent, hMenu, GetModuleHandle(nullptr), this
		);

		return (m_hwnd ? TRUE : FALSE);
	}

	HWND Window() const { return m_hwnd; }

protected:
	virtual PCTSTR ClassName() const = 0;
	virtual LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) = 0;

	HWND m_hwnd;
};

class MainWindow final : public BaseWindow<MainWindow>
{
public:
	PCTSTR ClassName() const { return TEXT("WmPointerDemo"); }
	LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam);
	void Log(std::string_view Line) const;
	bool Throttle(UINT uMsg);

protected:
	HWND m_hwndEdit;
	HWND m_hwndMotionEnabled;

	bool m_motionEnabled = false;
	std::unordered_map<UINT, ULONGLONG> m_msgTickMap;
	std::unordered_map<UINT, int> m_throttleCount;

	constexpr static int IDC_TEXTLOG = 100;
	constexpr static int IDC_MOTIONENABLE = 101;
};

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow)
{
	UNREFERENCED_PARAMETER(hInstance);
	UNREFERENCED_PARAMETER(pCmdLine);

	EnableMouseInPointer(TRUE);
	
	MainWindow win{};
	if (!win.Create(TEXT("WmPointer Demo"), WS_OVERLAPPEDWINDOW, 0, CW_USEDEFAULT, CW_USEDEFAULT, 640, 480))
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

#define ENUM_STR_START(name) std::string name##_STR(int val) { switch (val) { default: return fmt::format("{}", val)
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
	switch (down)
	{
	case true: return "X";
	case false: return " ";
	}
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

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam)
{
#define MESSAGE_CASE(message) \
	case message: \
		if (this->Throttle(message)) { break; } \
		this->Log(fmt::format("\r\n" #message "(wParam: {}, lParam: {})", wParam, lParam)); \
		if (this->m_throttleCount[message] > 0) \
		{ \
			this->Log(fmt::format("; (throttled {} previous " #message " messages)", this->m_throttleCount[message])); \
			this->m_throttleCount.erase(message); \
		}

#define LOG_DERIVED(derived, desc) \
	this->Log(fmt::format("; - " #derived " = {}  " desc, derived))

	if (!m_motionEnabled)
	{
		switch (uMsg)
		{
		case WM_NCPOINTERUPDATE:
		case WM_POINTERUPDATE:
			return DefWindowProc(m_hwnd, uMsg, wParam, lParam);
		}
	}
	
	switch (uMsg)
	{
	case WM_CREATE:
		{
			RegisterPointerDeviceNotifications(m_hwnd, TRUE);
			RegisterTouchWindow(m_hwnd, 0);
			
			m_hwndEdit = CreateWindowEx(
				0, 
				TEXT("EDIT"),
				nullptr,
				WS_CHILD | WS_VISIBLE | WS_VSCROLL | ES_LEFT | ES_MULTILINE | ES_READONLY | ES_AUTOVSCROLL,
				0, 
				0, 
				0, 
				0,
				m_hwnd,
				reinterpret_cast<HMENU>(IDC_TEXTLOG),
				reinterpret_cast<HINSTANCE>(GetWindowLongPtr(m_hwnd, GWLP_HINSTANCE)),
				nullptr
			);

			HFONT hFontConsolas = CreateFont(18, 0, 0, 0, FW_DONTCARE, FALSE, FALSE, FALSE, DEFAULT_CHARSET, OUT_OUTLINE_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY, VARIABLE_PITCH, TEXT("Consolas"));
			SendMessage(m_hwndEdit, WM_SETFONT, reinterpret_cast<WPARAM>(hFontConsolas), MAKELPARAM(TRUE, 0));
			SendMessage(m_hwndEdit, EM_SETLIMITTEXT, 0, 0);

			m_hwndMotionEnabled = CreateWindowEx(
				0, 
				TEXT("BUTTON"), 
				TEXT("Enable Motion Events"), 
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
		}
		return 0;

	case WM_SIZE:
		{
			MoveWindow(m_hwndEdit, 0, 0, LOWORD(lParam), HIWORD(lParam) / 2, TRUE);
			MoveWindow(m_hwndMotionEnabled, 0, HIWORD(lParam) / 2, LOWORD(lParam), 40, TRUE);
		}
		return 0;

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
		case IDC_MOTIONENABLE:
			m_motionEnabled = !m_motionEnabled;
			Button_SetCheck(m_hwndMotionEnabled, m_motionEnabled);
			break;
		}
		break;

	MESSAGE_CASE(DM_POINTERHITTEST)
		break;

	MESSAGE_CASE(WM_NCPOINTERDOWN)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(HIWORD(wParam), "hit test value");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM_NCPOINTERUP)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(HIWORD(wParam), "hit test value");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM_NCPOINTERUPDATE)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(HIWORD(wParam), "hit test value");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM_POINTERACTIVATE)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(HIWORD(wParam), "hit test value");
			LOG_DERIVED(lParam, "window being activated");
		}
		break;

	MESSAGE_CASE(WM_POINTERCAPTURECHANGED)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(lParam, "window capturing pointer");
		}
		break;

	MESSAGE_CASE(WM_POINTERDEVICECHANGE)
		{
			LOG_DERIVED(PDC_STR(wParam), "pointer identifier");
		}
		break;

	MESSAGE_CASE(WM_POINTERDEVICEINRANGE)
		break;

	MESSAGE_CASE(WM_POINTERDEVICEOUTOFRANGE)
		break;

	MESSAGE_CASE(WM_POINTERDOWN)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(PointerState(wParam), "pointer state");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM_POINTERENTER)
		break;

	MESSAGE_CASE(WM_POINTERLEAVE)
		break;

	MESSAGE_CASE(WM_POINTERROUTEDAWAY)
		break;

	MESSAGE_CASE(WM_POINTERROUTEDRELEASED)
		break;

	MESSAGE_CASE(WM_POINTERROUTEDTO)
		break;

	MESSAGE_CASE(WM_POINTERUP)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(PointerState(wParam), "pointer state");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM_POINTERUPDATE)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(PointerState(wParam), "pointer state");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM_POINTERWHEEL)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(GET_WHEEL_DELTA_WPARAM(wParam), "wheel delta");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM_POINTERHWHEEL)
		{
			LOG_DERIVED(GET_POINTERID_WPARAM(wParam), "pointer identifier");
			LOG_DERIVED(GET_WHEEL_DELTA_WPARAM(wParam), "wheel delta");
			LOG_DERIVED(GET_X_LPARAM(lParam), "x coordinate");
			LOG_DERIVED(GET_Y_LPARAM(lParam), "y coordinate");
		}
		break;

	MESSAGE_CASE(WM_TOUCHHITTESTING)
		{
			const auto* info = reinterpret_cast<TOUCH_HIT_TESTING_INPUT*>(wParam);
			LOG_DERIVED(info->pointerId, "pointer identifier");
			LOG_DERIVED(info->point.x, "x coordinate");
			LOG_DERIVED(info->point.y, "y coordinate");
			LOG_DERIVED(info->orientation, "orientation");
		}
		break;

	default:
		break;
	}

#undef LOG_DERIVED
#undef MESSAGE_CASE

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