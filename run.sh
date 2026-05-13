#!/bin/bash
set -e

SUDO_CMD=""
if [ "$(id -u)" -ne 0 ]; then
    SUDO_CMD="sudo"
fi

echo "--- Checking Dependencies ---"
PACKAGES="cmake g++ nodejs npm nlohmann-json3-dev zlib1g-dev libssl-dev"
TO_INSTALL=""

for pkg in $PACKAGES; do
    if ! dpkg -s "$pkg" >/dev/null 2>&1; then
        TO_INSTALL="$TO_INSTALL $pkg"
    fi
done

if [ -z "$TO_INSTALL" ]; then
    echo "All dependencies are already installed. Skipping..."
else
    echo "Installing missing: $TO_INSTALL"
    $SUDO_CMD apt-get update
    $SUDO_CMD apt-get install -y $TO_INSTALL
fi

echo "--- Building Backend ---"
mkdir -p backend/build
cd backend/build
cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j$(nproc)
cd ../..

echo "--- Preparing Public Folder ---"
rm -rf public/*
mkdir -p public/src

echo "--- Compiling Frontend (TypeScript) ---"
cd frontend
npm install --quiet
npx tsc --outDir ../public/
cd ..

cp frontend/*.html public/ 2>/dev/null || true
cp frontend/*.css public/ 2>/dev/null || true

cp frontend/src/*.js public/src/ 2>/dev/null || true

echo "--- Starting System Monitor ---"
if [ -f "./backend/build/monitor_backend" ]; then
    ./backend/build/monitor_backend
else
    echo "Error: Binary not found at ./backend/build/monitor_backend"
    ls ./backend/build/
fi