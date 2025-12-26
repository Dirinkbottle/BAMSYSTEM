/**
 * @file ui.c
 * @brief 用户界面模块实现
 * @author BAMSYSTEM团队
 * @date 2025-11-08
 * @version 1.0
 */

#include <lib/ui.h>
#include <lib/account.h>

/* ==================== 静态常量定义 ==================== */

/** @brief 业务菜单选项数组 */
static const char *BUSINESS_MENU[] = {
    "1.创建账户         \n",
    "2.账户存款         \n",
    "3.账户取款         \n",
    "4.账户转账         \n",
    "5.注销账户         \n",
    "6.生成测试账户   \n",
    "0.退出系统         \n"
};

/** @brief 菜单项数量 */
static const int MENU_COUNT = sizeof(BUSINESS_MENU) / sizeof(BUSINESS_MENU[0]);

/* ==================== 函数实现 ==================== */

/**
 * @brief 清除屏幕内容
 */
void clear_screen(void)
{
#ifdef _WIN32
    system("cls");
#else
    printf(ANSI_SCREEN);
    fflush(stdout);
#endif
}

/**
 * @brief 输出业务菜单选项
 */
void output_business(void)
{
    for (int i = 0; i < MENU_COUNT; i++)
    {
        PRINTF_G("%s", BUSINESS_MENU[i]);
    }
}

/**
 * @brief 消耗stdin
 */
void consume_stdin(void)
{
    char c;
    while ((c = getchar()) != '\n' && c != EOF);
}

/**
 * @brief UI主循环函数
 * @return 返回0表示正常退出
 */
int ui_loop(void)
{
    while (1)
    {
        clear_screen();
        PRINTF_G("--------------------BAMSYSTEM-银行账户管理系统--------------------\n");
        
        int opcode = -1;
        PRINTF_G("-请选择你的业务-\n");
        output_business();
        
        /* 读取用户输入 */
        if (!scanf("%d", &opcode))
        {
            while (getchar() != '\n');
            continue;
        }
        
        /* 执行相应操作 */
        switch (opcode)
        {
        case 0:
            /* 退出系统 */
            clear_screen();
            PRINTF_G("%s", "感谢你的使用，再见！\n");
            return 0;
            
        case 1:
            /* 创建账户 */
            clear_screen();
            PRINTF_G("请输入你的7位密码(数字组合): ");
            LLUINT password = 0;
            if (scanf("%llu", &password) == 1) {//stdin剩余'\n' 
                create_account(password);
            }
            PRINTF_G("\n按回车键继续...");
            consume_stdin();
            getchar(); 
            break;
            
        case 2:
            /* 存款 */
            clear_screen();
            deposit();
            PRINTF_G("\n按回车键继续...");
            consume_stdin();
            getchar();
            break;
            
        case 3:
            /* 取款 */
            clear_screen();
            withdraw();
            PRINTF_G("\n按回车键继续...");
            consume_stdin();
            getchar();
            break;
            
        case 4:
            /* 转账 */
            clear_screen();
            transfer();
            PRINTF_G("\n按回车键继续...");
            consume_stdin();
            getchar();
            break;
            
        case 5:
            /* 销户 */
            clear_screen();
            delete_account();
            PRINTF_G("\n按回车键继续...");
            consume_stdin();
            getchar();
            break;

        case 6:
            /* 生成测试账户 */
            generate_test_account();
            PRINTF_G("\n按回车键继续...");
            consume_stdin();
            getchar();
            break;
            
        default:
            /* 无效选项 */
            PRINTF_G("无效的选项，请重新选择！\n");
            #ifdef _WIN32
                system("timeout /t 2 >nul");
            #else
                system("sleep 2");
            #endif
            continue;
        }
    }
    
    return 0;
}
