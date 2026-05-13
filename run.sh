#!/bin/bash

set -e

cd "$(dirname "$0")"

SUDO_CMD=""
if [ "$(id -u)" -ne 0 ]; then
    if command -v sudo >/dev/null 2>&1; then
        SUDO_CMD="sudo"
    else
        echo "Warning: Running as non-root and 'sudo' not found. Trying without it..."
    fi
fi

echo "--- 1. Installing System Dependencies ---"
if ! command -v curl >/dev/null 2>&1; then
    $SUDO_CMD apt-get update && $SUDO_CMD apt-get install -y curl
fi

if ! command -v node >/dev/null 2>&1 || [ "$(node -v | cut -d. -f1)" != "v20" ]; then
    echo "Configuring NodeSource for Node.js 20..."
    curl -fsSL https://deb.nodesource.com/setup_20.x | $SUDO_CMD bash -
    $SUDO_CMD apt-get install -y nodejs
fi

PACKAGES="cmake g++ nlohmann-json3-dev zlib1g-dev libssl-dev"
$SUDO_CMD apt-get update
$SUDO_CMD apt-get install -y $PACKAGES

echo "--- 2. Building Backend (C++) ---"
mkdir -p backend/build
cd backend/build
cmake .. 
make -j$(nproc)
cd ../..

echo "--- 3. Preparing Public Folder (Frontend) ---"
rm -rf public
mkdir -p public/src

echo "--- 4. Compiling Frontend (TypeScript) ---"
cd frontend
npm install --quiet
npx tsc --outDir ../public/
cd ..

echo "--- 5. Copying Static Assets ---"
cp frontend/*.html public/ 2>/dev/null || true
cp frontend/*.css public/ 2>/dev/null || true
cp frontend/src/*.js public/src/ 2>/dev/null || true

echo "--- 6. Starting System Monitor ---"
if [ -f "./backend/build/monitor_backend" ]; then
    $SUDO_CMD ./backend/build/monitor_backend
else
    echo "ERROR: Binary 'monitor_backend' not found!"
    exit 1
fi