#!/bin/bash
# BAMSYSTEM Backend 重启脚本

set -e

BACKEND_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$BACKEND_DIR"

echo "=========================================="
echo "  重启 BAMSYSTEM Backend Server"
echo "=========================================="

if ! command -v pm2 &> /dev/null; then
    echo "错误: PM2未安装"
    exit 1
fi

# 重新编译
echo "正在重新编译..."
go build -o bamsystem-server main.go

if [ $? -ne 0 ]; then
    echo "错误: 编译失败"
    exit 1
fi

echo "✓ 编译成功"

# 重启服务
pm2 restart bamsystem-backend

echo ""
echo "✓ 服务器已重启"
echo "查看日志: pm2 logs bamsystem-backend"

