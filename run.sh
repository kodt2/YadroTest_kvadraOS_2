#!/bin/bash

set -e

cd "$(dirname "$0")"

echo "--- Installing System Dependencies ---"
sudo apt-get update
sudo apt-get install -y cmake g++ nodejs npm nlohmann-json3-dev

echo "--- Building Backend ---"
#rm -rf backend/build
mkdir -p backend/build
cd backend/build
cmake .. -DCMAKE_POLICY_VERSION_MINIMUM=3.5
make -j$(nproc)
cd ../..

echo "--- Preparing Public Folder ---"
rm -rf public
mkdir -p public

echo "--- Compiling Frontend (TypeScript) ---"
cd frontend
npm install --quiet
npx -p typescript tsc --project tsconfig.json --rootDir src --outDir ../public/src
cd ..

cp frontend/*.html public/ 2>/dev/null || true
cp frontend/*.css public/ 2>/dev/null || true

cp frontend/src/*.js public/ 2>/dev/null || true
cp frontend/*.js public/ 2>/dev/null || true

echo "--- Starting System Monitor ---"
./backend/build/monitor_backend