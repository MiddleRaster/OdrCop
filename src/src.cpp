

#include "OdrCop.h"

int wmain(int argc, wchar_t** argv)
{
    if (argc < 3)
    {
        std::wcout << L"Usage: odrcop <a.pdb> <b.pdb> [c.pdb ...] where each .cpp file/TU is built with /Zi and /Fd\"x64\\Debug\\%(Filename).pdb\"\n";
        return -1;
    }

    OdrCop odrCop;

    for (int i=1; i<argc; ++i)
    {
        std::wstring path(argv[i]);
        std::wcout << L"Loading: " << path << L'\n';
        HRESULT hr = odrCop.LoadPdb(path);
        if (FAILED(hr))
            std::wcerr << L"Failed to load: " << path << L" with HRESULT: 0x" << std::hex << hr << std::dec << L'\n';
    }
    std::wcout << L'\n';

    int violationCount = odrCop.ReportViolations();
    if (violationCount == 0)
        std::wcout                   << L"No ODR violations found.\n";
    else
        std::wcout << violationCount << L" ODR violation(s) found.\n";
    return 0;
}
