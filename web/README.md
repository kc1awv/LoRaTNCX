# LoRaTNCX Web Interface Development

This folder contains the development version of the LoRaTNCX web interface. Here you can work with unminified, readable code.

## Development Workflow

### 1. Make Changes
Edit the source files in this folder:
- `index.html` - Main HTML structure
- `style.css` - CSS styles (unminified)
- `script.js` - JavaScript functionality (unminified)

### 2. Build for Production
When changes are complete, run the build process:

```bash
npm run build
# or
node build.js
```

This will:
- Minify JavaScript with Terser
- Minify CSS with CSSO
- Minify HTML with html-minifier
- Copy minified files to `../data/` folder
- Copy static assets (Font Awesome CSS, Tippy.js, webfonts)

### 3. Upload to ESP32
After building, upload the filesystem to your ESP32:

```bash
cd ..
platformio run --target uploadfs --environment heltec_wifi_lora_32_V4
```

## Available Scripts

- `npm run build` - Build and minify all files
- `npm run dev` - Same as build (for development convenience)
- `npm run clean` - Remove built files from data/ folder

## Tools Used

- **JavaScript**: [Terser](https://github.com/terser/terser) - Modern JavaScript minifier
- **CSS**: [CSSO](https://github.com/css/csso) - CSS minifier with structural optimizations
- **HTML**: [html-minifier](https://github.com/kangax/html-minifier) - HTML minifier

## File Structure

```
web/
├── index.html             # Source HTML (readable)
├── style.css              # Source CSS (readable)
├── script.js              # Source JavaScript (readable)
├── webfonts/              # Font Awesome font files
│   └── fa-solid-900.woff2
├── build.js               # Build script
├── package.json           # Node.js dependencies
└── README.md              # This file

data/ (generated)
├── index.html             # Minified HTML (18-19KB)
├── style.min.css          # Minified CSS (11-12KB)
├── script.min.js          # Minified JavaScript (20-21KB)
├── webfonts/              # Font Awesome fonts (124KB)
├── font-awesome.min.css   # Font Awesome CSS (87KB)
├── tippy-bundle.umd.js    # Tooltip library (25KB)
├── tippy-light-theme.css  # Tooltip styles (0.7KB)
└── popper.min.js          # Positioning library (20KB)
```

## Tips

- Keep console.log statements in your code - Terser preserves them for debugging
- The build process shows file sizes to help monitor optimization
- Static assets (Font Awesome, Tippy.js, webfonts) are copied as-is since they're already optimized
- The data/ folder has been optimized to ~320KB total (21% reduction from original 404KB)
- Only essential files are included - unused development files and fonts were removed

## Filesystem Optimization

The ESP32 filesystem has been optimized for faster development uploads:

- **Removed unused files**: `script.js`, `style.css`, `README.md` from data/ folder
- **Removed unused fonts**: Only `fa-solid-900.woff2` kept (interface only uses solid Font Awesome icons)
- **Total size**: ~320KB (down from 404KB = 21% reduction)
- **Upload time**: Significantly faster development iterations

The build process ensures only production-optimized files are included.

## Troubleshooting

If build fails:
1. Ensure Node.js and npm are installed
2. Run `npm install` to install dependencies
3. Check that source files exist in this folder

For ESP32 upload issues, ensure the device is connected and PlatformIO is configured correctly.