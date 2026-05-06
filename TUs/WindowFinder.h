#ifndef FIND_WINDOWS_H
#define FIND_WINDOWS_H

#include <windows.h>

namespace WindowFinder
{
    struct Is
    {
        inline static bool Visible(HWND hwnd) { return !!IsWindowVisible(hwnd); }
        inline static bool UnOwned(HWND hwnd) { return GetWindow(hwnd, GW_OWNER) == nullptr; }
        inline static bool Enabled(HWND hwnd) { return !!IsWindowEnabled(hwnd); }
        struct Nth
        {
            mutable int n;
            bool operator()(HWND) const
            {
                return --n == 0;
            }
        };
    };
    namespace Has
    {
        struct Pid
        {
            DWORD pid;
            bool operator()(HWND hwnd) const
            {
                DWORD dw = 0;
                GetWindowThreadProcessId(hwnd, &dw);
                return pid == dw;
            }
        };
        struct ClassName
        {
            const std::wstring className;
            bool operator()(HWND hwnd) const
            {
                wchar_t cls[256]{};
                GetClassNameW(hwnd, cls, static_cast<int>(std::size(cls)));
                return std::wcscmp(cls, className.c_str()) == 0;
            }
        };
        struct Caption
        {
            const std::wstring caption;
            bool operator()(HWND hwnd) const
            {
                wchar_t cap[256]{};
                GetWindowText(hwnd, cap, static_cast<int>(std::size(cap)));
                return std::wcscmp(cap, caption.c_str()) == 0;
            }
        };
        struct NotStyle
        {
            DWORD notThisStyle{};
            bool operator()(HWND hwnd) const
            {
                DWORD style = GetWindowLong(hwnd, GWL_STYLE);
                if (style & notThisStyle)
                    return false;
                return true;
            }
        };
    }

    namespace Contains
    {
        inline static bool ChildThatIsStaticIcon(HWND hwnd)
        {   // does HWND have a child that is a static icon

            bool found = false;
            EnumChildWindows(hwnd,  [](HWND child, LPARAM lParam) -> BOOL
                                    {
                                        wchar_t cls[32]{};
                                        if (GetClassNameW(child, cls, 32) != 0)
                                        {
                                            if (wcscmp(cls, L"Static") == 0)
                                            {
                                                LONG_PTR style = GetWindowLongPtrW(child, GWL_STYLE);
                                                if (style & SS_ICON)
                                                {
                                                    *reinterpret_cast<bool*>(lParam) = true;
                                                    return FALSE;
                                                }
                                            }
                                        }
                                        return TRUE;
                                    }, reinterpret_cast<LPARAM>(&found));
            return found;
        }
    }

    HWND FindDesiredChildWindow(HWND parent, auto... preds)
    {
        auto combined = [...ps = preds](HWND hwnd) { return (ps(hwnd) && ...); };
        struct Finder
        {
            decltype(combined) pred;
            HWND hwnd{};
        } finder{combined};

        EnumChildWindows(parent,[](HWND hWnd, LPARAM lParam) -> BOOL
                                {
                                    auto* finder = reinterpret_cast<Finder*>(lParam);
                                    if (!finder->pred(hWnd))
                                        return TRUE;
                                    finder->hwnd = hWnd;
                                    return FALSE;
                                }, reinterpret_cast<LPARAM>(&finder));
        return finder.hwnd;
    }
}
#endif



