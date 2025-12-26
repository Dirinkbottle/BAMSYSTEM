/**
 * @file account.c
 * @brief 账户管理模块实现
 * @author BAMSYSTEM团队-Dirinkbottle
 * @date 2025-11-08
 * @version 1.0
 */

#include <lib/account.h>
#include <lib/server_api.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#ifdef _WIN32
    #include <direct.h>
    #include <io.h>
    #define mkdir(path, mode) _mkdir(path) //windows平台兼容，不用改代码了哈哈哈
    #define access(path, mode) _access(path, mode) //windows兼容
    #define F_OK 0 //文件存在标识符
#else
    #include <dirent.h>
    #include <unistd.h>
#endif

/* ==================== 全局变量 ==================== */

static unsigned char g_system_key[16];  /* 128位系统密钥 */
static bool g_key_loaded = false;       /* 密钥是否已加载 */
static AccountHashTable g_hash_table;   /* 全局账户 Hash 表 */
static bool g_hash_table_initialized = false;  /* Hash 表是否已初始化 */

/* ==================== Hash 表常量配置 ==================== */

#define INITIAL_HASH_TABLE_SIZE 16    /* 初始桶数量 */
#define LOAD_FACTOR_THRESHOLD 0.75    /* 扩容阈值 */

/* ==================== Hash 表内部函数声明 ==================== */

static unsigned long hash_function(const char *str, size_t table_size);
static double calculate_load_factor(void);
static bool resize_hash_table(void);

/* ==================== Hash 表实现 ==================== */

/**
 * @brief Hash 函数（DJB2 算法）
 * @param str 字符串（UUID）
 * @param table_size 表大小
 * @return Hash 值（桶索引）
 */
static unsigned long hash_function(const char *str, size_t table_size)
{
    unsigned long hash = 5381;
    int c;
    
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c; /* hash * 33 + c */
    }
    
    return hash % table_size;
}

/**
 * @brief 计算当前负载因子
 * @return 负载因子
 */
static double calculate_load_factor(void)
{
    if (g_hash_table.size == 0) {
        return 0.0;
    }
    return (double)g_hash_table.count / (double)g_hash_table.size;
}

/**
 * @brief 扩容 Hash 表
 * @return 成功返回true，失败返回false
 */
static bool resize_hash_table(void)
{
    size_t new_size = g_hash_table.size * 2;
    
    printf("[Hash] 正在扩容 Hash 表：%zu -> %zu\n", g_hash_table.size, new_size);
    
    /* 分配新的桶数组 */
    AccountNode **new_buckets = (AccountNode **)calloc(new_size, sizeof(AccountNode *));
    if (new_buckets == NULL) {
        fprintf(stderr, "错误：扩容 Hash 表时内存分配失败\n");
        return false;
    }
    
    /* 重新哈希所有节点 */
    for (size_t i = 0; i < g_hash_table.size; i++) {
        AccountNode *current = g_hash_table.buckets[i];
        
        while (current != NULL) {
            AccountNode *next = current->next;
            
            /* 计算新的桶索引 */
            unsigned long new_index = hash_function(current->account.UUID, new_size);
            
            /* 插入到新桶（头插法） */
            current->next = new_buckets[new_index];
            new_buckets[new_index] = current;
            
            current = next;
        }
    }
    
    /* 释放旧桶数组 */
    free(g_hash_table.buckets);
    
    /* 更新 Hash 表 */
    g_hash_table.buckets = new_buckets;
    g_hash_table.size = new_size;
    
    printf("[Hash] 扩容完成，当前负载因子: %.2f\n", calculate_load_factor());
    
    return true;
}

/**
 * @brief 初始化账户 Hash 表
 */
bool init_account_hash_table(void)
{
    if (g_hash_table_initialized) {
        return true;
    }
    
    printf("[Hash] 正在初始化账户 Hash 表...\n");
    
    /* 分配桶数组 */
    g_hash_table.buckets = (AccountNode **)calloc(INITIAL_HASH_TABLE_SIZE, sizeof(AccountNode *));
    if (g_hash_table.buckets == NULL) {
        fprintf(stderr, "错误：Hash 表初始化失败（内存分配失败）\n");
        return false;
    }
    
    g_hash_table.size = INITIAL_HASH_TABLE_SIZE;
    g_hash_table.count = 0;
    g_hash_table.load_factor_threshold = LOAD_FACTOR_THRESHOLD;
    
    g_hash_table_initialized = true;
    
    printf("[Hash] Hash 表初始化成功（初始大小: %zu）\n", g_hash_table.size);
    
    return true;
}

