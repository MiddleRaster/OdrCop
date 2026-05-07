// Regression tests

#include <windows.h>
#include <string>
#include <iostream>


// especially tricky code:  a locally (to a function template) defined struct, using template parameter pack
// copying such code here so that it's included in my regression suite
#include "WindowFinder.h"
void TestWindowFinder()
{
    HWND hwnd = WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::ClassName{ L"#32770" });
    if (hwnd == nullptr)    hwnd = WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::ClassName{ L"#32770" }, WindowFinder::Is::Visible);
    if (hwnd == nullptr)    hwnd = WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::Pid{ ::GetCurrentProcessId() });
    if (hwnd == nullptr)    hwnd = WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Is::Enabled);
    if (hwnd == nullptr)    hwnd = WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Is::UnOwned);
    if (hwnd == nullptr)    hwnd = WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Is::Enabled, WindowFinder::Is::UnOwned);
    if (hwnd == nullptr)    hwnd = WindowFinder::FindDesiredChildWindow(nullptr, WindowFinder::Has::ClassName{ L"#32770" }, WindowFinder::Is::Enabled, WindowFinder::Is::UnOwned);

    std::wcout << L"WindowFinder found 0x" << std::hex << hwnd << std::dec << L'\n';
}


std::wstring ReturningAnStlTypeIsNotExcludedWhenUsingSwitch() { return L"hi"; }

