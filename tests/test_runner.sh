#!/usr/bin/env bash
# Automated Verification & Smoke Test Script for V-BioWatch

set -e

echo "=== Running Automated Test Suite: V-BioWatch ==="

echo "[1/4] Building all components..."
make clean
make all

echo "[2/4] Verifying build outputs..."
test -f driver/smart_watch_bio.ko
test -f src/biowatch_daemon
echo "Binaries verified."

echo "[3/4] Loading driver and testing node..."
sudo insmod driver/smart_watch_bio.ko
sudo chmod 666 /dev/smart_watch_bio

echo "Checking /dev/smart_watch_bio:"
ls -l /dev/smart_watch_bio
cat /dev/smart_watch_bio

echo "[4/4] Cleaning up..."
sudo rmmod smart_watch_bio
echo "All sanity checks PASSED successfully!"
