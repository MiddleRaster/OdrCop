#include <filesystem>
#include <iostream>
#include <vector>

#include "OdrCop.h"

int wmain(int argc, wchar_t** argv)
{
    if (argc < 2)
    {
        std::wcout << L"Usage: odrcop <folder of .obj/.pdb files> [more folders ...] where each .obj/.pdb is built with /Zi, /Fd\"blah.pdb\" and /Ob0\n";
        return -1;
    }

    std::vector<std::filesystem::path> pdbs;
    for (int i=1; i<argc; ++i)
    {
        std::filesystem::path root = argv[i];

        std::error_code ec;
        if (!std::filesystem::exists(root, ec) || !std::filesystem::is_directory(root, ec)) {
            std::wcerr << L"Path not found or not a directory: " << root.wstring() << L'\n';
            continue;
        }

        try {
            for (const auto& e : std::filesystem::recursive_directory_iterator(
                root, std::filesystem::directory_options::skip_permission_denied)) {
                if (e.is_regular_file() && e.path().extension() == ".pdb") {
                    pdbs.push_back(e.path());
                }
            }
        }
        catch (const std::filesystem::filesystem_error& ex) {
            std::wcerr << L"Filesystem error enumerating " << root.wstring() << L": " << ex.what() << L'\n';
            continue;
        }
    }
    if (pdbs.empty())
        return -1;

    Odr::Cop odrCop;

    for(auto& pdbPath : pdbs)
    {
        std::wcout << L"Loading: " << pdbPath << L'\n';
        HRESULT hr = odrCop.LoadPdb(pdbPath);
        if (FAILED(hr))
            std::wcerr << L"Failed to load: " << pdbPath << L" with HRESULT: 0x" << std::hex << hr << std::dec << L'\n';
    }
    std::wcout << L'\n';

    int violationCount = odrCop.ReportViolations();
    if (violationCount == 0)
        std::wcout                   << L"No ODR violations found.\n";
    else
        std::wcout << violationCount << L" ODR violation(s) found.\n";
    return 0;
}
