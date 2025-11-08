/**
 * @file server_api.c
 * @brief 服务器API模块实现
 * @author BAMSYSTEM团队
 * @date 2025-11-08
 * @version 1.0
 */

#include <lib/server_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

/* Windows平台必须先包含winsock2.h再包含windows.h */
#ifdef _WIN32
    #include <winsock2.h>
    #include <windows.h>
    #include <wincrypt.h>
    /* 注意: MinGW 使用 Makefile 链接库，不需要 #pragma comment */
#endif

#ifndef DISABLE_NETWORK
#include <curl/curl.h>
#include <cjson/cJSON.h>
#endif

#ifndef _WIN32
    #include <openssl/sha.h>
    #include <openssl/hmac.h>
#endif

/* ==================== 全局变量 ==================== */

static ServerConfig g_config;          /**< 服务器配置 */
static RunMode g_run_mode = MODE_UNKNOWN;  /**< 当前运行模式 */
static bool g_api_initialized = false;  /**< API是否已初始化 */

/* ==================== 内部辅助结构 ==================== */

/**
 * @brief HTTP响应数据缓冲区
 */
typedef struct {
    char *data;      /**< 数据指针 */
    size_t size;     /**< 数据大小 */
} ResponseBuffer;

/* ==================== 内部辅助函数声明 ==================== */

/* 这些函数总是需要的（用于 generate_client_id） */
static void sha256_hash(const unsigned char *data, size_t len, char *output);
static void bytes_to_hex(const unsigned char *bytes, size_t len, char *hex);

/* 这些函数仅在网络模式下需要 */
#ifndef DISABLE_NETWORK
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp);
static bool parse_config_line(const char *line, const char *section);
#endif

/* ==================== 初始化与清理 ==================== */

/**
 * @brief 初始化服务器API模块
 */
bool init_server_api(void)
{
    if (g_api_initialized) {
        return true;
    }
    
#ifndef DISABLE_NETWORK
    /* 初始化libcurl */
    CURLcode res = curl_global_init(CURL_GLOBAL_ALL);
    if (res != CURLE_OK) {
        fprintf(stderr, "错误：libcurl初始化失败: %s\n", curl_easy_strerror(res));
        return false;
    }
    
    /* 加载配置文件 */
    if (!load_server_config()) {
        fprintf(stderr, "警告：无法加载服务器配置，将使用本地模式\n");
        g_run_mode = MODE_LOCAL;
        g_api_initialized = true;
        return true;
    }
#else
    /* 网络功能已禁用，仅使用本地模式 */
    fprintf(stderr, "提示：程序编译时未启用网络功能，仅支持本地模式\n");
    g_run_mode = MODE_LOCAL;
#endif
    
    g_api_initialized = true;
    return true;
}

/**
 * @brief 清理服务器API模块
 */
void cleanup_server_api(void)
{
    if (g_api_initialized) {
#ifndef DISABLE_NETWORK
        curl_global_cleanup();
#endif
        g_api_initialized = false;
    }
}

/* ==================== 配置管理 ==================== */

#ifndef DISABLE_NETWORK

/**
 * @brief 去除字符串首尾空格、换行符和引号
 * @param str 要处理的字符串
 * @return 处理后的字符串指针
 */
static char* trim_string(char *str)
{
    char *end;
    
    /* 去除前导空格 */
    while (*str == ' ' || *str == '\t') {
        str++;
    }
    
    if (*str == '\0') {
        return str;
    }
    
    /* 去除尾部空格、换行符和回车符 */
    end = str + strlen(str) - 1;
    while (end > str && (*end == ' ' || *end == '\t' || *end == '\n' || *end == '\r')) {
        end--;
    }
    *(end + 1) = '\0';
    
    return str;
}

/**
 * @brief 解析配置文件行
 * @param line 配置行内容
 * @param section 当前所在的配置节
 */
