#!/usr/bin/env python3
"""
Web Interface Builder for LoRaTNCX
Automatically downloads dependencies and builds the web interface
"""

import os
import sys
import gzip
import shutil
import hashlib
import requests
from pathlib import Path

def build_web_interface(*args, **kwargs):
    """Build and compress the web interface - accepts any arguments from SCons"""
    try:
        current_dir = Path(__file__).parent
        project_root = current_dir.parent
        web_dir = project_root / "web"
        src_dir = web_dir / "src"
        dist_dir = web_dir / "dist"
        data_dir = project_root / "data"
        
        # Create directories if they don't exist
        dist_dir.mkdir(parents=True, exist_ok=True)
        data_dir.mkdir(parents=True, exist_ok=True)
        
        print("📦 Building web interface...")
        
        # Download external dependencies
        deps = {
            "bootstrap.min.css": "https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css",
            "bootstrap.bundle.min.js": "https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js",
            "bootstrap-icons.min.css": "https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.1/font/bootstrap-icons.css",
            "chart.umd.js": "https://cdn.jsdelivr.net/npm/chart.js@4.4.0/dist/chart.umd.js"
        }
        
        # Download font files for Bootstrap Icons
        font_deps = {
            "bootstrap-icons.woff": "https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.1/font/fonts/bootstrap-icons.woff",
            "bootstrap-icons.woff2": "https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.1/font/fonts/bootstrap-icons.woff2"
        }
        
        for filename, url in {**deps, **font_deps}.items():
            file_path = dist_dir / filename
            if not file_path.exists():
                print(f"Downloading {filename}...")
                response = requests.get(url, timeout=30)
                response.raise_for_status()
                file_path.write_bytes(response.content)
        
        # Copy source files
        for src_file in src_dir.glob("*"):
            if src_file.is_file():
                shutil.copy2(src_file, dist_dir)
        
        # Copy all files to data directory (both compressed and uncompressed)
        for file_path in dist_dir.glob("*"):
            if file_path.is_file():
                shutil.copy2(file_path, data_dir)
        
        # Compress all files
        total_size = 0
        for file_path in dist_dir.glob("*"):
            if file_path.is_file() and not file_path.name.endswith('.gz'):
                print(f"Compressing {file_path.name}...")
                gz_path = data_dir / f"{file_path.name}.gz"
                
                with open(file_path, 'rb') as f_in:
                    with gzip.open(gz_path, 'wb') as f_out:
                        f_out.write(f_in.read())
                
                total_size += gz_path.stat().st_size
        
        print(f"✅ Web interface built successfully!")
        print(f"📊 Total compressed size: {total_size / 1024:.1f} KB")
        
        # Check SPIFFS capacity (9.375 MB available)
        spiffs_size = 9.375 * 1024 * 1024
        usage_percent = (total_size / spiffs_size) * 100
        print(f"💾 SPIFFS usage: {usage_percent:.1f}%")
        
        return True
        
    except Exception as e:
        print(f"❌ Error building web interface: {e}")
        return False

if __name__ == "__main__":
    build_web_interface()