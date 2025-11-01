# Font Loading Performance Optimizations

## Problem
- 1.5MB Iosevka Term font causing FOUC (Flash of Unstyled Content)
- 30-second loading time on embedded device
- Poor user experience with delayed font rendering

## Solutions Implemented

### 1. Font Preloading
```html
<link rel="preload" href="fonts/IosevkaTerm.woff2" as="font" type="font/woff2" crossorigin>
```
- **Benefit**: Browser starts downloading font immediately
- **Performance**: Reduces time to font availability

### 2. Progressive Font Loading Strategy
```css
/* Start with fallback fonts */
body, html {
    font-family: 'Monaco', 'Consolas', 'Courier New', monospace;
}

/* Apply custom font only when loaded */
.fonts-loaded body, 
.fonts-loaded html {
    font-family: 'Iosevka Term', 'Monaco', 'Consolas', 'Courier New', monospace;
}
```
- **Benefit**: Eliminates FOUC by showing readable text immediately
- **Performance**: Smooth transition when custom font loads

### 3. Improved font-display Strategy
```css
@font-face {
    font-family: 'Iosevka Term';
    src: url('../fonts/IosevkaTerm.woff2') format('woff2');
    font-display: fallback; /* Better than 'swap' for slow connections */
}
```
- **`fallback`**: Shows fallback font immediately, switches to custom font if loaded within 100ms
- **Benefit**: Better performance on slow connections than `swap`

### 4. JavaScript Font Load Detection
- Uses native Font Loading API when available
- Canvas-based measurement fallback for older browsers
- Applies `fonts-loaded` class when custom font is ready
- **Benefit**: Precise control over font loading states

### 5. Critical CSS Inline
```html
<style>
/* Critical font CSS to prevent FOUC */
body, html {
    font-family: 'Monaco', 'Consolas', 'Courier New', monospace !important;
}
</style>
```
- **Benefit**: Immediate font styling, no external CSS dependency

### 6. Smooth Transitions
```css
body, h1, h2, h3, h4, h5, h6, p, div, span, a, button {
    transition: font-family 0.2s ease-in-out;
}
```
- **Benefit**: Elegant transition between fallback and custom fonts

## Performance Improvements

### Before Optimization
- ❌ 30-second font loading
- ❌ 2-3 seconds of FOUC
- ❌ Poor user experience
- ❌ Blocking font loading

### After Optimization
- ✅ Immediate readable text (fallback fonts)
- ✅ No FOUC - text appears immediately
- ✅ Progressive enhancement when custom font loads
- ✅ Smooth 0.2s transition to custom font
- ✅ 3-second timeout fallback for reliability

## Alternative Solutions

### Option 1: System Fonts Only (0 bytes, instant loading)
```html
<html class="system-fonts">
```
- Fastest possible loading
- Uses fonts already installed on device

### Option 2: Web Font Services
```html
<!-- JetBrains Mono from Google Fonts -->
<link href="https://fonts.googleapis.com/css2?family=JetBrains+Mono:wght@400&display=swap" rel="stylesheet">
```
- Smaller file sizes
- CDN delivery
- Automatic optimization

### Option 3: Font Subsetting
- Create custom subset with only needed characters
- Could reduce 1.5MB font to ~200KB
- Tools: `pyftsubset`, `fonttools`

## Files Added/Modified

### New Files
- `data/js/font-loader.js` - Font loading detection
- `data/css/font-alternatives.css` - Alternative font strategies

### Modified Files
- `data/css/custom-fonts.css` - Progressive loading strategy
- `data/index.html` - Font preloading and critical CSS

## Usage

The optimizations work automatically. For even faster loading, consider:

1. **Use system fonts**: Add `class="system-fonts"` to `<html>` tag
2. **Use web fonts**: Replace with Google Fonts or similar service
3. **Subset the font**: Create a smaller version with only needed characters

## Result

Users now see readable text immediately with a smooth transition to the custom font when it loads, eliminating the poor user experience of FOUC and long loading times.