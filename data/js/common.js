import { initThemeSync, bindThemeControl } from './theme.js';
import { currentThemeState } from './theme.js';

function setActiveNavLink(pathname) {
    document.querySelectorAll('[data-nav]')
        .forEach((link) => {
            if (link.dataset.nav === pathname) {
                link.classList.add('active');
                link.setAttribute('aria-current', 'page');
            } else {
                link.classList.remove('active');
                link.removeAttribute('aria-current');
            }
        });
}

function wireThemeIndicator() {
    const indicator = document.querySelector('[data-theme-indicator]');
    if (!indicator) {
        return;
    }

    const updateIndicator = ({ theme, override }) => {
        const label = override ? `${theme} (override)` : `${theme} (system)`;
        indicator.textContent = label;
    };

    const initial = currentThemeState();
    updateIndicator({ theme: initial.theme, override: Boolean(initial.override) });

    document.documentElement.addEventListener('themechange', (event) => {
        updateIndicator(event.detail);
    });
}

export function initPage({ activeNav } = {}) {
    initThemeSync();

    const themeSelector = document.querySelector('[data-theme-toggle]');
    if (themeSelector) {
        bindThemeControl(themeSelector);
    }

    wireThemeIndicator();

    if (activeNav) {
        setActiveNavLink(activeNav);
    }
}

export function renderAlert(target, message, type = 'danger') {
    if (!target) {
        return;
    }
    target.innerHTML = `
        <div class="alert alert-${type} alert-dismissible fade show" role="alert">
            ${message}
            <button type="button" class="btn-close" data-bs-dismiss="alert" aria-label="Close"></button>
        </div>`;
}
