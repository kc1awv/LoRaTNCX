/**
 * Theme Switcher for LoRaTNCX Web Interface
 * Handles light/dark theme switching with browser preference detection
 */

class ThemeSwitcher {
    constructor() {
        this.themes = ['auto', 'light', 'dark'];
        this.currentTheme = this.getStoredTheme() || 'auto';
        this.mediaQuery = window.matchMedia('(prefers-color-scheme: dark)');
        
        this.init();
    }

    init() {
        // Apply initial theme
        this.applyTheme(this.currentTheme);
        
        // Set up event listeners
        this.setupEventListeners();
        
        // Listen for system theme changes
        this.mediaQuery.addEventListener('change', () => {
            if (this.currentTheme === 'auto') {
                this.applySystemTheme();
            }
        });
        
        // Update theme toggle button
        this.updateThemeToggle();
    }

    setupEventListeners() {
        // Theme switcher dropdown items
        document.querySelectorAll('[data-theme-switcher]').forEach(item => {
            item.addEventListener('click', (e) => {
                e.preventDefault();
                const theme = e.target.getAttribute('data-theme-switcher');
                this.setTheme(theme);
            });
        });
    }

    setTheme(theme) {
        if (!this.themes.includes(theme)) return;
        
        this.currentTheme = theme;
        this.storeTheme(theme);
        this.applyTheme(theme);
        this.updateThemeToggle();
        
        // Dispatch custom event for other components
        document.dispatchEvent(new CustomEvent('themeChanged', {
            detail: { theme: theme, effectiveTheme: this.getEffectiveTheme() }
        }));
    }

    applyTheme(theme) {
        const html = document.documentElement;
        
        // Remove existing theme attributes
        html.removeAttribute('data-theme');
        
        switch (theme) {
            case 'light':
                html.setAttribute('data-theme', 'light');
                break;
            case 'dark':
                html.setAttribute('data-theme', 'dark');
                break;
            case 'auto':
            default:
                this.applySystemTheme();
                break;
        }
    }

    applySystemTheme() {
        const html = document.documentElement;
        const prefersDark = this.mediaQuery.matches;
        
        html.removeAttribute('data-theme');
        if (prefersDark) {
            // Let Pico.css handle the dark theme automatically
            // The CSS media query will apply the dark theme
        } else {
            html.setAttribute('data-theme', 'light');
        }
    }

    getEffectiveTheme() {
        if (this.currentTheme === 'auto') {
            return this.mediaQuery.matches ? 'dark' : 'light';
        }
        return this.currentTheme;
    }

    updateThemeToggle() {
        const toggleButton = document.getElementById('theme-toggle');
        if (!toggleButton) return;

        const icons = {
            auto: '🔄',
            light: '☀️',
            dark: '🌙'
        };

        const effectiveTheme = this.getEffectiveTheme();
        const displayIcon = this.currentTheme === 'auto' ? '🔄' : icons[effectiveTheme];
        
        toggleButton.innerHTML = `${displayIcon} Theme`;
        
        // Update active state in dropdown
        document.querySelectorAll('[data-theme-switcher]').forEach(item => {
            const itemTheme = item.getAttribute('data-theme-switcher');
            if (itemTheme === this.currentTheme) {
                item.style.fontWeight = 'bold';
                item.setAttribute('aria-current', 'true');
            } else {
                item.style.fontWeight = 'normal';
                item.removeAttribute('aria-current');
            }
        });
    }

    storeTheme(theme) {
        try {
            localStorage.setItem('loratncx-theme', theme);
        } catch (e) {
            console.warn('Could not store theme preference:', e);
        }
    }

    getStoredTheme() {
        try {
            return localStorage.getItem('loratncx-theme');
        } catch (e) {
            console.warn('Could not retrieve theme preference:', e);
            return null;
        }
    }

    // Public API
    getCurrentTheme() {
        return this.currentTheme;
    }

    getAvailableThemes() {
        return [...this.themes];
    }

    isDarkMode() {
        return this.getEffectiveTheme() === 'dark';
    }
}

// Initialize theme switcher when DOM is ready
document.addEventListener('DOMContentLoaded', () => {
    window.themeSwitcher = new ThemeSwitcher();
    
    // Expose theme info for debugging
    if (typeof window !== 'undefined' && window.console) {
        console.log('🎨 Theme Switcher initialized');
        console.log('Current theme:', window.themeSwitcher.getCurrentTheme());
        console.log('Effective theme:', window.themeSwitcher.getEffectiveTheme());
        console.log('System prefers dark:', window.matchMedia('(prefers-color-scheme: dark)').matches);
    }
});

// Handle theme changes for dynamic content
document.addEventListener('themeChanged', (e) => {
    // Update any theme-dependent elements
    console.log('Theme changed to:', e.detail.theme, '(effective:', e.detail.effectiveTheme + ')');
});

// Export for module usage (if needed)
if (typeof module !== 'undefined' && module.exports) {
    module.exports = ThemeSwitcher;
}