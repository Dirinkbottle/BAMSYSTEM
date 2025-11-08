/**
 * @file main.c
 * @brief BAMSYSTEM银行账户管理系统主程序
 * @author BAMSYSTEM团队
 * @date 2025-11-08
 * @version 1.0
 */

#include <lib/ui.h>
#include <lib/account.h>
#include <lib/platform.h>
#include <lib/server_api.h>
#include <stdio.h>
#include <unistd.h>

/**
 * @brief 主函数
 * @return 程序退出代码
 */
int main(void)
{
    /* 初始化平台环境（Windows设置UTF-8编码） */
    if (init_platform() != 0) {
        fprintf(stderr, "警告：平台初始化失败，可能出现中文乱码\n");
    }
    
    /* 初始化账户系统 */
    if (!init_account_system()) {
        fprintf(stderr, "系统初始化失败！\n");
        return 1;
    }
    
    /* 初始化服务器API */
    printf("正在初始化服务器连接...\n");
    if (init_server_api()) {
        /* 检测服务器可用性 */
        printf("正在检测服务器状态...\n");
        RunMode mode = check_server_availability();
        
        if (mode == MODE_SERVER) {
            printf("✓ 运行模式: 联网版本（服务器已连接）\n");
            printf("\n========== 开始双向同步 ==========\n");
            
            /* 第一步：推送本地账户到服务器 */
            int pushed = sync_all_accounts_to_server();
            
            /* 第二步：从服务器拉取账户到本地 */
            int pulled = pull_accounts_from_server();
            
            printf("========== 同步完成 ==========\n");
            printf("推送: %d 个账户 | 拉取: %d 个账户\n\n", pushed, pulled);
        } else {
            printf("✓ 运行模式: 本地版本（服务器不可用或已禁用）\n");
        }
    } else {
        printf("✓ 运行模式: 本地版本（服务器API初始化失败）\n");
        set_run_mode(MODE_LOCAL);
    }
    
    printf("\n按回车键继续...");
    getchar();
    
    /* 进入UI主循环 */
    ui_loop();
    
    /* 清理服务器API资源 */
    cleanup_server_api();
    
    return 0;
}
