#include <string>
#include <iostream>

#include "RakNet/BitStream.h"

// Функция конвертации UTF-8 → wstring
std::wstring to_wstring(const std::string& str) {
    if (str.empty()) return L"";

    int size_needed = MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), NULL, 0);
    std::wstring wstr(size_needed, 0);
    MultiByteToWideChar(CP_UTF8, 0, str.c_str(), (int)str.size(), &wstr[0], size_needed);
    return wstr;
}

template <typename T>
std::string read_with_size(RakNet::BitStream* bs) {
    T size;
    if (!bs->Read(size))
        return {};
    std::string str(size, '\0');
    bs->Read(str.data(), size);
    return str;
}

template <typename T>
void write_with_size(RakNet::BitStream* bs, std::string_view str) {
    T size = static_cast<T>(str.size());
    bs->Write(size);
    bs->Write(str.data(), size);
}