#define GMMODULE
#include "GarrysMod/Lua/Interface.h"
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <cstring>

using namespace GarrysMod::Lua;

static std::string g_botToken;
static CURL* g_curl = nullptr;

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    return size * nmemb;
}

void InitializeCurl() {
    if (!g_curl) {
        curl_global_init(CURL_GLOBAL_DEFAULT);
        g_curl = curl_easy_init();
    }
}

void CleanupCurl() {
    if (g_curl) {
        curl_easy_cleanup(g_curl);
        g_curl = nullptr;
        curl_global_cleanup();
    }
}

std::string EscapeJson(const std::string& s) {
    std::ostringstream o;
    for (auto c = s.cbegin(); c != s.cend(); c++) {
        switch (*c) {
        case '"': o << "\\\""; break;
        case '\\': o << "\\\\"; break;
        case '\b': o << "\\b"; break;
        case '\f': o << "\\f"; break;
        case '\n': o << "\\n"; break;
        case '\r': o << "\\r"; break;
        case '\t': o << "\\t"; break;
        default:
            if (*c < 0x20) {
                o << "\\u" << std::hex << std::uppercase << (int)*c;
            } else {
                o << *c;
            }
        }
    }
    return o.str();
}

bool SendDiscordRequest(const std::string& endpoint, const std::string& jsonData) {
    if (!g_curl || g_botToken.empty()) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Not initialized or invalid token\n");
        return false;
    }

    std::string url = "https://discord.com/api/v10" + endpoint;
    
    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(g_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(g_curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(g_curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 10L);
    
    struct curl_slist* headers = nullptr;
    std::string authHeader = "Authorization: Bot " + g_botToken;
    headers = curl_slist_append(headers, authHeader.c_str());
    headers = curl_slist_append(headers, "Content-Type: application/json");
    headers = curl_slist_append(headers, "User-Agent: DiscordBot (https://github.com, v1.0.0)");
    curl_easy_setopt(g_curl, CURLOPT_HTTPHEADER, headers);
    
    CURLcode res = curl_easy_perform(g_curl);
    curl_slist_free_all(headers);
    
    if (res != CURLE_OK) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Request failed: %s\n", curl_easy_strerror(res));
        return false;
    }
    
    long http_code = 0;
    curl_easy_getinfo(g_curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    if (http_code < 200 || http_code >= 300) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: HTTP %ld\n", http_code);
        return false;
    }
    
    return true;
}

int Initialize(lua_State* state) {
    if (!LUA->IsType(1, Type::STRING)) {
        LUA->PushBool(false);
        return 1;
    }
    
    g_botToken = LUA->GetString(1);
    InitializeCurl();
    
    printf("[ Xenor-Binaries ] [ DiscordGateway ] [ INFO ]: Initialized\n");
    LUA->PushBool(true);
    return 1;
}

int SendMessage(lua_State* state) {
    if (!LUA->IsType(1, Type::STRING) || !LUA->IsType(2, Type::STRING)) {
        LUA->PushBool(false);
        return 1;
    }
    
    std::string channelId = LUA->GetString(1);
    std::string message = LUA->GetString(2);
    
    std::string jsonData = "{\"content\":\"" + EscapeJson(message) + "\"}";
    std::string endpoint = "/channels/" + channelId + "/messages";
    
    bool success = SendDiscordRequest(endpoint, jsonData);
    LUA->PushBool(success);
    return 1;
}

int SendEmbed(lua_State* state) {
    if (!LUA->IsType(1, Type::STRING) || !LUA->IsType(2, Type::STRING) || 
        !LUA->IsType(3, Type::STRING) || !LUA->IsType(4, Type::NUMBER)) {
        LUA->PushBool(false);
        return 1;
    }
    
    std::string channelId = LUA->GetString(1);
    std::string title = LUA->GetString(2);
    std::string description = LUA->GetString(3);
    int color = (int)LUA->GetNumber(4);
    
    std::ostringstream json;
    json << "{\"embeds\":[{";
    json << "\"title\":\"" << EscapeJson(title) << "\",";
    json << "\"description\":\"" << EscapeJson(description) << "\",";
    json << "\"color\":" << color;
    json << "}]}";
    
    std::string endpoint = "/channels/" + channelId + "/messages";
    
    bool success = SendDiscordRequest(endpoint, json.str());
    LUA->PushBool(success);
    return 1;
}

GMOD_MODULE_OPEN() {
    LUA->PushSpecial(SPECIAL_GLOB);
    LUA->CreateTable();
    
    LUA->PushString("Initialize");
    LUA->PushCFunction(Initialize);
    LUA->SetTable(-3);
    
    LUA->PushString("SendMessage");
    LUA->PushCFunction(SendMessage);
    LUA->SetTable(-3);
    
    LUA->PushString("SendEmbed");
    LUA->PushCFunction(SendEmbed);
    LUA->SetTable(-3);
    
    LUA->SetField(-2, "DiscordGateway");
    LUA->Pop();
    
    return 0;
}

GMOD_MODULE_CLOSE() {
    CleanupCurl();
    return 0;
}