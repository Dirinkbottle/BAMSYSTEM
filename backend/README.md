# BAMSYSTEM Backend Server

基于Go和MySQL的BAMSYSTEM后端服务器实现。

## 功能特性

- ✅ RESTful API设计
- ✅ MySQL数据库持久化
- ✅ 自动建表机制
- ✅ 安全认证中间件（Client Key + 时间戳）
- ✅ HTTPS/HTTP双模式支持
- ✅ 事务支持（转账操作）
- ✅ 完整的错误处理

## 快速开始

### 1. 安装依赖

```bash
cd backend
go mod download
```

### 2. 配置文件

编辑 `config.json`：

```json
{
  "server": {
    "port": 8080,
    "cert_file": "server/cert.pem",
    "key_file": "server/key.pem"
  },
  "database": {
    "host": "localhost",
    "port": 3306,
    "user": "root",
    "password": "your_password",
    "dbname": "bamsystem"
  }
}
```

### 3. 创建数据库

```sql
CREATE DATABASE bamsystem CHARACTER SET utf8mb4 COLLATE utf8mb4_unicode_ci;
```

数据表会在服务器启动时自动创建。

### 4. 生成HTTPS证书（可选）

```bash
mkdir -p server
openssl req -x509 -newkey rsa:4096 -keyout server/key.pem -out server/cert.pem -days 365 -nodes
```

如果不配置证书，服务器会自动切换到HTTP模式。

### 5. 启动服务器

#### 方式一：直接运行（开发模式）
```bash
go run main.go
```

#### 方式二：使用PM2管理（生产模式，推荐）

首先安装PM2（如果尚未安装）：
```bash
npm install -g pm2
```

然后使用启动脚本：
```bash
# 编译并启动服务器
chmod +x start.sh stop.sh restart.sh
./start.sh

# 查看状态
pm2 status

# 查看日志
pm2 logs bamsystem-backend

# 实时日志
pm2 logs bamsystem-backend --lines 100

# 停止服务器
./stop.sh
# 或
pm2 stop bamsystem-backend

# 重启服务器（自动重新编译）
./restart.sh
# 或
pm2 restart bamsystem-backend

# 删除服务
pm2 delete bamsystem-backend

# 保存PM2配置（开机自启）
pm2 save
pm2 startup
```

## API端点

| 方法 | 端点 | 描述 |
|------|------|------|
| GET | `/api/check` | 检查服务器状态 |
| POST | `/api/account/create` | 创建账户 |
| POST | `/api/account/deposit` | 存款 |
| POST | `/api/account/withdraw` | 取款 |
| POST | `/api/account/transfer` | 转账 |
| DELETE | `/api/account/{uuid}` | 删除账户 |
| POST | `/api/account/sync` | 同步账户 |
| GET | `/api/public_key` | 获取服务器证书 |

## 安全认证

所有API请求（除了 `/api/check`）需要包含以下请求头：

```
Content-Type: application/json
X-Client-Key: <SHA256哈希值，64字符>
X-Request-Time: <Unix时间戳>
```

时间戳验证：允许±5分钟误差。

## 数据库结构

### accounts 表

| 字段 | 类型 | 说明 |
|------|------|------|
| uuid | VARCHAR(36) | 主键，账户UUID |
| balance | BIGINT UNSIGNED | 余额（单位：分） |
| created_at | TIMESTAMP | 创建时间 |
| updated_at | TIMESTAMP | 更新时间 |

## 开发说明

### 项目结构

```
backend/
├── main.go              # 主入口
├── config.json          # 配置文件
├── ecosystem.config.js  # PM2配置文件
├── start.sh             # 启动脚本
├── stop.sh              # 停止脚本
├── restart.sh           # 重启脚本
├── config/
│   └── config.go        # 配置加载
├── database/
│   └── db.go            # 数据库连接
├── models/
│   └── account.go       # 账户模型
├── handlers/
│   └── api.go           # API处理
├── middleware/
│   └── auth.go          # 认证中间件
├── logs/                # 日志目录（PM2自动创建）
└── go.mod               # 依赖管理
```

### 编译

```bash
# Linux/Mac
go build -o bamsystem-server main.go

# Windows
go build -o bamsystem-server.exe main.go
```

### 测试

使用curl测试API：

```bash
# 检查服务器
curl http://localhost:8080/api/check

# 创建账户（需要认证头）
curl -X POST http://localhost:8080/api/account/create \
  -H "Content-Type: application/json" \
  -H "X-Client-Key: abcd1234..." \
  -H "X-Request-Time: $(date +%s)" \
  -d '{"uuid":"550e8400-e29b-41d4-a716-446655440000","balance":0,"timestamp":1699459200}'
```

## 错误码

| HTTP状态码 | 说明 |
|-----------|------|
| 200 | 请求成功 |
| 400 | 请求参数错误 |
| 401 | 认证失败 |
| 404 | 资源不存在 |
| 409 | 资源冲突 |
| 500 | 服务器内部错误 |

## License

MIT License