/**
 * @brief 清理账户 Hash 表
 */
void cleanup_account_hash_table(void)
{
    if (!g_hash_table_initialized) {
        return;
    }
    
    printf("[Hash] 正在清理 Hash 表...\n");
    
    /* 释放所有节点 */
    for (size_t i = 0; i < g_hash_table.size; i++) {
        AccountNode *current = g_hash_table.buckets[i];
        
        while (current != NULL) {
            AccountNode *next = current->next;
            free(current);
            current = next;
        }
    }
    
    /* 释放桶数组 */
    free(g_hash_table.buckets);
    
    g_hash_table.buckets = NULL;
    g_hash_table.size = 0;
    g_hash_table.count = 0;
    g_hash_table_initialized = false;
    
    printf("[Hash] Hash 表清理完成\n");
}

/**
 * @brief 插入账户到 Hash 表
 */
bool hash_insert_account(const ACCOUNT *acc)
{
    if (!g_hash_table_initialized) {
        fprintf(stderr, "错误：Hash 表未初始化\n");
        return false;
    }
    
    /* 检查是否需要扩容 */
    if (calculate_load_factor() >= g_hash_table.load_factor_threshold) {
        if (!resize_hash_table()) {
            return false;
        }
    }
    
    /* 计算桶索引 */
    unsigned long index = hash_function(acc->UUID, g_hash_table.size);
    
    /* 检查是否已存在（避免重复插入） */
    AccountNode *current = g_hash_table.buckets[index];
    while (current != NULL) {
        if (strcmp(current->account.UUID, acc->UUID) == 0) {
            /* 账户已存在，更新数据 */
            current->account = *acc;
            return true;
        }
        current = current->next;
    }
    
    /* 创建新节点 */
    AccountNode *new_node = (AccountNode *)malloc(sizeof(AccountNode));
    if (new_node == NULL) {
        fprintf(stderr, "错误：插入账户时内存分配失败\n");
        return false;
    }
    
    new_node->account = *acc;
    new_node->next = g_hash_table.buckets[index];
    g_hash_table.buckets[index] = new_node;
    
    g_hash_table.count++;
    
    return true;
}

/**
 * @brief 从 Hash 表查找账户
 */
ACCOUNT* hash_find_account(const char *uuid)
{
    if (!g_hash_table_initialized) {
        return NULL;
    }
    
    /* 计算桶索引 */
    unsigned long index = hash_function(uuid, g_hash_table.size);
    
    /* 遍历链表查找 */
    AccountNode *current = g_hash_table.buckets[index];
    while (current != NULL) {
        if (strcmp(current->account.UUID, uuid) == 0) {
            return &current->account;
        }
        current = current->next;
    }
    
    return NULL;
}

/**
 * @brief 更新 Hash 表中的账户
 */
bool hash_update_account(const ACCOUNT *acc)
{
    if (!g_hash_table_initialized) {
        fprintf(stderr, "错误：Hash 表未初始化\n");
        return false;
    }
    
    /* 计算桶索引 */
    unsigned long index = hash_function(acc->UUID, g_hash_table.size);
    
    /* 查找并更新 */
    AccountNode *current = g_hash_table.buckets[index];
    while (current != NULL) {
        if (strcmp(current->account.UUID, acc->UUID) == 0) {
            current->account = *acc;
            return true;
        }
        current = current->next;
    }
    
    /* 账户不存在，插入新账户 */
    return hash_insert_account(acc);
}

/**
 * @brief 从 Hash 表删除账户
 */
bool hash_delete_account(const char *uuid)
{
    if (!g_hash_table_initialized) {
        fprintf(stderr, "错误：Hash 表未初始化\n");
        return false;
    }
    
    /* 计算桶索引 */
    unsigned long index = hash_function(uuid, g_hash_table.size);
    
    /* 查找并删除 */
    AccountNode *current = g_hash_table.buckets[index];
    AccountNode *prev = NULL;
    
    while (current != NULL) {
        if (strcmp(current->account.UUID, uuid) == 0) {
            /* 找到节点，删除 */
            if (prev == NULL) {
                /* 删除头节点 */
                g_hash_table.buckets[index] = current->next;
            } else {
                /* 删除中间或尾节点 */
                prev->next = current->next;
            }
            
            free(current);
            g_hash_table.count--;
            
            return true;
        }
        
        prev = current;
        current = current->next;
    }
    
    return false;  /* 账户不存在 */
}

/* ==================== 系统初始化 ==================== */

/**
 * @brief 初始化账户系统
 */
