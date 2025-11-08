# BAMSYSTEM 服务器API规范文档

## 概述

本文档为BAMSYSTEM服务器端API接口规范，供后端开发人员实现。

**版本**: 1.0  
**日期**: 2025-11-08  
**协议**: HTTP/HTTPS + JSON  

---

## 认证机制

### 请求头（HTTP Headers）

所有API请求必须包含以下认证头：

```
Content-Type: application/json
X-Client-Key: <客户端唯一标识，SHA256哈希值>
X-Request-Time: <Unix时间戳>
```

### 客户端标识生成

客户端唯一标识（Client Key）生成规则：
- 读取客户端本地`system.key`文件（16字节）
- 计算SHA256哈希
- 转换为64位十六进制字符串

---

## API端点

### 1. 服务器状态检测

**端点**: `GET /api/check`

**描述**: 检测服务器是否支持客户端连接

**请求参数**: 无

**响应示例**:
```json
{
  "status": "Support"
}
```
或
```json
{
  "status": "noSupport"
}
```

**说明**:
- 返回`"Support"`：客户端切换到服务器模式
- 返回`"noSupport"`：客户端切换到本地模式

---

### 2. 创建账户

**端点**: `POST /api/account/create`

**描述**: 创建新账户并同步到服务器

**请求体**:
```json
{
  "uuid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "balance": 0,
  "timestamp": 1699459200
}
```

**字段说明**:
- `uuid`: 账户唯一标识（UUID v4格式）
- `balance`: 账户余额（单位：分，整数）
- `timestamp`: 请求时间戳（Unix时间）

**响应示例**:
```json
{
  "success": true,
  "message": "账户创建成功"
}
```

**错误响应**:
```json
{
  "success": false,
  "error": "账户已存在"
}
```

---

### 3. 存款

**端点**: `POST /api/account/deposit`

**描述**: 向指定账户存款

**请求体**:
```json
{
  "uuid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "amount": 10000,
  "timestamp": 1699459200
}
```

**字段说明**:
- `uuid`: 账户UUID
- `amount`: 存款金额（单位：分，整数）
- `timestamp`: 请求时间戳

**响应示例**:
```json
{
  "success": true,
  "balance": 10000,
  "message": "存款成功"
}
```

---

### 4. 取款

**端点**: `POST /api/account/withdraw`

**描述**: 从指定账户取款

**请求体**:
```json
{
  "uuid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "amount": 5000,
  "timestamp": 1699459200
}
```

**字段说明**:
- `uuid`: 账户UUID
- `amount`: 取款金额（单位：分，整数）
- `timestamp`: 请求时间戳

**响应示例**:
```json
{
  "success": true,
  "balance": 5000,
  "message": "取款成功"
}
```

**错误响应**:
```json
{
  "success": false,
  "error": "余额不足"
}
```

---

### 5. 转账

**端点**: `POST /api/account/transfer`

**描述**: 从一个账户向另一个账户转账

**请求体**:
```json
{
  "uuid_from": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "uuid_to": "yyyyyyyy-yyyy-yyyy-yyyy-yyyyyyyyyyyy",
  "amount": 3000,
  "timestamp": 1699459200
}
```

**字段说明**:
- `uuid_from`: 转出账户UUID
- `uuid_to`: 转入账户UUID
- `amount`: 转账金额（单位：分，整数）
- `timestamp`: 请求时间戳

**响应示例**:
```json
{
  "success": true,
  "message": "转账成功"
}
```

**错误响应**:
```json
{
  "success": false,
  "error": "转出账户余额不足"
}
```

---

### 6. 销户

**端点**: `DELETE /api/account/{uuid}`

**描述**: 删除指定账户

**URL参数**:
- `uuid`: 要删除的账户UUID

**请求体**: 无

**响应示例**:
```json
{
  "success": true,
  "message": "账户已删除"
}
```

---

### 7. 同步账户数据

**端点**: `POST /api/account/sync`

**描述**: 完整同步账户数据到服务器

**请求体**:
```json
{
  "uuid": "xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx",
  "balance": 15000,
  "timestamp": 1699459200
}
```