static bool parse_config_line(const char *line, const char *section)
{
    char key[64], value[256];
    
    /* 跳过空行和注释 */
    if (line[0] == '\0' || line[0] == '#' || line[0] == '\n') {
        return true;
    }
    
    /* 解析键值对 */
    if (sscanf(line, "%63[^=]=%255[^\n]", key, value) == 2) {
        /* 去除首尾空格和换行符 */
        char *k = trim_string(key);
        char *v = trim_string(value);
        
        /* 根据节名处理不同的配置 */
        if (strcmp(section, "server") == 0) {
            if (strcmp(k, "url") == 0) {
                strncpy(g_config.server_url, v, sizeof(g_config.server_url) - 1);
            } else if (strcmp(k, "port") == 0) {
                g_config.port = atoi(v);
            } else if (strcmp(k, "timeout") == 0) {
                g_config.timeout = atoi(v);
            }
        } else if (strcmp(section, "security") == 0) {
            if (strcmp(k, "use_https") == 0) {
                g_config.use_https = (strcmp(v, "true") == 0);
            } else if (strcmp(k, "verify_cert") == 0) {
                g_config.verify_cert = (strcmp(v, "true") == 0);
            } else if (strcmp(k, "cert_path") == 0) {
                strncpy(g_config.cert_path, v, sizeof(g_config.cert_path) - 1);
            }
        } else if (strcmp(section, "client") == 0) {
            if (strcmp(k, "client_id") == 0) {
                strncpy(g_config.client_id, v, sizeof(g_config.client_id) - 1);
            }
        }
    }
    
    return true;
}

/**
 * @brief 加载服务器配置文件
 */
bool load_server_config(void)
{
    FILE *file = fopen("server.conf", "r");
    if (file == NULL) {
        fprintf(stderr, "[DEBUG] 无法打开配置文件 server.conf\n");
        return false;
    }
    
    printf("[DEBUG] 正在加载配置文件...\n");
    
    /* 初始化默认配置 */
    memset(&g_config, 0, sizeof(ServerConfig));
    g_config.port = 443;
    g_config.timeout = 10;
    g_config.use_https = true;
    g_config.verify_cert = true;
    
    char line[512];
    char current_section[64] = "";
    
    /* 逐行读取配置 */
    while (fgets(line, sizeof(line), file)) {
        /* 检测配置节 */
        if (line[0] == '[') {
            char *end = strchr(line, ']');
            if (end) {
                *end = '\0';
                strncpy(current_section, line + 1, sizeof(current_section) - 1);
                continue;
            }
        }
        
        parse_config_line(line, current_section);
    }
    
    fclose(file);
    
    /* 打印配置信息 */
    printf("[DEBUG] 配置加载完成:\n");
    printf("[DEBUG]   server_url = '%s'\n", g_config.server_url);
    printf("[DEBUG]   port = %d\n", g_config.port);
    printf("[DEBUG]   timeout = %d\n", g_config.timeout);
    printf("[DEBUG]   use_https = %s\n", g_config.use_https ? "true" : "false");
    printf("[DEBUG]   verify_cert = %s\n", g_config.verify_cert ? "true" : "false");
    printf("[DEBUG]   cert_path = '%s'\n", g_config.cert_path);
    
    /* 如果client_id为空，生成新的 */
    if (g_config.client_id[0] == '\0') {
        if (!generate_client_id(g_config.client_id)) {
            fprintf(stderr, "警告：无法生成客户端ID\n");
        } else {
            printf("[DEBUG]   client_id = %s\n", g_config.client_id);
        }
    } else {
        printf("[DEBUG]   client_id = %s\n", g_config.client_id);
    }
    
    return true;
}

/**
 * @brief 获取服务器配置
 */
const ServerConfig* get_server_config(void)
{
    return &g_config;
}

#else  /* DISABLE_NETWORK */

/* 网络功能禁用时的存根 */
bool load_server_config(void)
{
    return false;
}

const ServerConfig* get_server_config(void)
{
    return &g_config;
}

#endif  /* DISABLE_NETWORK */

/* ==================== 运行模式管理 ==================== */

/**
 * @brief 检测服务器可用性
 */
