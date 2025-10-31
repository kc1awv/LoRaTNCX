// Service Worker for LoRaTNCX Web Interface
const CACHE_NAME = 'loraTNCX-v2.0';
const STATIC_CACHE_URLS = [
  '/',
  '/index.html',
  '/status.html',
  '/config.html',
  '/console.html',
  '/pair.html',
  '/css/bootstrap.css',
  '/css/modern-theme.css',
  '/js/bootstrap.js',
  '/js/api.js',
  '/js/common.js',
  '/js/index-modern.js',
  '/js/status.js',
  '/js/config.js',
  '/js/console.js',
  '/js/pair.js',
  '/js/theme.js',
  '/js/websocket.js',
  '/manifest.json'
];

// Install event - cache static resources
self.addEventListener('install', event => {
  console.log('SW: Installing service worker');
  event.waitUntil(
    caches.open(CACHE_NAME)
      .then(cache => {
        console.log('SW: Caching static resources');
        return cache.addAll(STATIC_CACHE_URLS);
      })
      .then(() => {
        console.log('SW: Static resources cached');
        return self.skipWaiting();
      })
      .catch(error => {
        console.error('SW: Failed to cache static resources:', error);
      })
  );
});

// Activate event - clean up old caches
self.addEventListener('activate', event => {
  console.log('SW: Activating service worker');
  event.waitUntil(
    caches.keys()
      .then(cacheNames => {
        return Promise.all(
          cacheNames
            .filter(cacheName => cacheName !== CACHE_NAME)
            .map(cacheName => {
              console.log('SW: Deleting old cache:', cacheName);
              return caches.delete(cacheName);
            })
        );
      })
      .then(() => {
        console.log('SW: Service worker activated');
        return self.clients.claim();
      })
  );
});

// Fetch event - serve from cache, fallback to network
self.addEventListener('fetch', event => {
  const { request } = event;
  const url = new URL(request.url);

  // Handle API requests differently
  if (url.pathname.startsWith('/api/') || url.pathname.startsWith('/ws')) {
    // For API and WebSocket requests, always try network first
    event.respondWith(
      fetch(request)
        .catch(() => {
          // Return offline message for API failures
          if (request.headers.get('accept')?.includes('application/json')) {
            return new Response(
              JSON.stringify({ 
                error: 'Device offline', 
                offline: true 
              }),
              {
                status: 503,
                statusText: 'Service Unavailable',
                headers: { 'Content-Type': 'application/json' }
              }
            );
          }
          return new Response('Service unavailable', { status: 503 });
        })
    );
    return;
  }

  // For static resources, try cache first, then network
  event.respondWith(
    caches.match(request)
      .then(cachedResponse => {
        if (cachedResponse) {
          console.log('SW: Serving from cache:', request.url);
          return cachedResponse;
        }

        console.log('SW: Fetching from network:', request.url);
        return fetch(request)
          .then(response => {
            // Don't cache non-successful responses
            if (!response || response.status !== 200 || response.type !== 'basic') {
              return response;
            }

            // Clone the response for caching
            const responseToCache = response.clone();
            
            caches.open(CACHE_NAME)
              .then(cache => {
                cache.put(request, responseToCache);
              });

            return response;
          })
          .catch(() => {
            // Return a basic offline page for navigation requests
            if (request.mode === 'navigate') {
              return caches.match('/index.html');
            }
            return new Response('Resource not available offline', { 
              status: 404,
              statusText: 'Not Found' 
            });
          });
      })
  );
});

// Background sync for queued actions
self.addEventListener('sync', event => {
  console.log('SW: Background sync triggered:', event.tag);
  
  if (event.tag === 'config-sync') {
    event.waitUntil(syncQueuedConfigs());
  }
});

async function syncQueuedConfigs() {
  try {
    // Get queued configurations from IndexedDB
    // This would need to be implemented based on your specific needs
    console.log('SW: Syncing queued configurations');
  } catch (error) {
    console.error('SW: Failed to sync queued configurations:', error);
  }
}

// Push notifications (for future use)
self.addEventListener('push', event => {
  console.log('SW: Push notification received');
  
  const options = {
    body: 'Your LoRaTNCX device needs attention',
    icon: '/icon-192.png',
    badge: '/icon-192.png',
    vibrate: [200, 100, 200],
    data: {
      dateOfArrival: Date.now(),
      primaryKey: 1
    },
    actions: [
      {
        action: 'explore',
        title: 'Open Interface',
        icon: '/icon-192.png'
      },
      {
        action: 'close',
        title: 'Close',
        icon: '/icon-192.png'
      }
    ]
  };

  event.waitUntil(
    self.registration.showNotification('LoRaTNCX', options)
  );
});

// Notification click handler
self.addEventListener('notificationclick', event => {
  console.log('SW: Notification clicked');
  event.notification.close();

  if (event.action === 'explore') {
    event.waitUntil(
      clients.openWindow('/')
    );
  }
});