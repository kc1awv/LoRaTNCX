#!/usr/bin/env node

const fs = require('fs');
const path = require('path');
const { minify } = require('terser');
const csso = require('csso');
const { minify: minifyHtml } = require('html-minifier');

const srcDir = __dirname;
const destDir = path.join(__dirname, '..', 'data');

// Ensure destination directory exists
if (!fs.existsSync(destDir)) {
    fs.mkdirSync(destDir, { recursive: true });
}

console.log('🚀 Building LoRaTNCX web interface...');

// Minify JavaScript
async function minifyJS() {
    console.log('📦 Minifying JavaScript...');
    const inputFile = path.join(srcDir, 'script.js');
    const outputFile = path.join(destDir, 'script.min.js');

    if (!fs.existsSync(inputFile)) {
        console.log('⚠️  script.js not found, skipping JS minification');
        return;
    }

    const code = fs.readFileSync(inputFile, 'utf8');
    const result = await minify(code, {
        compress: {
            drop_console: false, // Keep console.log for debugging
            drop_debugger: true,
        },
        mangle: true,
        format: {
            comments: false,
        },
    });

    fs.writeFileSync(outputFile, result.code);
    console.log(`✅ script.min.js created (${(result.code.length / 1024).toFixed(1)} KB)`);
}

// Minify CSS
function minifyCSS() {
    console.log('🎨 Minifying CSS...');
    const inputFile = path.join(srcDir, 'style.css');
    const outputFile = path.join(destDir, 'style.min.css');

    if (!fs.existsSync(inputFile)) {
        console.log('⚠️  style.css not found, skipping CSS minification');
        return;
    }

    const css = fs.readFileSync(inputFile, 'utf8');
    const result = csso.minify(css);

    fs.writeFileSync(outputFile, result.css);
    console.log(`✅ style.min.css created (${(result.css.length / 1024).toFixed(1)} KB)`);
}

// Minify HTML
function minifyHTML() {
    console.log('📄 Minifying HTML...');
    const inputFile = path.join(srcDir, 'index.html');
    const outputFile = path.join(destDir, 'index.html');

    if (!fs.existsSync(inputFile)) {
        console.log('⚠️  index.html not found, skipping HTML minification');
        return;
    }

    const html = fs.readFileSync(inputFile, 'utf8');
    const result = minifyHtml(html, {
        collapseWhitespace: true,
        removeComments: true,
        removeRedundantAttributes: true,
        removeScriptTypeAttributes: true,
        removeStyleLinkTypeAttributes: true,
        useShortDoctype: true,
        minifyCSS: true,
        minifyJS: true,
    });

    fs.writeFileSync(outputFile, result);
    console.log(`✅ index.html created (${(result.length / 1024).toFixed(1)} KB)`);
}

// Helper function to copy directories recursively
function copyDirRecursive(src, dest) {
    const stats = fs.statSync(src);
    if (stats.isDirectory()) {
        fs.mkdirSync(dest, { recursive: true });
        const files = fs.readdirSync(src);
        files.forEach(file => {
            const srcPath = path.join(src, file);
            const destPath = path.join(dest, file);
            copyDirRecursive(srcPath, destPath);
        });
    } else {
        fs.copyFileSync(src, dest);
    }
}

// Helper function to get directory size
function getDirSize(dirPath) {
    let totalSize = 0;
    const files = fs.readdirSync(dirPath);
    files.forEach(file => {
        const filePath = path.join(dirPath, file);
        const stats = fs.statSync(filePath);
        if (stats.isDirectory()) {
            totalSize += getDirSize(filePath);
        } else {
            totalSize += stats.size;
        }
    });
    return totalSize;
}

// Copy static assets (if any)
function copyAssets() {
    console.log('📋 Copying static assets...');
    const assets = ['font-awesome.min.css', 'popper.min.js', 'tippy-bundle.umd.js', 'tippy-light-theme.css'];
    const directories = ['webfonts'];

    assets.forEach(asset => {
        const srcPath = path.join(srcDir, asset);
        const destPath = path.join(destDir, asset);

        if (fs.existsSync(srcPath)) {
            fs.copyFileSync(srcPath, destPath);
            const stats = fs.statSync(destPath);
            console.log(`✅ ${asset} copied (${(stats.size / 1024).toFixed(1)} KB)`);
        } else {
            console.log(`⚠️  ${asset} not found, skipping`);
        }
    });

    directories.forEach(dir => {
        const srcPath = path.join(srcDir, dir);
        const destPath = path.join(destDir, dir);

        if (fs.existsSync(srcPath)) {
            // Remove existing directory if it exists
            if (fs.existsSync(destPath)) {
                fs.rmSync(destPath, { recursive: true, force: true });
            }
            // Copy directory recursively
            copyDirRecursive(srcPath, destPath);
            const stats = getDirSize(destPath);
            console.log(`✅ ${dir}/ copied (${(stats / 1024).toFixed(1)} KB)`);
        } else {
            console.log(`⚠️  ${dir}/ not found, skipping`);
        }
    });
}

// Main build process
async function build() {
    try {
        await minifyJS();
        minifyCSS();
        minifyHTML();
        copyAssets();

        console.log('\n🎉 Build completed successfully!');
        console.log('📁 Minified files are ready in the data/ folder');
        console.log('🚀 Run "platformio run --target uploadfs" to upload to ESP32');

    } catch (error) {
        console.error('❌ Build failed:', error.message);
        process.exit(1);
    }
}

build();