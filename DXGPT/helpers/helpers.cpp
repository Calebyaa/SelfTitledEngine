#include "helpers.h"

std::wstring helpers::GetErrorMessageFromHRESULT(HRESULT hr)  {
    LPWSTR messageBuffer = nullptr;
    DWORD bufferLength = FormatMessageW(
        FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
        nullptr,
        hr,
        MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
        (LPWSTR)&messageBuffer,
        0,
        nullptr
    );

    std::wstring errorMessage;

    if (bufferLength) {
        errorMessage.assign(messageBuffer, bufferLength);
        LocalFree(messageBuffer);
    } else {
        errorMessage = L"Unknown error";
    }

    return errorMessage;
}