RunMode check_server_availability(void)
{
#ifdef DISABLE_NETWORK
    /* 网络功能已禁用 */
    printf("[DEBUG] 网络功能已禁用，使用本地模式\n");
    g_run_mode = MODE_LOCAL;
    return MODE_LOCAL;
#else
    if (!g_api_initialized) {
        printf("[DEBUG] API未初始化，使用本地模式\n");
        return MODE_LOCAL;
    }
    
    printf("[DEBUG] 正在检测服务器可用性...\n");
    
    /* 发送检测请求 */
    char *response = server_request("/api/check", "GET", NULL);
    if (response == NULL) {
        printf("[DEBUG] 服务器请求失败，切换到本地模式\n");
        g_run_mode = MODE_LOCAL;
        return MODE_LOCAL;
    }
    
    /* 解析JSON响应 */
    cJSON *json = cJSON_Parse(response);
    free(response);
    
    if (json == NULL) {
        printf("[DEBUG] JSON解析失败，切换到本地模式\n");
        g_run_mode = MODE_LOCAL;
        return MODE_LOCAL;
    }
    
    cJSON *status = cJSON_GetObjectItem(json, "status");
    if (status != NULL && cJSON_IsString(status)) {
        printf("[DEBUG] 服务器返回状态: %s\n", status->valuestring);
        if (strcmp(status->valuestring, "Support") == 0) {
            printf("[DEBUG] 服务器支持，切换到服务器模式\n");
            g_run_mode = MODE_SERVER;
            cJSON_Delete(json);
            return MODE_SERVER;
        }
    }
    
    cJSON_Delete(json);
    printf("[DEBUG] 服务器不支持或状态异常，切换到本地模式\n");
    g_run_mode = MODE_LOCAL;
    return MODE_LOCAL;
#endif
}

/**
 * @brief 获取当前运行模式
 */
RunMode get_run_mode(void)
{
    return g_run_mode;
}

/**
 * @brief 设置运行模式
 */
void set_run_mode(RunMode mode)
{
    g_run_mode = mode;
}

/* ==================== libcurl回调函数 ==================== */

#ifndef DISABLE_NETWORK

/**
 * @brief libcurl写入回调函数
 * @param contents 接收到的数据
 * @param size 数据块大小
 * @param nmemb 数据块数量
 * @param userp 用户数据指针
 * @return 实际处理的字节数
 */
static size_t write_callback(void *contents, size_t size, size_t nmemb, void *userp)
{
    size_t realsize = size * nmemb;
    ResponseBuffer *buffer = (ResponseBuffer *)userp;
    
    char *ptr = realloc(buffer->data, buffer->size + realsize + 1);
    if (ptr == NULL) {
        fprintf(stderr, "错误：内存分配失败\n");
        return 0;
    }
    
    buffer->data = ptr;
    memcpy(&(buffer->data[buffer->size]), contents, realsize);
    buffer->size += realsize;
    buffer->data[buffer->size] = '\0';
    
    return realsize;
}

/* ==================== 通用HTTP请求 ==================== */

/**
 * @brief 发送HTTP/HTTPS请求
 */
