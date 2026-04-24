struct DifferentSizedMember
{
#ifdef ONE
    char a;
#else
    wchar_t* a;
#endif
};
