#!/bin/bash
# Vortex Engine - Automated Concurrency Benchmark

# Exit immediately if a command exits with a non-zero status
set -e

PORT=8080
URL="http://127.0.0.1:$PORT/"

echo "=========================================="
echo "       VORTEX ENGINE: LOAD TESTER         "
echo "=========================================="

# Check if ApacheBench is installed
if ! command -v ab &> /dev/null; then
    echo "[ERROR] ApacheBench (ab) could not be found."
    echo "Please install it using: sudo apt-get install apache2-utils"
    exit 1
fi

echo "[INFO] Target Engine: $URL"
echo "[INFO] Verifying server status..."

# Quick curl to ensure the server is actually running
if ! curl -s --head  --request GET $URL | grep "200 OK" > /dev/null; then
   echo "[ERROR] Server is not responding on port $PORT."
   echo "Make sure you are running ./build/vortex_server in another terminal."
   exit 1
fi

echo "[INFO] Server is online. Initiating warm-up phase..."
# Send 5,000 requests to prime the OS file cache and socket buffers
ab -n 5000 -c 100 -q $URL > /dev/null 2>&1
echo "[INFO] Warm-up complete. Page cache primed."

echo ""
echo "[INFO] Commencing Maximum Throughput Test..."
echo "[INFO] Payload: 50,000 Requests | 1,000 Concurrent Connections"
echo "------------------------------------------"

# The real crucible: 50k requests, 1k concurrency
ab -n 50000 -c 1000 $URL

echo "------------------------------------------"
echo "[INFO] Stress test concluded."
echo "=========================================="