char* server_request(const char *endpoint, const char *method, const char *json_data)
{
    if (!g_api_initialized) {
        fprintf(stderr, "[DEBUG] API未初始化\n");
        return NULL;
    }
    
    CURL *curl = curl_easy_init();
    if (!curl) {
        fprintf(stderr, "错误：无法初始化CURL\n");
        return NULL;
    }
    
    /* 构建完整URL */
    char url[512];
    snprintf(url, sizeof(url), "%s%s", g_config.server_url, endpoint);
    
    printf("[DEBUG] HTTP请求信息:\n");
    printf("[DEBUG]   方法: %s\n", method);
    printf("[DEBUG]   端点: %s\n", endpoint);
    printf("[DEBUG]   完整URL: %s\n", url);
    if (json_data) {
        printf("[DEBUG]   请求体: %s\n", json_data);
    }
    
    /* 初始化响应缓冲区 */
    ResponseBuffer response;
    response.data = malloc(1);
    response.size = 0;
    
    /* 设置基本选项 */
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_callback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&response);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, g_config.timeout);
    
    /* 设置HTTP方法 */
    if (strcmp(method, "POST") == 0) {
        curl_easy_setopt(curl, CURLOPT_POST, 1L);
        if (json_data) {
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, json_data);
        }
    } else if (strcmp(method, "DELETE") == 0) {
        curl_easy_setopt(curl, CURLOPT_CUSTOMREQUEST, "DELETE");
    } else {
        curl_easy_setopt(curl, CURLOPT_HTTPGET, 1L);
    }
    
    /* 设置HTTP头 */
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers, "Content-Type: application/json");
    
    /* 添加认证头 */
    char auth_header[128];
    snprintf(auth_header, sizeof(auth_header), "X-Client-Key: %s", g_config.client_id);
    headers = curl_slist_append(headers, auth_header);
    
    /* 添加时间戳 */
    char time_header[64];
    snprintf(time_header, sizeof(time_header), "X-Request-Time: %ld", (long)time(NULL));
    headers = curl_slist_append(headers, time_header);
    
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    
    /* HTTPS配置 */
    if (g_config.use_https) {
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, g_config.verify_cert ? 1L : 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, g_config.verify_cert ? 2L : 0L);
        
        if (g_config.verify_cert && g_config.cert_path[0] != '\0') {
            curl_easy_setopt(curl, CURLOPT_CAINFO, g_config.cert_path);
        }
    }
    
    /* 执行请求 */
    printf("[DEBUG] 正在发送HTTP请求...\n");
    CURLcode res = curl_easy_perform(curl);
    
    /* 获取HTTP状态码 */
    long http_code = 0;
    curl_easy_getinfo(curl, CURLINFO_RESPONSE_CODE, &http_code);
    
    /* 清理 */
    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    
    if (res != CURLE_OK) {
        fprintf(stderr, "[DEBUG] CURL错误码: %d\n", res);
        fprintf(stderr, "错误：HTTP请求失败: %s\n", curl_easy_strerror(res));
        free(response.data);
        return NULL;
    }
    
    printf("[DEBUG] HTTP响应状态码: %ld\n", http_code);
    if (response.data && response.size > 0) {
        printf("[DEBUG] 响应内容: %s\n", response.data);
    }
    
    return response.data;
}

#endif  /* DISABLE_NETWORK - 结束网络功能块 */

/* ==================== 安全机制 ==================== */
/* 这些函数在所有模式下都需要（用于 generate_client_id） */

/**
 * @brief 字节数组转十六进制字符串
 * @param bytes 字节数组
 * @param len 字节数组长度
 * @param hex 输出十六进制字符串
 */
static void bytes_to_hex(const unsigned char *bytes, size_t len, char *hex)
{
    for (size_t i = 0; i < len; i++) {
        sprintf(hex + i * 2, "%02x", bytes[i]);
    }
    hex[len * 2] = '\0';
}

/**
 * @brief SHA256哈希计算
 * @param data 输入数据
 * @param len 数据长度
 * @param output 输出哈希字符串（65字节）
 */
static void sha256_hash(const unsigned char *data, size_t len, char *output)
{
#ifdef _WIN32
    /* Windows使用CryptoAPI */
    HCRYPTPROV hProv = 0;
    HCRYPTHASH hHash = 0;
    BYTE hash[32];
    DWORD hashLen = 32;
    
    if (CryptAcquireContext(&hProv, NULL, NULL, PROV_RSA_AES, CRYPT_VERIFYCONTEXT)) {
        if (CryptCreateHash(hProv, CALG_SHA_256, 0, 0, &hHash)) {
            if (CryptHashData(hHash, data, (DWORD)len, 0)) {
                if (CryptGetHashParam(hHash, HP_HASHVAL, hash, &hashLen, 0)) {
                    bytes_to_hex(hash, 32, output);
                }
            }
            CryptDestroyHash(hHash);
        }
        CryptReleaseContext(hProv, 0);
    }
#else
    /* Linux使用OpenSSL */
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256(data, len, hash);
    bytes_to_hex(hash, SHA256_DIGEST_LENGTH, output);
#endif
}

/**
 * @brief 生成客户端唯一标识
 */
