#include "CHttpClient.h"

#include <curl/curl.h>
#include <nlohmann/json.hpp>
#include <chrono>
#include <memory>
#include <stdexcept>

namespace
{
    // One process-wide libcurl lifetime, shared by all client instances.
    class CurlRuntime
    {
    public:
        CurlRuntime() : result(curl_global_init(CURL_GLOBAL_DEFAULT)) {}
        ~CurlRuntime() { if (result == CURLE_OK) curl_global_cleanup(); }
        CURLcode result;
    };
}

CHttpClient::CHttpClient(const std::string& baseUrl) : baseUrl_(baseUrl)
{
    while (!baseUrl_.empty() && baseUrl_.back() == '/') baseUrl_.pop_back();
}

bool CHttpClient::Init()
{
    static CurlRuntime runtime;
    initialized_ = runtime.result == CURLE_OK;
    return initialized_;
}


st_ClientLoginResult CHttpClient::Login(
    const std::string& id, const std::string& password) const
{
    st_ClientLoginResult result;
    if (!initialized_)
    {
        result.error = "Call CHttpClient::Init before Login";
        return result;
    }
    if (id.empty() || password.empty())
    {
        result.error = "ID and password are required";
        return result;
    }

    using CurlHandle = std::unique_ptr<CURL, decltype(&curl_easy_cleanup)>;
    CurlHandle curl(curl_easy_init(), &curl_easy_cleanup);
    using HeaderList = std::unique_ptr<curl_slist, decltype(&curl_slist_free_all)>;
    HeaderList headers(curl_slist_append(nullptr, "Content-Type: application/json"),
        &curl_slist_free_all);
    if (!curl || !headers)
    {
        result.error = "Failed to allocate HTTP request";
        return result;
    }

    const std::string url = baseUrl_ + "/api/clientconnection/login";
    const std::string jsonString = nlohmann::json{
        {"id", id}, {"password", password}
    }.dump();
    std::string responseBody;

    curl_easy_setopt(curl.get(), CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_PROTOCOLS_STR, "http,https");
    curl_easy_setopt(curl.get(), CURLOPT_POST, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_HTTPHEADER, headers.get());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDS, jsonString.c_str());
    curl_easy_setopt(curl.get(), CURLOPT_POSTFIELDSIZE_LARGE,
        static_cast<curl_off_t>(jsonString.size()));
    curl_easy_setopt(curl.get(), CURLOPT_CONNECTTIMEOUT_MS, 1 * 1000 * 60);
    curl_easy_setopt(curl.get(), CURLOPT_TIMEOUT_MS, 1 * 1000 * 60);
    curl_easy_setopt(curl.get(), CURLOPT_NOSIGNAL, 1L);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEFUNCTION, WriteResponseCallback);
    curl_easy_setopt(curl.get(), CURLOPT_WRITEDATA, &responseBody);

    const CURLcode status = curl_easy_perform(curl.get());
    if (status != CURLE_OK)
    {
        result.error = curl_easy_strerror(status);
        return result;
    }

    result.transportSuccess = true;
    curl_easy_getinfo(curl.get(), CURLINFO_RESPONSE_CODE, &result.httpCode);
    try
    {
        const auto response = nlohmann::json::parse(responseBody);
        if (!response.is_object()) throw std::runtime_error("Expected JSON object");
        result.code = response.value("code", std::string{});
        result.message = response.value("message", response.value("title", std::string{}));
        const bool accepted = response.value("success", false);
        if (result.httpCode == 200 && accepted)
        {
			const auto tokenIt = response.find("tokenValue");
            const auto token = response.at("token").get<std::string>();
            const auto expiresAt = response.at("expiresAt").get<std::int64_t>();
            const auto now = std::chrono::duration_cast<std::chrono::milliseconds>(
                std::chrono::system_clock::now().time_since_epoch()).count();
            if (result.code != "SUCCESS" || token.empty() || expiresAt <= now)
                throw std::runtime_error("Invalid or expired login session");
            result.tokenValue = tokenIt != response.end() ? tokenIt.value().get<std::int64_t>() : 0;
            result.token = token;
            result.expiresAt = expiresAt;
            result.success = true;
        }
        else
        {
            result.error = "HTTP " + std::to_string(result.httpCode);
        }
    }
    catch (const std::exception&)
    {
        result.error = "Invalid LoginServer JSON response";
        result.success = false;
    }
    return result;
}

size_t CHttpClient::WriteResponseCallback(
    char* contents, size_t size, size_t count, void* userData) noexcept
{
    auto* response = static_cast<std::string*>(userData);
    constexpr size_t maxResponseBytes = 64 * 1024;
    if (size != 0 && count > maxResponseBytes / size) return 0;
    const size_t bytes = size * count;
    if (bytes > maxResponseBytes - response->size()) return 0;
    try
    {
        response->append(contents, bytes);
        return bytes;
    }
    catch (...) { return 0; }
}
