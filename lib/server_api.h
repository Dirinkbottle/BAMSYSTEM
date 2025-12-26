/**
 * @file server_api.h
 * @brief 服务器API接口头文件
 * @author BAMSYSTEM团队
 * @date 2025-11-08
 * @version 1.0
 */

#ifndef SERVER_API_H
#define SERVER_API_H

/* ==================== 标准库头文件 ==================== */
#include <stdbool.h>
#include <lib/account.h>

/* ==================== 枚举定义 ==================== */

/**
 * @brief 运行模式枚举
 */
typedef enum {
    MODE_LOCAL,      /**< 本地模式 - 不连接服务器 */
    MODE_SERVER,     /**< 服务器模式 - 联网同步 */
    MODE_UNKNOWN     /**< 未初始化状态 */
} RunMode;

/* ==================== 结构体定义 ==================== */

/**
 * @brief 服务器配置结构体
 */
typedef struct {
    char server_url[256];      /**< 服务器URL地址 */
    int port;                  /**< 服务器端口 */
    int timeout;               /**< 请求超时时间（秒） */
    bool use_https;            /**< 是否使用HTTPS */
    bool verify_cert;          /**< 是否验证服务器证书 */
    char cert_path[256];       /**< CA证书文件路径 */
    char client_id[65];        /**< 客户端唯一标识（SHA256哈希，64字符+\0） */
} ServerConfig;

/* ==================== 初始化与清理 ==================== */

/**
 * @brief 初始化服务器API模块
 * @return 成功返回true，失败返回false
 * @note 必须在使用任何API函数前调用 含有服务器API地址设置 属性字段
 */
bool init_server_api(void);

/**
 * @brief 清理服务器API模块
 * @note 程序退出前调用，释放资源
 */
void cleanup_server_api(void);

/* ==================== 配置管理 ==================== */

/**
 * @brief 加载服务器配置文件
 * @return 成功返回true，失败返回false
 * @note 从server.conf读取配置信息
 */
bool load_server_config(void);

/**
 * @brief 获取服务器配置
 * @return 返回配置结构体指针，失败返回NULL
 */
const ServerConfig* get_server_config(void);

/* ==================== 运行模式管理 ==================== */

/**
 * @brief 检测服务器可用性
 * @return 返回运行模式（MODE_SERVER或MODE_LOCAL）
 * @note 向服务器发送检测请求，根据响应确定运行模式
 */
RunMode check_server_availability(void);

/**
 * @brief 获取当前运行模式
 * @return 当前运行模式
 */
RunMode get_run_mode(void);

/**
 * @brief 设置运行模式
 * @param mode 要设置的运行模式
 */
void set_run_mode(RunMode mode);

/* ==================== 通用HTTP请求 ==================== */

/**
 * @brief 发送HTTP/HTTPS请求
 * @param endpoint API端点（如：/api/check）
 * @param method HTTP方法（GET/POST/DELETE等）
 * @param json_data 请求体JSON数据（可为NULL）
 * @return 返回响应JSON字符串，需调用者释放；失败返回NULL
 * @note 自动添加认证头和时间戳
 * @warning 调用者需要使用free()释放返回的字符串
 */
char* server_request(const char *endpoint, const char *method, const char *json_data);

/* ==================== 安全机制 ==================== */

/**
 * @brief 生成客户端唯一标识
 * @param client_id 输出缓冲区，至少65字节
 * @return 成功返回true，失败返回false
 * @note 使用system.key的SHA256哈希作为客户端ID
 */
bool generate_client_id(char *client_id);

/**
 * @brief 获取服务器公钥/证书
 * @return 成功返回true，失败返回false
 * @note 从服务器下载证书并保存到cert_path
 */
bool fetch_server_certificate(void);

/**
 * @brief 计算请求签名
 * @param data 要签名的数据
 * @param signature 输出签名字符串，至少65字节
 * @return 成功返回true，失败返回false
 * @note 使用HMAC-SHA256算法
 */
bool sign_request(const char *data, char *signature);

/* ==================== 业务API函数 ==================== */

/**
 * @brief 创建账户API
 * @param acc 账户结构体指针
 * @return 成功返回true，失败返回false
 * @note 将账户信息同步到服务器
 */
bool api_create_account(const ACCOUNT *acc);

/**
 * @brief 存款API
 * @param uuid 账户UUID
 * @param amount 存款金额（单位：分）
 * @return 成功返回true，失败返回false
 */
bool api_deposit(const char *uuid, LLUINT amount);

/**
 * @brief 取款API
 * @param uuid 账户UUID
 * @param amount 取款金额（单位：分）
 * @return 成功返回true，失败返回false
 */
bool api_withdraw(const char *uuid, LLUINT amount);

/**
 * @brief 转账API
 * @param uuid_from 转出账户UUID
 * @param uuid_to 转入账户UUID
 * @param amount 转账金额（单位：分）
 * @return 成功返回true，失败返回false
 */
bool api_transfer(const char *uuid_from, const char *uuid_to, LLUINT amount);

/**
 * @brief 销户API
 * @param uuid 账户UUID
 * @return 成功返回true，失败返回false
 */
bool api_delete_account(const char *uuid);

/**
 * @brief 同步账户数据API
 * @param acc 账户结构体指针
 * @return 成功返回true，失败返回false
 * @note 将本地账户数据完整同步到服务器
 */
bool api_sync_account(const ACCOUNT *acc);

/**
 * @brief 从服务器拉取所有账户
 * @param accounts 账户数组指针
 * @param max_count 最大账户数量
 * @return 实际获取的账户数量，失败返回-1
 * @note 从服务器获取所有账户列表
 */
int api_fetch_all_accounts(ACCOUNT *accounts, int max_count);

#endif /* SERVER_API_H */

