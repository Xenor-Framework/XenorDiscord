#define GMMODULE
#include "GarrysMod/Lua/Interface.h"
#include <curl/curl.h>
#include <string>
#include <sstream>
#include <cstring>

using namespace GarrysMod::Lua;

static std::string g_botToken;
static CURL* g_curl = nullptr;


// Util
struct ResponseData {
    std::string data;
};

size_t WriteCallback(void* contents, size_t size, size_t nmemb, void* userp) {
    ResponseData* response = static_cast<ResponseData*>(userp);
    size_t totalSize = size * nmemb;
    response->data.append(static_cast<char*>(contents), totalSize);
    return totalSize;
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

// JSON Handling
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

std::string ExtractJsonField(const std::string& json, const std::string& field) {
    std::string searchKey = "\"" + field + "\":";
    size_t pos = json.find(searchKey);
    if (pos == std::string::npos) return "";
    
    pos += searchKey.length();
    while (pos < json.length() && (json[pos] == ' ' || json[pos] == '\t')) pos++;
    
    if (pos >= json.length()) return "";
    
    if (json[pos] == '"') {
        pos++;
        size_t endPos = json.find('"', pos);
        if (endPos != std::string::npos) {
            return json.substr(pos, endPos - pos);
        }
    } else {
        size_t endPos = pos;
        while (endPos < json.length() && json[endPos] != ',' && json[endPos] != '}' && json[endPos] != ' ') {
            endPos++;
        }
        return json.substr(pos, endPos - pos);
    }
    return "";
}

// Communication
bool SendDiscordRequest(const std::string& endpoint, const std::string& jsonData) {
    if (!g_curl || g_botToken.empty()) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Not initialized or invalid token\n");
        return false;
    }

    std::string url = "https://discord.com/api/v10" + endpoint;
    ResponseData response;
    
    curl_easy_reset(g_curl);
    curl_easy_setopt(g_curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(g_curl, CURLOPT_POST, 1L);
    curl_easy_setopt(g_curl, CURLOPT_POSTFIELDS, jsonData.c_str());
    curl_easy_setopt(g_curl, CURLOPT_WRITEFUNCTION, WriteCallback);
    curl_easy_setopt(g_curl, CURLOPT_WRITEDATA, &response);
    curl_easy_setopt(g_curl, CURLOPT_SSL_VERIFYPEER, 1L);
    curl_easy_setopt(g_curl, CURLOPT_SSL_VERIFYHOST, 2L);
    curl_easy_setopt(g_curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(g_curl, CURLOPT_CONNECTTIMEOUT, 10L);
    curl_easy_setopt(g_curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(g_curl, CURLOPT_MAXREDIRS, 3L);
    
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
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: HTTP %ld", http_code);
        
        if (!response.data.empty()) {
            std::string errorMsg = ExtractJsonField(response.data, "message");
            std::string errorCode = ExtractJsonField(response.data, "code");
            
            if (!errorMsg.empty()) {
                printf(" - %s", errorMsg.c_str());
            }
            if (!errorCode.empty()) {
                printf(" (Code: %s)", errorCode.c_str());
            }
            
            if (http_code == 401) {
                printf(" - Check your bot token");
            } else if (http_code == 403) {
                printf(" - Bot lacks permissions or channel access");
            } else if (http_code == 404) {
                printf(" - Channel not found or bot not in server");
            } else if (http_code == 429) {
                printf(" - Rate limited");
            }
        }
        printf("\n");
        return false;
    }
    
    return true;
}

bool IsValidChannelId(const std::string& channelId) {
    if (channelId.empty() || channelId.length() < 15 || channelId.length() > 20) {
        return false;
    }
    
    for (char c : channelId) {
        if (!std::isdigit(c)) {
            return false;
        }
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

bool IsValidChannelId(const std::string& channelId) {
    if (channelId.empty() || channelId.length() < 15 || channelId.length() > 20) {
        return false;
    }
    
    for (char c : channelId) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}

int SendMessage(lua_State* state) {
    if (!LUA->IsType(1, Type::STRING) || !LUA->IsType(2, Type::STRING)) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Invalid arguments - expected (string) CHANNEL_ID, (string) MESSAGE\n");
        LUA->PushBool(false);
        return 1;
    }
    
    std::string channelId = LUA->GetString(1);
    std::string message = LUA->GetString(2);
    
    if (!IsValidChannelId(channelId)) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Invalid channel ID format\n");
        LUA->PushBool(false);
        return 1;
    }
    
    if (message.empty() || message.length() > 2000) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Message too long (max 2000 characters)\n");
        LUA->PushBool(false);
        return 1;
    }
    
    std::string jsonData = "{\"content\":\"" + EscapeJson(message) + "\"}";
    std::string endpoint = "/channels/" + channelId + "/messages";
    
    bool success = SendDiscordRequest(endpoint, jsonData);
    LUA->PushBool(success);
    return 1;
}

int SendEmbed(lua_State* state) {
    if (!LUA->IsType(1, Type::STRING) || !LUA->IsType(2, Type::STRING) || 
        !LUA->IsType(3, Type::STRING) || !LUA->IsType(4, Type::NUMBER)) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Invalid arguments - expected (string, string, string, number)\n");
        LUA->PushBool(false);
        return 1;
    }
    
    std::string channelId = LUA->GetString(1);
    std::string title = LUA->GetString(2);
    std::string description = LUA->GetString(3);
    int color = (int)LUA->GetNumber(4);
    
    if (!IsValidChannelId(channelId)) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Invalid channel ID format\n");
        LUA->PushBool(false);
        return 1;
    }
    
    if (title.length() > 256) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Embed title too long (max 256 characters)\n");
        LUA->PushBool(false);
        return 1;
    }
    
    if (description.length() > 4096) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Embed description too long (max 4096 characters)\n");
        LUA->PushBool(false);
        return 1;
    }
    
    if (color < 0 || color > 16777215) {
        printf("[ Xenor-Binaries ] [ DiscordGateway ] [ ERROR ]: Invalid color value (0-16777215)\n");
        LUA->PushBool(false);
        return 1;
    }
    
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
