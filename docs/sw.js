// Pulsar C6 - Service Worker
const CACHE_NAME = 'pulsar-c6-v9';

// Solo assets estaticos (NO index.html ni styles.css para que siempre lleguen frescos de red)
const ASSETS = [
  './script-pulsar-c6-ble.js',
  './manifest.json',
  './icon-192.png',
  './icon-512.png'
];

// Instalar: cachear assets estáticos
self.addEventListener('install', event => {
  event.waitUntil(
    caches.open(CACHE_NAME).then(cache => cache.addAll(ASSETS))
  );
  self.skipWaiting();
});

// Activar: limpiar caches viejos
self.addEventListener('activate', event => {
  event.waitUntil(
    caches.keys().then(keys =>
      Promise.all(
        keys.filter(k => k !== CACHE_NAME).map(k => caches.delete(k))
      )
    )
  );
  self.clients.claim();
});

// Fetch:
// - Navegaciones/HTML: network-first para reflejar cambios rapido.
// - Assets: cache-first con fallback a red.
self.addEventListener('fetch', event => {
  // No interceptar peticiones BLE ni externas (Tailwind CDN, Google Fonts)
  const url = new URL(event.request.url);
  if (url.origin !== self.location.origin) return;

  const isNavigation = event.request.mode === 'navigate';

  if (isNavigation) {
    event.respondWith(
      fetch(event.request)
        .then(response => {
          if (response && response.status === 200 && response.type === 'basic') {
            const clone = response.clone();
            caches.open(CACHE_NAME).then(cache => cache.put(event.request, clone));
          }
          return response;
        })
        .catch(() => caches.match(event.request))
    );
    return;
  }

  event.respondWith(
    caches.match(event.request).then(cached => {
      if (cached) return cached;
      return fetch(event.request).then(response => {
        // Solo cachear respuestas válidas de nuestro origen
        if (response && response.status === 200 && response.type === 'basic') {
          const clone = response.clone();
          caches.open(CACHE_NAME).then(cache => cache.put(event.request, clone));
        }
        return response;
      }).catch(() => cached);
    })
  );
});
