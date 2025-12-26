# BAMSYSTEM - 银行账户管理系统

较为完备的银行账户管理系统，支持本地模式和服务器模式。

## 功能特性

### 核心功能
- ✅ 创建账户（UUID + 7位密码）
- ✅ 账户存款
- ✅ 账户取款
- ✅ 账户转账
- ✅ 注销账户
- ✅ 数据加密存储（XOR加密）
- ✅ 跨平台支持（Windows/Linux）

### 服务器同步功能（v1.0新增）
- ✅ 双模式运行：本地模式 / 联网模式
- ✅ 自动检测服务器可用性
- ✅ HTTPS安全通信
- ✅ 服务器证书验证
- ✅ 客户端身份认证
- ✅ 所有交易实时同步到服务器
- ✅ 本地Card文件保留兼容

## 运行模式

### 本地模式
- 所有数据存储在本地`Card/`目录
- 系统密钥存储在`system.key`文件
- 无需网络连接

### 服务器模式（联网模式）
- 本地存储 + 服务器同步
- 所有账户操作自动同步到服务器
- 服务器不可用时自动降级到本地模式
- 网络故障不影响本地操作

## 编译说明

### Linux平台

#### 安装依赖
```bash
# Ubuntu/Debian
sudo apt-get update
sudo apt-get install -y build-essential uuid-dev libcurl4-openssl-dev libcjson-dev libssl-dev

# 或者从源码编译cJSON（如果包管理器没有）
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON
mkdir build && cd build
cmake ..
make
sudo make install
```

#### 编译
```bash
make PLATFORM=linux
```

### Windows平台

#### 在Linux中交叉编译Windows版本
```bash
# 安装MinGW64
sudo apt-get install -y mingw-w64

# 需要准备Windows版本的库文件（libcurl, cJSON）
# 将库文件放在MinGW的lib目录下

make PLATFORM=windows
```

#### 在Windows中直接编译
需要安装：
- MinGW-w64或MSVC编译器
- libcurl Windows版本
- cJSON Windows版本

```bash
make PLATFORM=windows
```

## 运行说明

### 首次运行

1. **配置服务器（可选）**

编辑`server.conf`文件：
```ini
[server]
url=https://your-server.com
port=443
timeout=10

[security]
use_https=true
verify_cert=true
cert_path=server/ca-cert.pem
```

2. **运行程序**
```bash
./bamsystem          # Linux
bamsystem.exe        # Windows
```

3. **查看运行模式**

程序启动时会自动检测服务器：
- 显示"联网版本"：所有操作会同步到服务器
- 显示"本地版本"：仅使用本地存储

### 服务器配置

#### 禁用服务器功能
如果不需要服务器同步，有两种方式：

1. 删除或重命名`server.conf`文件
2. 修改`server.conf`中的URL为无效地址

#### 获取服务器证书
首次连接HTTPS服务器时：
- 如果`server/ca-cert.pem`不存在
- 程序会尝试从服务器获取证书
- 建议验证证书指纹确保安全

## 目录结构

```
BAMSYSTEM/
├── main.c              # 主程序
├── account.c           # 账户管理模块
├── ui.c               # 用户界面模块
├── platform.c         # 平台相关功能
├── server_api.c       # 服务器API模块（新增）
├── lib/               # 头文件
│   ├── account.h
│   ├── ui.h
│   ├── platform.h
│   └── server_api.h   # 服务器API头文件（新增）
├── Card/              # 账户数据目录（自动创建）
├── server/            # 服务器证书目录（自动创建）
├── system.key         # 系统密钥（自动生成）
├── server.conf        # 服务器配置文件（新增）
├── Makefile           # 编译脚本
└── README.md          # 本文件
```

## 数据安全

### 本地安全
- 账户密码使用7位数字（1000000-9999999）
- 账户数据使用XOR加密存储
- 系统密钥（128位）存储在`system.key`

### 网络安全
- HTTPS/TLS加密通信
- 服务器证书验证
- 客户端身份认证（基于system.key的SHA256哈希）
- 请求时间戳防重放攻击

### 增强安全建议
1. 定期备份`system.key`和`Card/`目录
2. 验证服务器证书指纹
3. 使用强密码
4. 不要在公共环境运行

## 服务器端开发

如需开发服务器端，请参考：
- `SERVER_API_SPEC.md` - 完整的API规范文档
- 包含所有端点定义、认证机制、安全建议

### 快速开始
服务器需要实现以下核心端点：
- `GET /api/check` - 状态检测
- `POST /api/account/create` - 创建账户
- `POST /api/account/deposit` - 存款
- `POST /api/account/withdraw` - 取款
- `POST /api/account/transfer` - 转账
- `DELETE /api/account/{uuid}` - 销户

详细规范请查看`SERVER_API_SPEC.md`。

## 故障排除

### 编译错误

**错误**：找不到`curl/curl.h`
```bash
# Ubuntu/Debian
sudo apt-get install libcurl4-openssl-dev

# CentOS/RHEL
sudo yum install libcurl-devel
```

**错误**：找不到`cjson/cJSON.h`
```bash
# 从源码安装
git clone https://github.com/DaveGamble/cJSON.git
cd cJSON && mkdir build && cd build
cmake .. && make && sudo make install
sudo ldconfig  # 更新库缓存
```

### 运行错误

**错误**：`服务器API初始化失败`
- 检查是否安装了libcurl运行库
- Linux: `sudo apt-get install libcurl4`
- 如不需要服务器功能，删除`server.conf`即可

**错误**：`服务器同步失败`
- 检查网络连接
- 检查服务器配置是否正确
- 查看服务器是否正常运行
- 不影响本地操作，数据已保存到本地

## 使用示例

### 创建账户并存款
```
1. 选择"1.创建账户"
2. 输入7位密码：1234567
3. 记下生成的UUID
4. 选择"2.账户存款"
5. 输入UUID和金额
```

### 转账
```
1. 选择"4.账户转账"
2. 输入转出账户UUID
3. 输入密码验证
4. 输入转入账户UUID
5. 输入转账金额
```

## 技术栈

- **语言**：C (C99标准)
- **编译器**：GCC / MinGW-w64
- **跨平台**：支持Windows和Linux
- **依赖库**：
  - uuid (Linux) / rpcrt4 (Windows) - UUID生成
  - libcurl - HTTP/HTTPS通信
  - cJSON - JSON数据处理
  - OpenSSL - 加密功能（Linux）
  - CryptoAPI - 加密功能（Windows）

## 开发团队

BAMSYSTEM团队:
- 周雨(Dirinkbottle):负责核心代码开发工作
- 彭爽爽:
- 李雨峰:
- 全伟:
- 闫攀:
- 任荣基:

## 许可证

详见LICENSE文件

## 版本历史

### v1.0 (2025-11-08)
- ✨ 新增服务器同步功能
- ✨ 支持双模式运行（本地/联网）
- ✨ HTTPS安全通信
- ✨ 完整的API文档
- 🔧 优化代码注释规范
- 🔧 更新Makefile

### v0.9 (初始版本)
- ✅ 基础账户管理功能
- ✅ 本地数据加密存储
- ✅ 跨平台支持

## 贡献指南

欢迎提交Issue和Pull Request！

## 联系方式

- GitHub: https://github.com/Dirinkbottle
- 问题反馈: 请在GitHub Issues中提交