bool init_account_system(void)
{
    /* 创建Card目录 */
    if (!create_card_directory()) {
        return false;
    }
    
    /* 检查密钥文件是否存在 */
    if (access("system.key", F_OK) != 0) {
        /* 密钥不存在，生成新密钥 */
        if (!generate_system_key()) {
            fprintf(stderr, "错误：无法生成系统密钥\n");
            return false;
        }
    }
    
    /* 加载密钥 */
    if (!load_system_key()) {
        fprintf(stderr, "错误：无法加载系统密钥\n");
        return false;
    }
    
    /* 初始化 Hash 表 */
    if (!init_account_hash_table()) {
        fprintf(stderr, "错误：无法初始化账户 Hash 表\n");
        return false;
    }
    
    /* 加载所有本地账户到 Hash 表 */
    printf("[Hash] 正在加载本地账户到 Hash 表...\n");
    int loaded_count = 0;
    
#ifdef _WIN32
    /* Windows平台 */
    struct _finddata_t fileinfo;
    intptr_t handle = _findfirst("Card/*.card", &fileinfo);
    
    if (handle != -1) {
        do {
            /* 提取UUID */
            char uuid[37];
            strncpy(uuid, fileinfo.name, 36);
            uuid[36] = '\0';
            
            /* 移除.card后缀 */
            char *dot = strrchr(uuid, '.');
            if (dot) *dot = '\0';
            
            /* 加载账户并插入 Hash 表 */
            ACCOUNT acc;
            if (load_account(uuid, &acc)) {
                if (hash_insert_account(&acc)) {
                    loaded_count++;
                }
            }
        } while (_findnext(handle, &fileinfo) == 0);
        
        _findclose(handle);
    }
#else
    /* Linux平台 */
    DIR *dir = opendir("Card");
    if (dir != NULL) {
        struct dirent *entry;
        while ((entry = readdir(dir)) != NULL) {
            /* 只处理.card文件 */
            if (strstr(entry->d_name, ".card") == NULL) {
                continue;
            }
            
            /* 提取UUID */
            char uuid[37];
            strncpy(uuid, entry->d_name, 36);
            uuid[36] = '\0';
            
            /* 加载账户并插入 Hash 表 */
            ACCOUNT acc;
            if (load_account(uuid, &acc)) {
                if (hash_insert_account(&acc)) {
                    loaded_count++;
                }
            }
        }
        
        closedir(dir);
    }
#endif
    
    printf("[Hash] 已加载 %d 个账户到 Hash 表\n", loaded_count);
    printf("[Hash] 当前负载因子: %.2f\n", calculate_load_factor());
    
    return true;
}

/**
 * @brief 清理账户系统资源
 */
void cleanup_account_system(void)
{
    cleanup_account_hash_table();
}

/* ==================== UUID生成（跨平台） ==================== */

/**
 * @brief 生成UUID v4字符串
 */
void generate_uuid_string(char *uuid_str)
{
#ifdef _WIN32
    /* Windows平台 */
    UUID uuid;
    UuidCreate(&uuid);
    
    unsigned char *str;
    UuidToStringA(&uuid, &str);
    strncpy(uuid_str, (char*)str, 36);
    uuid_str[36] = '\0';
    RpcStringFreeA(&str);
    
    /* 转换为小写 */
    for (int i = 0; uuid_str[i]; i++) {
        if (uuid_str[i] >= 'A' && uuid_str[i] <= 'F') {
            uuid_str[i] = uuid_str[i] - 'A' + 'a';
        }
    }
#else
    /* Linux平台 */
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_unparse_lower(uuid, uuid_str);
#endif
}

/* ==================== 密钥管理 ==================== */

/**
 * @brief 生成并保存系统密钥
 */
bool generate_system_key(void)
{
#ifdef _WIN32
    /* Windows: 使用UUID API生成随机数 */
    UUID uuid;
    UuidCreate(&uuid);
    memcpy(g_system_key, &uuid, 16);
#else
    /* Linux: 使用uuid_generate_random */
    uuid_t uuid;
    uuid_generate_random(uuid);
    memcpy(g_system_key, uuid, 16);
#endif
    
    /* 保存到文件 */
    FILE *file = fopen("system.key", "wb");
    if (file == NULL) {
        perror("错误：无法创建密钥文件");
        return false;
    }
    
    if (fwrite(g_system_key, 1, 16, file) != 16) {
        fprintf(stderr, "错误：密钥写入失败\n");
        fclose(file);
        return false;
    }
    
    fclose(file);
    g_key_loaded = true;
    return true;
}

