/**
 * @file platform.c
 * @brief 平台相关功能实现
 * @author BAMSYSTEM团队
 * @date 2025-11-08
 */

#include <lib/platform.h>
#include <stdio.h>

#ifdef _WIN32
    /* Windows平台必须先包含winsock2.h再包含windows.h */
    #include <winsock2.h>
    #include <windows.h>
#endif

/**
 * @brief 初始化平台环境
 * 
 * Windows平台：设置控制台输入输出为UTF-8编码 解决中文在一些windows版本的cmd（默认GBK）是设置为UTF-8解决乱码问题
 * Linux平台：无需特殊处理
 * @note windows控制台输入输出编码相关函数百度可查
 */
int init_platform(void)
{
#ifdef _WIN32
    /* Windows平台：设置控制台代码页为UTF-8 */
    
    /* 设置控制台输出代码页为UTF-8 (65001) */
    if (!SetConsoleOutputCP(65001)) {
        fprintf(stderr, "警告：无法设置控制台输出编码为UTF-8\n");
        return -1;
    }
    
    /* 设置控制台输入代码页为UTF-8 (65001) */
    if (!SetConsoleCP(65001)) {
        fprintf(stderr, "警告：无法设置控制台输入编码为UTF-8\n");
        return -1;
    }
    

    
#else
    /* Linux平台：通常默认使用UTF-8，无需特殊处理 输入输出默认为UTF-8*/
#endif
    
    return 0; //成功标志
}
