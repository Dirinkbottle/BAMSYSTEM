#!/bin/bash
# BAMSYSTEM Backend 停止脚本

set -e

BACKEND_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
cd "$BACKEND_DIR"

echo "=========================================="
echo "  停止 BAMSYSTEM Backend Server"
echo "=========================================="

if ! command -v pm2 &> /dev/null; then
    echo "错误: PM2未安装"
    exit 1
fi

pm2 stop bamsystem-backend
echo "✓ 服务器已停止"

echo ""
echo "选项:"
echo "  重启服务: pm2 restart bamsystem-backend"
echo "  删除服务: pm2 delete bamsystem-backend"