/**
 * @brief 加载系统密钥
 */
bool load_system_key(void)
{
    FILE *file = fopen("system.key", "rb");
    if (file == NULL) {
        perror("错误：无法打开密钥文件");
        return false;
    }
    
    if (fread(g_system_key, 1, 16, file) != 16) {
        fprintf(stderr, "错误：密钥读取失败\n");
        fclose(file);
        return false;
    }
    
    fclose(file);
    g_key_loaded = true;
    return true;
}

/* ==================== 加密解密 ==================== */

/**
 * @brief XOR加密/解密
 * @note 解密规则:异或后异或就是原数据 ->具体参照xor规则(不同才为1 -> 一个位为1就反转对应数据的对应位  0就保持原样)
 */
void xor_encrypt_decrypt(unsigned char *data, size_t len)
{
    if (!g_key_loaded) {
        fprintf(stderr, "警告：密钥未加载\n");
        return;
    }
    
    for (size_t i = 0; i < len; i++) {
        data[i] ^= g_system_key[i % 16];
    }
}

/* ==================== 文件操作 ==================== */

/**
 * @brief 创建Card文件夹
 */
bool create_card_directory(void)
{
    if (access("Card", F_OK) == 0) {
        return true;  /* 目录已存在 */
    }
    
    if (mkdir("Card", 0755) != 0) {
        perror("错误：无法创建Card目录");
        return false;
    }
    
    return true;
}

/**
 * @brief 保存账户到文件
 */
bool save_account(const ACCOUNT *acc)
{
    char filename[50];// 32位UUID 
    snprintf(filename, sizeof(filename), "Card/%s.card", acc->UUID);
    
    /* 准备加密数据 */
    unsigned char buffer[sizeof(LLUINT) * 2];  /* PASSWORD + BALANCE */
    memcpy(buffer, &acc->PASSWORD, sizeof(LLUINT));
    memcpy(buffer + sizeof(LLUINT), &acc->BALANCE, sizeof(LLUINT));
    
    /* 加密 */
    xor_encrypt_decrypt(buffer, sizeof(buffer));
    
    /* 写入文件 */
    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("错误：无法创建账户文件");
        return false;
    }
    
    /* 写入UUID（明文）方便用户查看 */
    fprintf(file, "%s\n", acc->UUID);
    
    /* 写入加密数据 PASSWORD BALANCE*/
    fwrite(buffer, 1, sizeof(buffer), file);
    
    fclose(file);
    
    /* 同步更新 Hash 表 */
    hash_update_account(acc);
    
    return true;
}

/**
 * @brief 从文件加载账户
 */
bool load_account(const char *uuid, ACCOUNT *acc)
{
    /* 首先尝试从 Hash 表查找 */
    ACCOUNT *cached_acc = hash_find_account(uuid);
    if (cached_acc != NULL) {
        *acc = *cached_acc;
        return true;
    }
    
    /* Hash 表未命中，从文件读取 */
    char filename[50];
    snprintf(filename, sizeof(filename), "Card/%s.card", uuid);
    
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        return false;
    }
    
    /* 读取UUID
     * 需要注意linux底层文件读取时文件指针指针移动规则->读取多少字节移动多少字节 问题引导:下一次是否会从最后一个字节位置开始读取 
    */
    if (fscanf(file, "%36s\n", acc->UUID) != 1) {
        fclose(file);
        return false;
    }
    
    /* 读取加密数据 */
    unsigned char buffer[sizeof(LLUINT) * 2];
    if (fread(buffer, 1, sizeof(buffer), file) != sizeof(buffer)) {
        fclose(file);
        return false;
    }
    
    fclose(file);
    
    /* 解密 */
    xor_encrypt_decrypt(buffer, sizeof(buffer));
    
    /* 提取数据 */
    memcpy(&acc->PASSWORD, buffer, sizeof(LLUINT));
    memcpy(&acc->BALANCE, buffer + sizeof(LLUINT), sizeof(LLUINT));
    
    /* 加载成功后，插入 Hash 表以加速后续查找 */
    hash_insert_account(acc);
    
    return true;
}

/**
 * @brief 列出所有账户
 */
