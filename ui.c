/**
 * @file ui.c
 * @brief 用户界面模块实现
 * @author BAMSYSTEM团队
 * @date 2025-11-08
 * @version 1.0
 */

#include <lib/ui.h>
#include <lib/account.h>

#ifdef _WIN32
 #include <conio.h>
#else
 #include <termios.h>
 #include <unistd.h>
#endif

/* ==================== 静态常量定义 ==================== */

/** @brief 业务菜单选项数组 */
static const char *BUSINESS_MENU[] = {
    "1.创建账户         \n",
    "2.账户存款         \n",
    "3.账户取款         \n",
    "4.账户转账         \n",
    "5.注销账户         \n",
    "6.生成测试账户   \n",
    "7.账户列表排序设置 \n",
    "0.退出系统         \n"
};

/** @brief 菜单项数量 */
static const int MENU_COUNT = sizeof(BUSINESS_MENU) / sizeof(BUSINESS_MENU[0]);

/* ==================== 函数实现 ==================== */

#ifndef _WIN32
static struct termios g_old_termios;
static bool g_raw_mode_enabled = false;
#endif

void ui_set_raw_mode(bool enable)
{
#ifdef _WIN32
    (void)enable;
#else
    if (enable && !g_raw_mode_enabled) {
        struct termios raw;
        if (tcgetattr(STDIN_FILENO, &g_old_termios) != 0) {
            return;
        }
        raw = g_old_termios;
        raw.c_lflag &= (tcflag_t)~(ECHO | ICANON);
        raw.c_cc[VMIN] = 1;
        raw.c_cc[VTIME] = 0;
        if (tcsetattr(STDIN_FILENO, TCSANOW, &raw) == 0) {
            g_raw_mode_enabled = true;
        }
    } else if (!enable && g_raw_mode_enabled) {
        tcsetattr(STDIN_FILENO, TCSANOW, &g_old_termios);
        g_raw_mode_enabled = false;
    }
#endif
}

UiKey ui_read_key(void)
{
#ifdef _WIN32
    int c = _getch();
    if (c == 0 || c == 224) {
        int c2 = _getch();
        if (c2 == 72) return UI_KEY_UP;
        if (c2 == 80) return UI_KEY_DOWN;
        return UI_KEY_NONE;
    }
    if (c == 13) return UI_KEY_ENTER;
    if (c == 27) return UI_KEY_ESC;
    return UI_KEY_NONE;
#else
    int c = getchar();
    if (c == 27) {
        int c2 = getchar();
        if (c2 == '[') {
            int c3 = getchar();
            if (c3 == 'A') return UI_KEY_UP;
            if (c3 == 'B') return UI_KEY_DOWN;
        }
        return UI_KEY_ESC;
    }
    if (c == '\n' || c == '\r') return UI_KEY_ENTER;
    return UI_KEY_NONE;
#endif
}

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

static void output_business_with_selection(int selected)
{
    for (int i = 0; i < MENU_COUNT; i++) {
        if (i == selected) {
            printf("\033[7m");
            PRINTF_G("%s", BUSINESS_MENU[i]);
            printf("\033[0m");
        } else {
            PRINTF_G("%s", BUSINESS_MENU[i]);
        }
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
    ui_set_raw_mode(true);

    int selected = 0;

    while (1)
    {
        clear_screen();
        PRINTF_G("--------------------BAMSYSTEM-银行账户管理系统--------------------\n");
        PRINTF_G("-请选择你的业务-\n");

        output_business_with_selection(selected);

        UiKey key = ui_read_key();
        if (key == UI_KEY_UP) {
            selected = (selected - 1 + MENU_COUNT) % MENU_COUNT;
            continue;
        }
        if (key == UI_KEY_DOWN) {
            selected = (selected + 1) % MENU_COUNT;
            continue;
        }
        if (key != UI_KEY_ENTER) {
            continue;
        }

        int opcode = -1;
        if (selected == MENU_COUNT - 1) {
            opcode = 0;
        } else {
            opcode = selected + 1;
        }

        ui_set_raw_mode(false);
        
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

        case 7:
            clear_screen();
            if (get_account_sort_mode() == ACCOUNT_SORT_BALANCE) {
                set_account_sort_mode(ACCOUNT_SORT_UUID_TIME);
                PRINTF_G("已切换账户排序：按UUID时间\n");
            } else {
                set_account_sort_mode(ACCOUNT_SORT_BALANCE);
                PRINTF_G("已切换账户排序：按余额\n");
            }
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

        ui_set_raw_mode(true);
    }

    ui_set_raw_mode(false);
    return 0;
}
