#!/bin/bash
# 自動 commit 並 push 的腳本

if [ -z "$1" ]; then
    echo "使用方法: ./gitpush.sh \"commit 訊息\""
    exit 1
fi

cd /home/ntust/Ivan/minigame

git add -A
git commit -m "$1"

# 如果有 remote 就 push
if git remote get-url origin > /dev/null 2>&1; then
    git push origin main 2>/dev/null || git push origin master 2>/dev/null
    echo "✅ 已push到遠端"
else
    echo "ℹ️ 沒有設定遠端，只有本地commit"
fi