bool generate_client_id(char *client_id)
{
    printf("[DEBUG] 正在生成客户端ID...\n");
    
    /* 读取system.key */
    FILE *file = fopen("system.key", "rb");
    if (file == NULL) {
        fprintf(stderr, "[DEBUG] 无法打开 system.key 文件\n");
        fprintf(stderr, "[DEBUG] 请确保 system.key 文件存在于程序运行目录\n");
        return false;
    }
    
    printf("[DEBUG] system.key 文件打开成功\n");
    
    unsigned char key_data[16];
    size_t bytes_read = fread(key_data, 1, 16, file);
    fclose(file);
    
    if (bytes_read != 16) {
        fprintf(stderr, "[DEBUG] system.key 文件大小错误（读取了 %zu 字节，需要16字节）\n", bytes_read);
        return false;
    }
    
    printf("[DEBUG] system.key 读取成功，正在计算SHA256哈希...\n");
    
    /* 计算SHA256哈希 */
    sha256_hash(key_data, 16, client_id);
    
    printf("[DEBUG] 客户端ID生成成功: %s\n", client_id);
    
    return true;
}

#ifndef DISABLE_NETWORK

/**
 * @brief 获取服务器公钥/证书
 */
bool fetch_server_certificate(void)
{
    /* 发送请求获取证书 */
    char *response = server_request("/api/public_key", "GET", NULL);
    if (response == NULL) {
        return false;
    }
    
    /* 解析JSON */
    cJSON *json = cJSON_Parse(response);
    free(response);
    
    if (json == NULL) {
        return false;
    }
    
    cJSON *cert = cJSON_GetObjectItem(json, "certificate");
    if (cert == NULL || !cJSON_IsString(cert)) {
        cJSON_Delete(json);
        return false;
    }
    
    /* 保存证书到文件 */
    FILE *file = fopen(g_config.cert_path, "w");
    if (file == NULL) {
        cJSON_Delete(json);
        return false;
    }
    
    fprintf(file, "%s", cert->valuestring);
    fclose(file);
    
    cJSON_Delete(json);
    
    printf("证书已保存到: %s\n", g_config.cert_path);
    printf("建议验证证书指纹以确保安全\n");
    
    return true;
}

/**
 * @brief 计算请求签名
 */
bool sign_request(const char *data, char *signature)
{
    /* 读取system.key作为密钥 */
    FILE *file = fopen("system.key", "rb");
    if (file == NULL) {
        return false;
    }
    
    unsigned char key[16];
    if (fread(key, 1, 16, file) != 16) {
        fclose(file);
        return false;
    }
    fclose(file);
    
#ifdef _WIN32
    /* Windows: 简化实现，使用SHA256 */
    char combined[1024];
    snprintf(combined, sizeof(combined), "%s%s", (char*)key, data);
    sha256_hash((unsigned char*)combined, strlen(combined), signature);
#else
    /* Linux: 使用HMAC-SHA256 */
    unsigned char hmac[32];
    unsigned int hmac_len;
    HMAC(EVP_sha256(), key, 16, (unsigned char*)data, strlen(data), hmac, &hmac_len);
    bytes_to_hex(hmac, hmac_len, signature);
#endif
    
    return true;
}

/* ==================== 业务API函数 ==================== */

/**
 * @brief 创建账户API
 */
bool api_create_account(const ACCOUNT *acc)
{
    if (g_run_mode != MODE_SERVER) {
        return false;
    }
    
    /* 构建JSON */
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "uuid", acc->UUID);
    cJSON_AddNumberToObject(json, "balance", (double)acc->BALANCE);
    cJSON_AddNumberToObject(json, "timestamp", (double)time(NULL));
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    /* 发送请求 */
    char *response = server_request("/api/account/create", "POST", json_str);
    free(json_str);
    
    if (response == NULL) {
        return false;
    }
    
    /* 解析响应 */
    cJSON *resp_json = cJSON_Parse(response);
    free(response);
    
    if (resp_json == NULL) {
        return false;
    }
    
    cJSON *success = cJSON_GetObjectItem(resp_json, "success");
    bool result = (success != NULL && cJSON_IsTrue(success));
    
    cJSON_Delete(resp_json);
    return result;
}

/**
 * @brief 存款API
 */