int list_all_accounts(void)
{
    int count = 0;
    
   PRINTF_G("\n========== 账户列表 ==========\n");
    
#ifdef _WIN32
    /* Windows平台 */
    struct _finddata_t fileinfo;
    intptr_t handle = _findfirst("Card/*.card", &fileinfo);
    
    if (handle == -1) {
       PRINTF_G("暂无账户\n");
        return 0;
    }
    
    do {
        /* 提取UUID */
        char uuid[37];
        strncpy(uuid, fileinfo.name, 36);
        uuid[36] = '\0';
        
        /* 移除.card后缀 */
        char *dot = strrchr(uuid, '.');
        if (dot) *dot = '\0';
        
        /* 加载账户 */
        ACCOUNT acc;
        if (load_account(uuid, &acc)) {
            count++;
            PRINTF_G("%d. UUID: %s\n", count, acc.UUID);
            PRINTF_G("   余额: %.2f 元\n", acc.BALANCE / 100.0); //注意BALANCE单位是分
        }
    } while (_findnext(handle, &fileinfo) == 0);
    
    _findclose(handle);
#else
    /* Linux平台 */
    DIR *dir = opendir("Card");
    if (dir == NULL) {
        PRINTF_G("暂无账户\n");
        return 0;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL) {
        /* 只处理.card文件 */
        if (strstr(entry->d_name, ".card") == NULL) {
            continue;
        }
        
        /* 提取UUID */
        char uuid[37];
        strncpy(uuid, entry->d_name, 36);
        uuid[36] = '\0';
        
        /* 加载账户 */
        ACCOUNT acc;
        if (load_account(uuid, &acc)) {
            count++;
            PRINTF_G("%d. UUID: %s\n", count, acc.UUID);
            PRINTF_G("   余额: %.2f 元\n", acc.BALANCE / 100.0);
        }
    }
    
    closedir(dir);
#endif
    
    PRINTF_G("==============================\n");
    PRINTF_G("共 %d 个账户\n\n", count);
    
    return count;
}

/**
 * @brief 获取所有本地账户的UUID列表
 */
int get_all_account_uuids(char uuids[][37], int max_count)
{
    int count = 0;
    
#ifdef _WIN32
    /* Windows平台 */
    struct _finddata_t fileinfo;
    intptr_t handle = _findfirst("Card/*.card", &fileinfo);
    
    if (handle == -1) {
        return 0;
    }
    
    do {
        if (count >= max_count) break;
        
        /* 提取UUID */
        strncpy(uuids[count], fileinfo.name, 36);
        uuids[count][36] = '\0';
        
        /* 移除.card后缀 */
        char *dot = strrchr(uuids[count], '.');
        if (dot) *dot = '\0';
        
        count++;
    } while (_findnext(handle, &fileinfo) == 0);
    
    _findclose(handle);
#else
    /* Linux平台 */
    DIR *dir = opendir("Card");
    if (dir == NULL) {
        return 0;
    }
    
    struct dirent *entry;
    while ((entry = readdir(dir)) != NULL && count < max_count) {
        /* 只处理.card文件 */
        if (strstr(entry->d_name, ".card") == NULL) {
            continue;
        }
        
        /* 提取UUID */
        strncpy(uuids[count], entry->d_name, 36);
        uuids[count][36] = '\0';
        count++;
    }
    
    closedir(dir);
#endif
    
    return count;
}

/**
 * @brief 同步所有本地账户到服务器
 */
int sync_all_accounts_to_server(void)
{
    /* 检查是否处于服务器模式 */
    if (get_run_mode() != MODE_SERVER) {
        printf("[推送] 未连接到服务器，跳过同步\n");
        return 0;
    }
    
    /* 获取所有本地账户UUID */
    char uuids[100][37];  /* 最多100个账户,虽然需求要求支持50个但是实际可以非常多 */
    int total_count = get_all_account_uuids(uuids, 100);
    
    if (total_count == 0) {
        printf("[推送] 本地没有账户需要同步\n");
        return 0;
    }
    
    printf("[推送] 发现 %d 个本地账户，开始推送到服务器...\n", total_count);
    
    int success_count = 0;
    int fail_count = 0;
    
    /* 逐个同步账户 */
    for (int i = 0; i < total_count; i++) {
        ACCOUNT acc;
        if (!load_account(uuids[i], &acc)) {
            fprintf(stderr, "[推送] 无法加载账户 %s\n", uuids[i]);
            fail_count++;
            continue;
        }
        
        //printf("[推送] 正在推送账户 %s (余额: %.2f元)...\n", 
        //       acc.UUID, acc.BALANCE / 100.0);
        
        /* 调用同步API */
        if (api_sync_account(&acc)) {
            //printf("[推送] ✓ 账户 %s 推送成功\n", acc.UUID);
            success_count++;
        } else {
            //fprintf(stderr, "[推送] ✗ 账户 %s 推送失败\n", acc.UUID);
            fail_count++;
        }
    }
    
    printf("[推送] 推送完成: 成功 %d 个, 失败 %d 个\n", success_count, fail_count);
    return success_count;
}

