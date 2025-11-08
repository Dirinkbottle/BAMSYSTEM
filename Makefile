# BAMSYSTEM Makefile
# 支持 Windows 和 Linux 平台编译
# 支持在 Ubuntu 中使用 MinGW64 交叉编译 Windows 版本

# 默认编译器
CC = gcc

# 编译选项
CFLAGS = -Wall -I.

# 链接选项（初始为空）
LDFLAGS =

# 源文件
SRCS = main.c account.c ui.c platform.c server_api.c

# 目标文件
OBJS = $(SRCS:.c=.o)

# 默认平台检测
ifndef PLATFORM
    ifeq ($(OS),Windows_NT)
        PLATFORM = windows
    else
        PLATFORM = linux
    endif
endif

# 网络功能开关（默认启用）
ifndef ENABLE_NETWORK
    ENABLE_NETWORK = yes
endif

# 平台配置
ifeq ($(PLATFORM),windows)
    # Windows 平台配置
    TARGET = bamsystem.exe
    LIBS = -lrpcrt4 -ladvapi32
    
    # 网络功能配置
    ifeq ($(ENABLE_NETWORK),yes)
        # 定义CURL_STATICLIB使用静态链接的curl
        CFLAGS += -DENABLE_NETWORK -DCURL_STATICLIB
        # curl及其依赖的Windows库（链接顺序：应用库 -> 依赖库 -> 系统库）
        LIBS += -lcurl -lcjson -lpthread -lwldap32 -lbcrypt -lcrypt32 -lws2_32
        # 静态链接标志（网络版本）
        LDFLAGS += -static-libgcc -static-libstdc++ -static
    else
        CFLAGS += -DDISABLE_NETWORK
        LIBS += -lws2_32
    endif
    
    # 确保链接标准 C 库（MinGW）
    CFLAGS += -D__USE_MINGW_ANSI_STDIO=1 -D_WIN32
    
    # 检查是否在 Linux 环境中交叉编译
    ifneq ($(OS),Windows_NT)
        # Ubuntu/Linux 环境下使用 MinGW64 交叉编译
        CC = x86_64-w64-mingw32-gcc
        CROSS_COMPILE = yes
    endif
    
    # Windows 命令
    RM = del /Q
    RMDIR = rmdir /S /Q
    MKDIR = mkdir
else ifeq ($(PLATFORM),linux)
    # Linux 平台配置
    TARGET = bamsystem
    LIBS = -luuid
    CC = gcc
    
    # 网络功能配置
    ifeq ($(ENABLE_NETWORK),yes)
        LIBS += -lcurl -lcjson -lssl -lcrypto
        CFLAGS += -DENABLE_NETWORK
    else
        CFLAGS += -DDISABLE_NETWORK
    endif
    
    # Linux 命令
    RM = rm -f
    RMDIR = rm -rf
    MKDIR = mkdir -p
else
    $(error 未知平台: $(PLATFORM), 请使用 linux 或 windows)
endif

# 根据当前运行环境选择命令（用于 clean 等操作）
ifeq ($(OS),Windows_NT)
    SHELL_RM = del /Q
    SHELL_RMDIR = rmdir /S /Q
else
    SHELL_RM = rm -f
    SHELL_RMDIR = rm -rf
endif

# 默认目标
all: info $(TARGET)
	@echo "编译完成：$(TARGET) (目标平台: $(PLATFORM))"

# 显示编译信息
info:
ifdef CROSS_COMPILE
	@echo "交叉编译模式：在 Linux 环境中编译 Windows 程序"
	@echo "编译器：$(CC)"
else
	@echo "本地编译模式：$(PLATFORM)"
	@echo "编译器：$(CC)"
endif
ifeq ($(ENABLE_NETWORK),yes)
	@echo "网络功能：启用"
else
	@echo "网络功能：禁用 (纯本地模式)"
endif
	@echo ""

# 链接生成可执行文件
$(TARGET): $(OBJS)
	$(CC) $(LDFLAGS) $(OBJS) $(LIBS) -o $(TARGET)

# 编译源文件为目标文件
%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

# 清理编译产物
clean:
	$(SHELL_RM) $(OBJS) 2>/dev/null || true
	$(SHELL_RM) bamsystem bamsystem.exe 2>/dev/null || true
	@echo "清理完成"

# 清理所有文件（包括生成的数据）
cleanall: clean
	@$(SHELL_RMDIR) Card 2>/dev/null || true
	@$(SHELL_RM) system.key 2>/dev/null || true
	@echo "已清理所有文件"

# 运行程序（仅限本地编译）
run: $(TARGET)
ifdef CROSS_COMPILE
	@echo "错误：交叉编译的程序无法直接运行"
	@echo "请将 $(TARGET) 复制到 Windows 系统中运行"
else
	./$(TARGET)
endif

# 安装 MinGW64（Ubuntu）
install-mingw:
	@echo "安装 MinGW64 交叉编译工具..."
	sudo apt-get update
	sudo apt-get install -y mingw-w64
	@echo "MinGW64 安装完成"
	@echo "现在可以使用: make PLATFORM=windows"

# 显示帮助信息
help:
	@echo "BAMSYSTEM Makefile - 跨平台编译"
	@echo ""
	@echo "使用方法："
	@echo "  make                           - 自动检测平台并编译"
	@echo "  make PLATFORM=linux            - 编译 Linux 版本"
	@echo "  make PLATFORM=windows          - 编译 Windows 版本"
	@echo "                                   (Ubuntu环境自动使用MinGW64交叉编译)"
	@echo "  make ENABLE_NETWORK=no         - 禁用网络功能（纯本地模式）"
	@echo "  make clean                     - 清理编译产物"
	@echo "  make cleanall                  - 清理所有文件（包括数据）"
	@echo "  make run                       - 编译并运行"
	@echo "  make install-mingw             - 安装MinGW64交叉编译工具(Ubuntu)"
	@echo "  make help                      - 显示此帮助信息"
	@echo ""
	@echo "网络功能开关："
	@echo "  ENABLE_NETWORK=yes   - 启用网络功能（默认，需要 curl 和 cJSON 库）"
	@echo "  ENABLE_NETWORK=no    - 禁用网络功能（纯本地模式，无需额外依赖）"
	@echo ""
	@echo "示例："
	@echo "  # 在 Ubuntu 中编译 Linux 版本"
	@echo "  make PLATFORM=linux"
	@echo ""
	@echo "  # 在 Ubuntu 中交叉编译 Windows 版本（不需要网络功能）"
	@echo "  make PLATFORM=windows ENABLE_NETWORK=no"
	@echo ""
	@echo "  # 在 Ubuntu 中交叉编译带网络功能的 Windows 版本（需要安装MinGW版依赖库）"
	@echo "  make PLATFORM=windows ENABLE_NETWORK=yes"
	@echo ""
	@echo "交叉编译依赖问题："
	@echo "  如果遇到 'cannot find -lcurl' 或 'cannot find -lcjson' 错误："
	@echo "  方案1: 使用 ENABLE_NETWORK=no 编译纯本地版本（推荐）"
	@echo "  方案2: 安装 MinGW 版本的 curl 和 cJSON 库"
	@echo ""
	@echo "当前环境："
ifeq ($(OS),Windows_NT)
	@echo "  操作系统: Windows"
else
	@echo "  操作系统: Linux/Unix"
endif
	@echo "  默认目标平台: $(PLATFORM)"
	@echo "  默认网络功能: $(ENABLE_NETWORK)"

.PHONY: all info clean cleanall run install-mingw help
