#!/bin/bash

# LoRaTNCX Web Build Script
# Convenience script to build web interface from project root

echo "🔧 Building LoRaTNCX web interface..."
cd web && npm run build
echo "✅ Web interface built successfully!"

echo "📋 Copying files to data directory for SPIFFS upload..."
cp -r dist/* ../data/
echo "✅ Files copied to data/ directory!"

echo "📤 Ready to upload with: platformio run --target uploadfs --environment heltec_wifi_lora_32_V4"