/**
 * @brief 从服务器拉取账户并保存到本地
 */
int pull_accounts_from_server(void)
{
    /* 检查是否处于服务器模式 */
    if (get_run_mode() != MODE_SERVER) {
        printf("[拉取] 未连接到服务器，跳过拉取\n");
        return 0;
    }
    
    /* 从服务器获取账户 */
    ACCOUNT server_accounts[100];  /* 最多100个账户 */
    int count = api_fetch_all_accounts(server_accounts, 100);
    
    if (count < 0) {
        fprintf(stderr, "[拉取] 从服务器获取账户失败\n");
        return 0;
    }
    
    if (count == 0) {
        printf("[拉取] 服务器没有账户数据\n");
        return 0;
    }
    
    printf("[拉取] 开始保存服务器账户到本地...\n");
    
    int success_count = 0;
    int fail_count = 0;
    
    /* 保存每个账户到本地 */
    for (int i = 0; i < count; i++) {
        ACCOUNT *acc = &server_accounts[i];
        
        /* 检查本地是否已存在该账户 */
        ACCOUNT local_acc;
        bool exists = load_account(acc->UUID, &local_acc);
        
        if (exists) {
            /* 本地已存在，比较并更新余额 */
            if (local_acc.BALANCE != acc->BALANCE) {
                //printf("[拉取] 更新账户 %s 余额: %.2f元 -> %.2f元\n",
                //       acc->UUID, local_acc.BALANCE / 100.0, acc->BALANCE / 100.0);
                
                /* 保留本地密码 */
                acc->PASSWORD = local_acc.PASSWORD;
                
                if (save_account(acc)) {
                    success_count++;
                } else {
                    //fprintf(stderr, "[拉取] ✗ 更新账户 %s 失败\n", acc->UUID);
                    fail_count++;
                }
            } else {
                //printf("[拉取] 账户 %s 无需更新\n", acc->UUID);
                success_count++;
            }
        } else {
            /* 本地不存在，新建账户（密码设为0） */
            //printf("[拉取] 新增账户 %s (余额: %.2f元)\n",
            //       acc->UUID, acc->BALANCE / 100.0);
            
            acc->PASSWORD = 0;  /* 新账户默认密码为0 */
            
            if (save_account(acc)) {
                success_count++;
                //printf("[拉取] ✓ 账户 %s 保存成功（注意：密码已设为0000000）\n", acc->UUID);
            } else {
                //fprintf(stderr, "[拉取] ✗ 保存账户 %s 失败\n", acc->UUID);
                fail_count++;
            }
        }
    }
    
    printf("[拉取] 拉取完成: 成功 %d 个, 失败 %d 个\n", success_count, fail_count);
    return success_count;
}

/**
 * @brief 删除账户文件
 */
bool delete_account_file(const char *uuid)
{
    char filename[50];
    snprintf(filename, sizeof(filename), "Card/%s.card", uuid);
    
    if (remove(filename) != 0) {
        perror("错误：无法删除账户文件");
        return false;
    }
    
    /* 同步从 Hash 表删除 */
    hash_delete_account(uuid);
    
    return true;
}

/* ==================== 业务功能 ==================== */

/**
 * @brief 创建账户
 */
bool create_account(LLUINT password)
{
    /* 验证密码 */
    if (password < 1000000 || password > 9999999) {
        fprintf(stderr, "错误：密码必须是7位数字（1000000-9999999）\n");
        return false;
    }
    
    /* 创建账户 */
    ACCOUNT new_account;
    generate_uuid_string(new_account.UUID);
    new_account.PASSWORD = password;
    new_account.BALANCE = 0;
    
    /* 保存到本地 */
    if (!save_account(&new_account)) {
        return false;
    }
    
    /* 服务器模式下同步到服务器 */
    if (get_run_mode() == MODE_SERVER) {
        if (!api_create_account(&new_account)) {
            fprintf(stderr, "警告：服务器同步失败，账户仅保存到本地\n");
        } else {
            PRINTF_G("账户已同步到服务器\n");
        }
    }
    
    PRINTF_G("\n账户创建成功！\n");
    PRINTF_G("账户UUID: %s\n", new_account.UUID);
    PRINTF_G("请妥善保管您的UUID和密码\n\n");
    
    return true;
}

/**
 * @brief 存款
 */