**响应示例**:
```json
{
  "success": true,
  "message": "账户数据已同步"
}
```

---

### 8. 获取服务器公钥/证书

**端点**: `GET /api/public_key`

**描述**: 获取服务器CA证书（用于HTTPS验证）

**请求参数**: 无

**响应示例**:
```json
{
  "certificate": "-----BEGIN CERTIFICATE-----\n...\n-----END CERTIFICATE-----"
}
```

**说明**:
- 返回PEM格式的CA证书
- 客户端将证书保存到`server/ca-cert.pem`

---

## 安全建议

### 基础安全（必须实现）

1. **HTTPS通信**: 强制使用TLS 1.2+加密通信
2. **客户端认证**: 验证`X-Client-Key`头，确保客户端身份
3. **时间戳验证**: 验证`X-Request-Time`，防止重放攻击（建议允许±5分钟误差）
4. **输入验证**: 严格验证所有输入参数（UUID格式、金额范围等）

### 增强安全（推荐实现）

1. **请求签名验证**:
   - 使用HMAC-SHA256对请求体进行签名
   - 签名放在`X-Request-Signature`头中
   - 服务器使用相同密钥验证签名

2. **Nonce机制**:
   - 每个请求携带唯一随机数`nonce`
   - 服务器记录已使用的nonce，防止重放

3. **速率限制**:
   - 对每个客户端进行API调用频率限制
   - 防止暴力攻击和滥用

4. **账户锁定**:
   - 多次失败操作后临时锁定账户
   - 防止恶意攻击

5. **审计日志**:
   - 记录所有交易操作
   - 便于追溯和审计

### 证书固定（可选）

客户端可实现证书指纹验证：
```c
// 硬编码服务器证书SHA256指纹
const char* EXPECTED_CERT_FINGERPRINT = "abcd1234...";
// 在HTTPS连接时验证证书指纹是否匹配
```

---

## 错误码规范

| HTTP状态码 | 说明 |
|-----------|------|
| 200 | 请求成功 |
| 400 | 请求参数错误 |
| 401 | 认证失败 |
| 403 | 无权限 |
| 404 | 资源不存在 |
| 409 | 资源冲突（如账户已存在） |
| 429 | 请求过于频繁 |
| 500 | 服务器内部错误 |

---

## 数据格式说明

### 金额表示

- 所有金额使用**整数**表示，单位为**分**（cent）
- 例如：100元 = 10000分
- 这样可以避免浮点数精度问题

### UUID格式

- 标准UUID v4格式
- 小写字母
- 示例：`550e8400-e29b-41d4-a716-446655440000`

### 时间戳

- Unix时间戳（秒）
- 32位/64位整数
- 示例：`1699459200`

---

## 测试建议

### 测试工具

推荐使用以下工具测试API：
- Postman
- curl命令行
- Python requests库

### 测试用例

1. **正常流程**:
   - 创建账户 → 存款 → 取款 → 转账 → 销户

2. **异常场景**:
   - 重复创建账户
   - 余额不足取款
   - 转账到不存在的账户
   - 无效的UUID格式
   - 过期的时间戳

3. **安全测试**:
   - 无认证头访问
   - 伪造客户端ID
   - 重放攻击
   - SQL注入测试

---

## 示例实现（伪代码）

### Python Flask示例

```python
from flask import Flask, request, jsonify
import time

app = Flask(__name__)

@app.route('/api/check', methods=['GET'])
def check_server():
    return jsonify({"status": "Support"})

@app.route('/api/account/create', methods=['POST'])
def create_account():
    # 验证认证头
    client_key = request.headers.get('X-Client-Key')
    if not verify_client(client_key):
        return jsonify({"success": False, "error": "认证失败"}), 401
    
    # 解析请求
    data = request.json
    uuid = data.get('uuid')
    balance = data.get('balance', 0)
    
    # 业务逻辑
    if account_exists(uuid):
        return jsonify({"success": False, "error": "账户已存在"}), 409
    
    create_account_in_db(uuid, balance)
    return jsonify({"success": True, "message": "账户创建成功"})

# ... 其他端点实现
```

---


**文档更新日期**: 2025-11-08

