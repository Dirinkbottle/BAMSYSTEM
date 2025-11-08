#!/bin/bash
# BAMSYSTEM Backend 启动脚本

set -e

BACKEND_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$BACKEND_DIR"

echo "=========================================="
echo "  BAMSYSTEM Backend Server 启动脚本"
echo "=========================================="

# 检查Go是否安装
if ! command -v go &> /dev/null; then
    echo "错误: Go未安装，请先安装Go"
    exit 1
fi

# 检查配置文件
if [ ! -f "config.json" ]; then
    echo "错误: config.json 配置文件不存在"
    exit 1
fi

# 编译Go程序
echo "正在编译Go程序..."
go build -o bamsystem-server main.go

if [ $? -ne 0 ]; then
    echo "错误: 编译失败"
    exit 1
fi

echo "✓ 编译成功"

# 创建日志目录
mkdir -p logs

# 检查PM2是否安装
if ! command -v pm2 &> /dev/null; then
    echo "警告: PM2未安装"
    echo "请运行: npm install -g pm2"
    echo ""
    echo "直接启动服务器..."
    ./bamsystem-server
else
    # 使用PM2启动
    echo "使用PM2启动服务器..."
    pm2 start ecosystem.config.js
    
    echo ""
    echo "=========================================="
    echo "  服务器已启动"
    echo "=========================================="
    echo "查看状态: pm2 status"
    echo "查看日志: pm2 logs bamsystem-backend"
    echo "停止服务: pm2 stop bamsystem-backend"
    echo "重启服务: pm2 restart bamsystem-backend"
    echo "删除服务: pm2 delete bamsystem-backend"
    echo "=========================================="
fi