bool api_deposit(const char *uuid, LLUINT amount)
{
    if (g_run_mode != MODE_SERVER) {
        return false;
    }
    
    /* 构建JSON */
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "uuid", uuid);
    cJSON_AddNumberToObject(json, "amount", (double)amount);
    cJSON_AddNumberToObject(json, "timestamp", (double)time(NULL));
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    /* 发送请求 */
    char *response = server_request("/api/account/deposit", "POST", json_str);
    free(json_str);
    
    if (response == NULL) {
        return false;
    }
    
    /* 解析响应 */
    cJSON *resp_json = cJSON_Parse(response);
    free(response);
    
    if (resp_json == NULL) {
        return false;
    }
    
    cJSON *success = cJSON_GetObjectItem(resp_json, "success");
    bool result = (success != NULL && cJSON_IsTrue(success));
    
    cJSON_Delete(resp_json);
    return result;
}

/**
 * @brief 取款API
 */
bool api_withdraw(const char *uuid, LLUINT amount)
{
    if (g_run_mode != MODE_SERVER) {
        return false;
    }
    
    /* 构建JSON */
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "uuid", uuid);
    cJSON_AddNumberToObject(json, "amount", (double)amount);
    cJSON_AddNumberToObject(json, "timestamp", (double)time(NULL));
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    /* 发送请求 */
    char *response = server_request("/api/account/withdraw", "POST", json_str);
    free(json_str);
    
    if (response == NULL) {
        return false;
    }
    
    /* 解析响应 */
    cJSON *resp_json = cJSON_Parse(response);
    free(response);
    
    if (resp_json == NULL) {
        return false;
    }
    
    cJSON *success = cJSON_GetObjectItem(resp_json, "success");
    bool result = (success != NULL && cJSON_IsTrue(success));
    
    cJSON_Delete(resp_json);
    return result;
}

/**
 * @brief 转账API
 */
bool api_transfer(const char *uuid_from, const char *uuid_to, LLUINT amount)
{
    if (g_run_mode != MODE_SERVER) {
        return false;
    }
    
    /* 构建JSON */
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "uuid_from", uuid_from);
    cJSON_AddStringToObject(json, "uuid_to", uuid_to);
    cJSON_AddNumberToObject(json, "amount", (double)amount);
    cJSON_AddNumberToObject(json, "timestamp", (double)time(NULL));
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    /* 发送请求 */
    char *response = server_request("/api/account/transfer", "POST", json_str);
    free(json_str);
    
    if (response == NULL) {
        return false;
    }
    
    /* 解析响应 */
    cJSON *resp_json = cJSON_Parse(response);
    free(response);
    
    if (resp_json == NULL) {
        return false;
    }
    
    cJSON *success = cJSON_GetObjectItem(resp_json, "success");
    bool result = (success != NULL && cJSON_IsTrue(success));
    
    cJSON_Delete(resp_json);
    return result;
}

/**
 * @brief 销户API
 */
bool api_delete_account(const char *uuid)
{
    if (g_run_mode != MODE_SERVER) {
        return false;
    }
    
    /* 构建URL */
    char endpoint[128];
    snprintf(endpoint, sizeof(endpoint), "/api/account/%s", uuid);
    
    /* 发送请求 */
    char *response = server_request(endpoint, "DELETE", NULL);
    
    if (response == NULL) {
        return false;
    }
    
    /* 解析响应 */
    cJSON *resp_json = cJSON_Parse(response);
    free(response);
    
    if (resp_json == NULL) {
        return false;
    }
    
    cJSON *success = cJSON_GetObjectItem(resp_json, "success");
    bool result = (success != NULL && cJSON_IsTrue(success));
    
    cJSON_Delete(resp_json);
    return result;
}

/**
 * @brief 同步账户数据API
 */
bool api_sync_account(const ACCOUNT *acc)
{
    if (g_run_mode != MODE_SERVER) {
        return false;
    }
    
    /* 构建JSON */
    cJSON *json = cJSON_CreateObject();
    cJSON_AddStringToObject(json, "uuid", acc->UUID);
    cJSON_AddNumberToObject(json, "balance", (double)acc->BALANCE);
    cJSON_AddNumberToObject(json, "timestamp", (double)time(NULL));
    
    char *json_str = cJSON_Print(json);
    cJSON_Delete(json);
    
    /* 发送请求 */
    char *response = server_request("/api/account/sync", "POST", json_str);
    free(json_str);
    
    if (response == NULL) {
        return false;
    }
    
    /* 解析响应 */
    cJSON *resp_json = cJSON_Parse(response);
    free(response);
    
    if (resp_json == NULL) {
        return false;
    }
    
    cJSON *success = cJSON_GetObjectItem(resp_json, "success");
    bool result = (success != NULL && cJSON_IsTrue(success));
    
    cJSON_Delete(resp_json);
    return result;
}

