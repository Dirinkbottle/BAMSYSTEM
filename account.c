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
    
    return true;
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
    return true;
}

/**
 * @brief 从文件加载账户
 */
bool load_account(const char *uuid, ACCOUNT *acc)
{
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
    char uuids[100][37];  /* 最多100个账户 */
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
        
        printf("[推送] 正在推送账户 %s (余额: %.2f元)...\n", 
               acc.UUID, acc.BALANCE / 100.0);
        
        /* 调用同步API */
        if (api_sync_account(&acc)) {
            printf("[推送] ✓ 账户 %s 推送成功\n", acc.UUID);
            success_count++;
        } else {
            fprintf(stderr, "[推送] ✗ 账户 %s 推送失败\n", acc.UUID);
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
                printf("[拉取] 更新账户 %s 余额: %.2f元 -> %.2f元\n",
                       acc->UUID, local_acc.BALANCE / 100.0, acc->BALANCE / 100.0);
                
                /* 保留本地密码 */
                acc->PASSWORD = local_acc.PASSWORD;
                
                if (save_account(acc)) {
                    success_count++;
                } else {
                    fprintf(stderr, "[拉取] ✗ 更新账户 %s 失败\n", acc->UUID);
                    fail_count++;
                }
            } else {
                printf("[拉取] 账户 %s 无需更新\n", acc->UUID);
                success_count++;
            }
        } else {
            /* 本地不存在，新建账户（密码设为0） */
            printf("[拉取] 新增账户 %s (余额: %.2f元)\n",
                   acc->UUID, acc->BALANCE / 100.0);
            
            acc->PASSWORD = 0;  /* 新账户默认密码为0 */
            
            if (save_account(acc)) {
                success_count++;
                printf("[拉取] ✓ 账户 %s 保存成功（注意：密码已设为0000000）\n", acc->UUID);
            } else {
                fprintf(stderr, "[拉取] ✗ 保存账户 %s 失败\n", acc->UUID);
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
