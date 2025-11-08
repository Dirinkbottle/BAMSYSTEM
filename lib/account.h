/**
 * @file account.h
 * @brief 账户管理模块头文件
 * @author BAMSYSTEM团队-Dirinkbottle
 * @date 2025-11-08
 * @version 1.0
 */

#ifndef ACCOUNT_H
#define ACCOUNT_H

/* ==================== 标准库头文件 ==================== */
#include <stdbool.h>
#include <lib/ui.h>
/* 跨平台UUID库 */
#ifdef _WIN32
    /* Windows平台必须先包含winsock2.h再包含windows.h */
    #include <winsock2.h>
    #include <windows.h>
    #include <rpc.h>
#else
    #include <uuid/uuid.h>
#endif

/* ==================== 类型定义 ==================== */

/** @brief 长长整型无符号数的类型别名 */
typedef long long unsigned int LLUINT; //最大取决于机器位数。可以尝试手动构建128位数，由于是演示目的足够了：）

/* ==================== 结构体定义 ==================== */

/**
 * @brief 账户结构体
 */
typedef struct account
{
    char UUID[37];      /** 唯一标识符（36字符+'\0'）8-4-4-4-12\0 */
    LLUINT PASSWORD;    /** 密码（7位数字） */
    LLUINT BALANCE;     /** 账户余额（单位：分） */
} ACCOUNT;



/* ==================== 系统初始化 ==================== */

/**
 * @brief 初始化账户系统
 * @return 成功返回true，失败返回false
 */
bool init_account_system(void);

/* ==================== UUID生成 ==================== */

/**
 * @brief 生成UUID v4字符串（跨平台）
 * @param uuid_str 输出缓冲区，至少37字节
 */
void generate_uuid_string(char *uuid_str);

/* ==================== 密钥管理 ==================== */

/**
 * @brief 生成并保存系统密钥
 * @return 成功返回true，失败返回false
 */
bool generate_system_key(void);

/**
 * @brief 加载系统密钥
 * @return 成功返回true，失败返回false
 */
bool load_system_key(void);

/* ==================== 加密解密 ==================== */

/**
 * @brief XOR加密/解密
 * @param data 要加密的数据
 * @param len 数据长度
 */
void xor_encrypt_decrypt(unsigned char *data, size_t len);

/* ==================== 文件操作 ==================== */

/**
 * @brief 创建Card文件夹
 * @return 成功返回true，失败返回false
 */
bool create_card_directory(void);

/**
 * @brief 保存账户到文件
 * @param acc 账户结构体指针
 * @return 成功返回true，失败返回false
 */
bool save_account(const ACCOUNT *acc);

/**
 * @brief 从文件加载账户
 * @param uuid 账户UUID
 * @param acc 输出账户结构体指针
 * @return 成功返回true，失败返回false
 */
bool load_account(const char *uuid, ACCOUNT *acc);

/**
 * @brief 列出所有账户
 * @return 账户数量
 */
int list_all_accounts(void);

/**
 * @brief 删除账户文件
 * @param uuid 账户UUID
 * @return 成功返回true，失败返回false
 */
bool delete_account_file(const char *uuid);

/* ==================== 业务功能 ==================== */

/**
 * @brief 创建账户
 * @param password 7位数字密码
 * @return 成功返回true，失败返回false
 */
bool create_account(LLUINT password);

/**
 * @brief 存款
 * @return 成功返回true，失败返回false
 */
bool deposit(void);

/**
 * @brief 取款
 * @return 成功返回true，失败返回false
 */
bool withdraw(void);

/**
 * @brief 转账
 * @return 成功返回true，失败返回false
 */
bool transfer(void);

/**
 * @brief 销户
 * @return 成功返回true，失败返回false
 */
bool delete_account(void);

/**
 * @brief 同步所有本地账户到服务器
 * @return 成功同步的账户数量
 * @note 仅在服务器模式下有效
 */
int sync_all_accounts_to_server(void);

/**
 * @brief 获取所有本地账户的UUID列表
 * @param uuids 输出UUID数组
 * @param max_count 最大数量
 * @return 实际账户数量
 */
int get_all_account_uuids(char uuids[][37], int max_count);

/**
 * @brief 从服务器拉取账户并保存到本地
 * @return 成功拉取并保存的账户数量
 * @note 仅在服务器模式下有效，会覆盖本地同UUID账户
 */
int pull_accounts_from_server(void);

#endif /* ACCOUNT_H */
