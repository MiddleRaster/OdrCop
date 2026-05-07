#pragma once

namespace Odr
{
    inline std::wstring BstrToWstr(BSTR b) { return b ? std::wstring(b) : std::wstring(L"<unnamed>"); }

    template <typename C, typename R, typename T> T Get(IDiaSymbol* sym, R(C::* m)(T*))
    {
        T value{};
        (sym->*m)(&value);
        return value;
    }
    template <typename R> CComBSTR Get(IDiaSymbol* sym, R(IDiaSymbol::* m)(BSTR*))
    {
        CComBSTR value{};
        (sym->*m)(&value.m_str);
        return value;
    }
    template <typename R> CComPtr<IDiaSymbol> Get(IDiaSymbol* sym, R(IDiaSymbol::* m)(IDiaSymbol*))
    {
        CComPtr<IDiaSymbol> value{};
        (sym->*m)(&value);
        return value;
    }
    template <typename R> std::wstring Get(IDiaSymbol* sym, R(IDiaSymbol::* m)(VARIANT*))
    {
        CComVariant value{};
        HRESULT hr = (sym->*m)(&value);
        if ((hr != S_OK && hr != S_FALSE) || FAILED(value.ChangeType(VT_BSTR)) || !value.bstrVal)
            return L"?";
        return std::wstring(value.bstrVal);
    }

    template <typename C, typename R, typename T> T GetN(ATL::CComPtr<C>& sym, R(C::* m)(T*))
    {
        T value{};
        (sym->*m)(&value);
        return value;
    }
}
