/**
 * @file ui.h
 * @brief 用户界面模块头文件
 * 
 * 本文件定义了银行账户管理系统的用户界面相关函数和宏定义。
 * 提供屏幕清理、菜单显示、主循环等功能的接口。
 * 
 * @author BAMSYSTEM团队
 * @date 2025-11-08
 * @version 1.0
 */

#ifndef UI_H
#define UI_H

/* ==================== 标准库头文件 ==================== */
#include <stdio.h>
#include <stdlib.h>

/* ==================== 宏定义 ==================== */

/**
 * @brief ANSI转义序列：清屏并将光标移到左上角
 * 
 * \033[2J 清除整个屏幕
 * \033[H  将光标移动到左上角(1,1)位置
 */
#define ANSI_SCREEN "\033[2J\033[H"

/**
 * @brief ANSI转义序列：绿色前景色
 */
#define ANSI_COLOR_FRONT_GREEN "\033[3;32m"

/**
 * @brief ANSI转义序列：重置所有颜色和样式
 */
#define ANSI_COLOR_RESET "\033[0m"

/**
 * @brief 打印绿色文本的宏函数
 * 
 * 使用ANSI颜色码在终端输出绿色文本，输出后自动重置颜色。
 * 
 * @param ST 格式化字符串
 * @param ... 可变参数列表
 * 
 * @note ##__VA_ARGS__ 是GNU扩展，允许可变参数为空
 */
#define PRINTF_G(ST, ...) printf(ANSI_COLOR_FRONT_GREEN ST ANSI_COLOR_RESET, ##__VA_ARGS__)

/* ==================== 函数声明 ==================== */

/**
 * @brief 清除屏幕内容
 * 
 * 根据不同的操作系统使用相应的方法清屏：
 * - Windows系统：调用system("cls")
 * - Unix/Linux系统：使用ANSI转义序列
 * 
 * @note 该函数会自动检测操作系统并选择合适的清屏方式
 */
void clear_screen(void);

/**
 * @brief 输出业务菜单选项
 * 
 * 在屏幕上显示所有可用的业务操作菜单，
 * 包括创建账户、存款、取款、转账等选项。
 * 
 * @note 菜单项的具体内容在ui.c中定义
 */
void output_business(void);

/**
 * @brief UI主循环函数
 * 
 * 用户界面的主循环，负责：
 * 1. 显示主菜单
 * 2. 接收用户输入
 * 3. 根据用户选择调用相应的功能模块
 * 4. 处理退出系统的操作
 * 
 * @return int 返回0表示正常退出，非0表示异常退出
 * 
 * @note 此函数在Windows 10及以上系统支持ANSI转义序列
 * @note 该函数会一直循环直到用户选择退出
 */
int ui_loop(void);

#endif /* UI_H */
