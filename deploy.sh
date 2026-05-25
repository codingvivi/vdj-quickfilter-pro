#!/bin/bash
# Run this on Linux to sync, build, and deploy to the Mac.
# Edit MAC_HOST below to match your setup.
set -e

MAC_HOST="mbp-21-16"                       # SSH host — set this (e.g. "user@192.168.1.50")
MAC_DIR="~/code/vdj-quickfilter-pro"   # where to put the project on the Mac

# Sync source to Mac (skip build artifacts, LSP cache, stubs)
rsync -avz --delete \
    --exclude='build/' \
    --exclude='tools/stubs/' \
    --exclude='.cache/' \
    --exclude='compile_flags.txt' \
    ./ "${MAC_HOST}:${MAC_DIR}/"

# Build + bundle + install (one SSH session)
ssh "$MAC_HOST" "
    cd ${MAC_DIR}
    meson setup build 2>/dev/null || true
    meson compile -C build
    bash bundle.sh
"
