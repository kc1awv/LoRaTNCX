#!/usr/bin/env python3
"""
LoRaTNCX Web Interface Build Script
Processes web source files and prepares them for SPIFFS deployment
"""

import os
import shutil
import gzip
import json
import argparse
from pathlib import Path
import requests
import tempfile
from typing import BinaryIO

class WebBuilder:
    def __init__(self, src_dir, dist_dir, data_dir):
        self.src_dir = Path(src_dir)
        self.dist_dir = Path(dist_dir)
        self.data_dir = Path(data_dir)
        
    def clean_dist(self):
        """Clean the distribution directory"""
        if self.dist_dir.exists():
            shutil.rmtree(self.dist_dir)
        self.dist_dir.mkdir(parents=True, exist_ok=True)
        
        if self.data_dir.exists():
            shutil.rmtree(self.data_dir)
        self.data_dir.mkdir(parents=True, exist_ok=True)
    
    def download_bootstrap(self):
        """Download Bootstrap CSS and JS files"""
        bootstrap_version = "5.3.2"
        bootstrap_icons_version = "1.11.1"
        chartjs_version = "4.4.0"
        
        files_to_download = [
            {
                'url': f'https://cdn.jsdelivr.net/npm/bootstrap@{bootstrap_version}/dist/css/bootstrap.min.css',
                'filename': 'bootstrap.min.css'
            },
            {
                'url': f'https://cdn.jsdelivr.net/npm/bootstrap@{bootstrap_version}/dist/js/bootstrap.bundle.min.js',
                'filename': 'bootstrap.bundle.min.js'
            },
            {
                'url': f'https://cdn.jsdelivr.net/npm/bootstrap-icons@{bootstrap_icons_version}/font/bootstrap-icons.min.css',
                'filename': 'bootstrap-icons.min.css'
            },
            {
                'url': f'https://cdn.jsdelivr.net/npm/chart.js@{chartjs_version}/dist/chart.min.js',
                'filename': 'chart.min.js'
            }
        ]
        
        print("Downloading external dependencies...")
        for file_info in files_to_download:
            self.download_file(file_info['url'], self.dist_dir / file_info['filename'])
    
    def download_file(self, url, filepath):
        """Download a file from URL"""
        try:
            print(f"  Downloading {filepath.name}...")
            response = requests.get(url)
            response.raise_for_status()
            
            with open(filepath, 'wb') as f:
                f.write(response.content)
            
            print(f"    Downloaded {len(response.content)} bytes")
        except Exception as e:
            print(f"    Error downloading {url}: {e}")
            return False
        return True
    
    def process_html(self):
        """Process HTML files and update CDN references to local files"""
        html_file = self.src_dir / 'index.html'
        if not html_file.exists():
            print("Error: index.html not found in src directory")
            return False
        
        print("Processing HTML file...")
        with open(html_file, 'r') as f:
            content = f.read()
        
        # Replace CDN links with local files
        replacements = {
            'https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/css/bootstrap.min.css': 'bootstrap.min.css',
            'https://cdn.jsdelivr.net/npm/bootstrap-icons@1.11.1/font/bootstrap-icons.css': 'bootstrap-icons.min.css',
            'https://cdn.jsdelivr.net/npm/bootstrap@5.3.2/dist/js/bootstrap.bundle.min.js': 'bootstrap.bundle.min.js',
            'https://cdn.jsdelivr.net/npm/chart.js': 'chart.min.js'
        }
        
        for cdn_url, local_file in replacements.items():
            content = content.replace(cdn_url, local_file)
        
        # Write processed HTML
        output_file = self.dist_dir / 'index.html'
        with open(output_file, 'w') as f:
            f.write(content)
        
        print(f"  Processed HTML: {output_file}")
        return True
    
    def copy_assets(self):
        """Copy CSS and JS files"""
        print("Copying assets...")
        
        # Copy custom CSS
        css_file = self.src_dir / 'style.css'
        if css_file.exists():
            shutil.copy2(css_file, self.dist_dir / 'style.css')
            print(f"  Copied: style.css")
        
        # Copy custom JS
        js_file = self.src_dir / 'app.js'
        if js_file.exists():
            shutil.copy2(js_file, self.dist_dir / 'app.js')
            print(f"  Copied: app.js")
    
    def compress_files(self):
        """Compress files with gzip for smaller SPIFFS usage"""
        print("Compressing files...")
        
        # Files to compress
        compress_extensions = ['.html', '.css', '.js']
        
        for file_path in self.dist_dir.glob('*'):
            if file_path.suffix in compress_extensions:
                self.compress_file(file_path)
    
    def compress_file(self, file_path):
        """Compress a single file with gzip"""
        compressed_path = file_path.with_suffix(file_path.suffix + '.gz')
        
        with open(file_path, 'rb') as f_in:
            content = f_in.read()
        
        # Create gzip file manually
        with open(compressed_path, 'wb') as f_out:
            f_out.write(gzip.compress(content))
        
        original_size = file_path.stat().st_size
        compressed_size = compressed_path.stat().st_size
        ratio = (1 - compressed_size / original_size) * 100
        
        print(f"  {file_path.name}: {original_size} -> {compressed_size} bytes ({ratio:.1f}% reduction)")
    
    def prepare_spiffs(self):
        """Prepare files for SPIFFS deployment"""
        print("Preparing SPIFFS data...")
        
        # Copy all files to data directory
        for file_path in self.dist_dir.glob('*'):
            if file_path.is_file():
                shutil.copy2(file_path, self.data_dir / file_path.name)
        
        print(f"  Files copied to: {self.data_dir}")
    
    def generate_manifest(self):
        """Generate a manifest file with build information"""
        manifest = {
            "build_time": "2024-10-27T12:00:00Z",
            "version": "1.0.0",
            "files": []
        }
        
        total_size = 0
        for file_path in self.data_dir.glob('*'):
            if file_path.is_file() and file_path.name != 'manifest.json':
                size = file_path.stat().st_size
                total_size += size
                manifest["files"].append({
                    "name": file_path.name,
                    "size": size
                })
        
        manifest["total_size"] = total_size
        
        with open(self.data_dir / 'manifest.json', 'w') as f:
            json.dump(manifest, f, indent=2)
        
        print(f"  Generated manifest: {total_size} bytes total")
    
    def print_summary(self):
        """Print build summary"""
        print("\n" + "="*50)
        print("BUILD SUMMARY")
        print("="*50)
        
        total_size = 0
        file_count = 0
        
        print(f"{'File':<30} {'Size':<15} {'Type':<10}")
        print("-" * 55)
        
        for file_path in sorted(self.data_dir.glob('*')):
            if file_path.is_file():
                size = file_path.stat().st_size
                total_size += size
                file_count += 1
                
                file_type = "Compressed" if file_path.suffix == '.gz' else "Regular"
                print(f"{file_path.name:<30} {self.format_size(size):<15} {file_type:<10}")
        
        print("-" * 55)
        print(f"{'TOTAL':<30} {self.format_size(total_size):<15} {file_count} files")
        print(f"\nSPIFFS Usage: {self.format_size(total_size)} / 9.375 MB ({(total_size / (9.375 * 1024 * 1024) * 100):.1f}%)")
        print("="*50)
    
    def format_size(self, size):
        """Format file size in human readable format"""
        for unit in ['B', 'KB', 'MB']:
            if size < 1024:
                return f"{size:.1f} {unit}"
            size /= 1024
        return f"{size:.1f} GB"
    
    def build(self, download_deps=True):
        """Run the complete build process"""
        print("Starting LoRaTNCX Web Interface Build")
        print("="*50)
        
        # Clean previous build
        self.clean_dist()
        
        # Download dependencies
        if download_deps:
            self.download_bootstrap()
        
        # Process files
        if not self.process_html():
            return False
        
        self.copy_assets()
        self.compress_files()
        self.prepare_spiffs()
        self.generate_manifest()
        
        # Print summary
        self.print_summary()
        
        print(f"\nBuild completed successfully!")
        print(f"Files ready for SPIFFS deployment in: {self.data_dir}")
        
        return True

def main():
    parser = argparse.ArgumentParser(description='Build LoRaTNCX Web Interface')
    parser.add_argument('--no-download', action='store_true', 
                       help='Skip downloading external dependencies')
    parser.add_argument('--src', default='web/src', 
                       help='Source directory (default: web/src)')
    parser.add_argument('--dist', default='web/dist', 
                       help='Distribution directory (default: web/dist)')
    parser.add_argument('--data', default='data', 
                       help='SPIFFS data directory (default: data)')
    
    args = parser.parse_args()
    
    builder = WebBuilder(args.src, args.dist, args.data)
    success = builder.build(download_deps=not args.no_download)
    
    return 0 if success else 1

if __name__ == '__main__':
    exit(main())