/**
 * @brief 从服务器拉取所有账户
 */
int api_fetch_all_accounts(ACCOUNT *accounts, int max_count)
{
    if (g_run_mode != MODE_SERVER) {
        return -1;
    }
    
    printf("[拉取] 正在从服务器获取账户列表...\n");
    
    /* 发送请求 */
    char *response = server_request("/api/accounts", "GET", NULL);
    if (response == NULL) {
        fprintf(stderr, "[拉取] 服务器请求失败\n");
        return -1;
    }
    
    /* 解析响应 */
    cJSON *resp_json = cJSON_Parse(response);
    free(response);
    
    if (resp_json == NULL) {
        fprintf(stderr, "[拉取] JSON解析失败\n");
        return -1;
    }
    
    cJSON *success = cJSON_GetObjectItem(resp_json, "success");
    if (success == NULL || !cJSON_IsTrue(success)) {
        fprintf(stderr, "[拉取] 服务器返回失败\n");
        cJSON_Delete(resp_json);
        return -1;
    }
    
    cJSON *accounts_array = cJSON_GetObjectItem(resp_json, "accounts");
    if (accounts_array == NULL || !cJSON_IsArray(accounts_array)) {
        fprintf(stderr, "[拉取] 账户列表格式错误\n");
        cJSON_Delete(resp_json);
        return -1;
    }
    
    int count = 0;
    int array_size = cJSON_GetArraySize(accounts_array);
    
    for (int i = 0; i < array_size && count < max_count; i++) {
        cJSON *item = cJSON_GetArrayItem(accounts_array, i);
        if (item == NULL) continue;
        
        cJSON *uuid = cJSON_GetObjectItem(item, "uuid");
        cJSON *balance = cJSON_GetObjectItem(item, "balance");
        
        if (uuid != NULL && cJSON_IsString(uuid) && 
            balance != NULL && cJSON_IsNumber(balance)) {
            
            strncpy(accounts[count].UUID, uuid->valuestring, sizeof(accounts[count].UUID) - 1);
            accounts[count].UUID[36] = '\0';
            accounts[count].BALANCE = (LLUINT)balance->valuedouble;
            accounts[count].PASSWORD = 0;  /* 服务器不存储密码 */
            
            count++;
        }
    }
    
    cJSON_Delete(resp_json);
    
    printf("[拉取] 成功获取 %d 个服务器账户\n", count);
    return count;
}

#else  /* DISABLE_NETWORK 定义时的存根实现 */

/* ==================== 网络功能禁用时的存根实现 ==================== */

char* server_request(const char *endpoint, const char *method, const char *json_data)
{
    (void)endpoint;
    (void)method;
    (void)json_data;
    return NULL;
}

bool fetch_server_certificate(void)
{
    fprintf(stderr, "错误：网络功能已禁用\n");
    return false;
}

bool sign_request(const char *data, char *signature)
{
    (void)data;
    (void)signature;
    return false;
}

bool api_create_account(const ACCOUNT *acc)
{
    (void)acc;
    return false;
}

bool api_deposit(const char *uuid, LLUINT amount)
{
    (void)uuid;
    (void)amount;
    return false;
}

bool api_withdraw(const char *uuid, LLUINT amount)
{
    (void)uuid;
    (void)amount;
    return false;
}

bool api_transfer(const char *uuid_from, const char *uuid_to, LLUINT amount)
{
    (void)uuid_from;
    (void)uuid_to;
    (void)amount;
    return false;
}

bool api_delete_account(const char *uuid)
{
    (void)uuid;
    return false;
}

bool api_sync_account(const ACCOUNT *acc)
{
    (void)acc;
    return false;
}

int api_fetch_all_accounts(ACCOUNT *accounts, int max_count)
{
    (void)accounts;
    (void)max_count;
    return -1;
}

#endif  /* DISABLE_NETWORK */
