#pragma once

#include <cstddef>
#include <cstdint>
#include <string>

struct st_ClientLoginResult
{
    bool transportSuccess = false;
    bool success = false;
    long httpCode = 0;
    std::string code;
    std::string message;
    std::int64_t tokenValue = 0;
    std::string token;
    std::int64_t expiresAt = 0;
    std::string error;

    bool IsSuccess() const
    {
        return transportSuccess && httpCode == 200 && success;
    }
};

class CHttpClient
{
public:
    explicit CHttpClient(const std::string& baseUrl);
    bool Init();
    st_ClientLoginResult Login(
        const std::string& id, const std::string& password) const;

private:
    std::string baseUrl_;
    bool initialized_ = false;

    static size_t WriteResponseCallback(
        char* contents, size_t size, size_t count, void* userData) noexcept;
};