bool deposit(void)
{
    /* 列出所有账户 */
    if (list_all_accounts() == 0) {
        return false;
    }
    
    /* 输入UUID */
    char uuid[37];
    PRINTF_G("请输入账户UUID: ");
    if (scanf("%36s", uuid) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    /* 加载账户 */
    ACCOUNT acc;
    if (!load_account(uuid, &acc)) {
        fprintf(stderr, "错误：账户不存在\n");
        return false;
    }
    
    /* 验证密码 */
    LLUINT password;
    PRINTF_G("请输入密码: ");
    if (scanf("%llu", &password) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    if (acc.PASSWORD != password) {
        fprintf(stderr, "错误：密码错误\n");
        return false;
    }
    
    /* 输入存款金额 */
    double amount;
    printf("请输入存款金额（元）: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0) {
        fprintf(stderr, "错误：金额无效\n");
        return false;
    }
    
    /* 更新余额 */
    LLUINT amount_cents = (LLUINT)(amount * 100);
    acc.BALANCE += amount_cents;
    
    /* 保存到本地 */
    if (!save_account(&acc)) {
        return false;
    }
    
    /* 服务器模式下同步到服务器 */
    if (get_run_mode() == MODE_SERVER) {
        if (!api_deposit(uuid, amount_cents)) {
            fprintf(stderr, "警告：服务器同步失败，仅保存到本地\n");
        } else {
           PRINTF_G("交易已同步到服务器\n");
        }
    }
    
   PRINTF_G("\n存款成功！\n");
   PRINTF_G("当前余额: %.2f 元\n\n", acc.BALANCE / 100.0);
    
    return true;
}

/**
 * @brief 取款
 */
bool withdraw(void)
{
    /* 列出所有账户 */
    if (list_all_accounts() == 0) {
        return false;
    }
    
    /* 输入UUID */
    char uuid[37];
   PRINTF_G("请输入账户UUID: ");
    if (scanf("%36s", uuid) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    /* 加载账户 */
    ACCOUNT acc;
    if (!load_account(uuid, &acc)) {
        fprintf(stderr, "错误：账户不存在\n");
        return false;
    }
    
    /* 验证密码 */
    LLUINT password;
   PRINTF_G("请输入密码: ");
    if (scanf("%llu", &password) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    if (acc.PASSWORD != password) {
        fprintf(stderr, "错误：密码错误\n");
        return false;
    }
    
    /* 显示余额 */
   PRINTF_G("当前余额: %.2f 元\n", acc.BALANCE / 100.0);
    
    /* 输入取款金额 */
    double amount;
   PRINTF_G("请输入取款金额（元）: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0) {
        fprintf(stderr, "错误：金额无效\n");
        return false;
    }
    
    LLUINT amount_cents = (LLUINT)(amount * 100);
    
    /* 检查余额 */
    if (acc.BALANCE < amount_cents) {
        fprintf(stderr, "错误：余额不足\n");
        return false;
    }
    
    /* 更新余额 */
    acc.BALANCE -= amount_cents;
    
    /* 保存到本地 */
    if (!save_account(&acc)) {
        return false;
    }
    
    /* 服务器模式下同步到服务器 */
    if (get_run_mode() == MODE_SERVER) {
        if (!api_withdraw(uuid, amount_cents)) {
            fprintf(stderr, "警告：服务器同步失败，仅保存到本地\n");
        } else {
           PRINTF_G("交易已同步到服务器\n");
        }
    }
    
   PRINTF_G("\n取款成功！\n");
   PRINTF_G("当前余额: %.2f 元\n\n", acc.BALANCE / 100.0);
    
    return true;
}

/**
 * @brief 转账
 */
bool transfer(void)
{
    

    /* 列出所有账户 */
    if (list_all_accounts() == 0) {
        return false;
    }
    
    /* 输入转出账户UUID */
    char uuid_from[37];
   PRINTF_G("请输入转出账户UUID: ");
    if (scanf("%36s", uuid_from) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    /* 加载转出账户 */
    ACCOUNT acc_from;
    if (!load_account(uuid_from, &acc_from)) {
        fprintf(stderr, "错误：转出账户不存在\n");
        return false;
    }
    
    /* 验证密码 */
    LLUINT password;
   PRINTF_G("请输入密码: ");
    if (scanf("%llu", &password) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    if (acc_from.PASSWORD != password) {
        fprintf(stderr, "错误：密码错误\n");
        return false;
    }
    
    /* 输入转入账户UUID */
    char uuid_to[37];
   PRINTF_G("请输入转入账户UUID: ");
    if (scanf("%36s", uuid_to) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    /* 检查是否转给自己 */
    if (strcmp(uuid_from, uuid_to) == 0) {
        fprintf(stderr, "错误：不能转账给自己\n");
        return false;
    }
    
    /* 加载转入账户 */
    ACCOUNT acc_to;
    if (!load_account(uuid_to, &acc_to)) {
        fprintf(stderr, "错误：转入账户不存在\n");
        return false;
    }
    
    /* 显示余额 */
   PRINTF_G("您的当前余额: %.2f 元\n", acc_from.BALANCE / 100.0);
    
    /* 输入转账金额 */
    double amount;
   PRINTF_G("请输入转账金额（元）: ");
    if (scanf("%lf", &amount) != 1 || amount <= 0) {
        fprintf(stderr, "错误：金额无效\n");
        return false;
    }
    
    LLUINT amount_cents = (LLUINT)(amount * 100);
    
    /* 检查余额 */
    if (acc_from.BALANCE < amount_cents) {
        fprintf(stderr, "错误：余额不足\n");
        return false;
    }
    
    /* 更新余额 */
    acc_from.BALANCE -= amount_cents;
    acc_to.BALANCE += amount_cents;
    
    /* 保存两个账户到本地 */
    if (!save_account(&acc_from) || !save_account(&acc_to)) {
        fprintf(stderr, "错误：保存账户失败\n");
        return false;
    }
    
    /* 服务器模式下同步到服务器 */
    if (get_run_mode() == MODE_SERVER) {
        if (!api_transfer(uuid_from, uuid_to, amount_cents)) {
            fprintf(stderr, "警告：服务器同步失败，仅保存到本地\n");
        } else {
           PRINTF_G("交易已同步到服务器\n");
        }
    }
    
   PRINTF_G("\n转账成功！\n");
   PRINTF_G("您的当前余额: %.2f 元\n\n", acc_from.BALANCE / 100.0);
    
    return true;
}

/**
 * @brief 销户
 */
bool delete_account(void)
{
    /* 列出所有账户 */
    if (list_all_accounts() == 0) {
        return false;
    }
    
    /* 输入UUID */
    char uuid[37];
   PRINTF_G("请输入要注销的账户UUID: ");
    if (scanf("%36s", uuid) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    /* 加载账户 */
    ACCOUNT acc;
    if (!load_account(uuid, &acc)) {
        fprintf(stderr, "错误：账户不存在\n");
        return false;
    }
    
    /* 验证密码 */
    LLUINT password;
   PRINTF_G("请输入密码: ");
    if (scanf("%llu", &password) != 1) {
        fprintf(stderr, "输入错误\n");
        return false;
    }
    
    if (acc.PASSWORD != password) {
        fprintf(stderr, "错误：密码错误\n");
        return false;
    }
    
    /* 显示账户信息 */
   PRINTF_G("\n账户信息：\n");
   PRINTF_G("UUID: %s\n", acc.UUID);
   PRINTF_G("余额: %.2f 元\n", acc.BALANCE / 100.0);

   /* 账户有余额不能注销 */
   if (acc.BALANCE > 0)
   {
       PRINTF_G("错误：账户有余额，不能注销\n");
       return false;
   }
   
    
    /* 二次确认 */
    char confirm[10];
   PRINTF_G("\n警告：销户后数据将永久删除，且余额将清零！\n");
   PRINTF_G("确认删除？(输入 yes 确认): ");
    scanf("%9s", confirm);
    
    if (strcmp(confirm, "yes") != 0) {
       PRINTF_G("已取消操作\n");
        return false;
    }
    
    /* 删除本地文件 */
    if (!delete_account_file(uuid)) {
        return false;
    }
    
    /* 服务器模式下同步到服务器 */
    if (get_run_mode() == MODE_SERVER) {
        if (!api_delete_account(uuid)) {
            fprintf(stderr, "警告：服务器同步失败，仅删除本地账户\n");
        } else {
           PRINTF_G("账户已从服务器删除\n");
        }
    }
    
   PRINTF_G("\n销户成功！\n\n");
    
    return true;
}

/**
 * @brief 测试函数，随机生成N个账户进行压力测试
 * @note  测试运用Hash表后的响应速度
 */
bool generate_test_account(){
    int count=0;
    PRINTF_G("请输入测试账户的数量:");
    scanf("%d",&count);
    
    /*合法性检查*/
    if (count<=0 || count > 1000000)
    {
        /* 错误！ */
        return false;
    }
    

    /* 循环生成测试账户 */
    for (int i = 0; i < count; i++)
    {
        LLUINT password = 1234567;
        create_account(password);
    }

    return true;
}