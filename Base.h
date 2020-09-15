#pragma once

#include <windows.h>
#include <windowsx.h>
#include <fmt/core.h>
#include <string>

constexpr int g_ThrottleMilliseconds = 500;

#ifdef UNICODE
#include <locale>

static std::wstring ToWinString(std::string_view utf8)
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
		WNDCLASS wc = { 0 };

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