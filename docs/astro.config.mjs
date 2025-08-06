// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';
import Icons from 'unplugin-icons/vite'

import tailwindcss from '@tailwindcss/vite';

// https://astro.build/config
export default defineConfig({
  site: 'https://gregoryw3.github.io',
  base: '/Pico-FreeRTOS',
  integrations: [
      starlight({
          title: 'Pico-FreeRTOS',
          description: 'Embedded Systems Library for Raspberry Pi Pico with FreeRTOS',
          social: [
            {
              icon: 'github',
              label: 'GitHub',
              href: 'https://github.com/gregoryw3/pico-freertos'
            }
          ],
          sidebar: [
              {
                  label: 'Getting Started',
                  items: [
                      { label: 'Introduction', slug: 'getting-started/introduction' },
                  ],
              },
              {
                  label: 'GPS & Navigation',
                  items: [
                        { label: 'u-blox Overview', slug: 'gps/ublox' },
                        { label: 'NMEA Protocol', slug: 'gps/nmea' },
                        { label: 'UBX Protocol', slug: 'gps/ubx' },
                  ],
              },
              {
                  label: 'Sensor Libraries',
                  items: [
                      { label: 'Sensor Overview', slug: 'sensors/overview' },
                  ],
              },
              {
                  label: 'Communication',
                  items: [
                      { label: 'Bluetooth Setup', slug: 'communication/bluetooth' },
                  ],
              },
              {
                  label: 'Examples & Tutorials',
                  items: [
                      { label: 'Multi-Task System', slug: 'examples/multitask' },
                  ],
              },
              {
                  label: 'API Reference',
                  autogenerate: { directory: 'reference' },
              },
          ],
          customCss: [
            './src/styles/custom.css',
          ],
      }),
	],

  vite: {
    plugins: [
        Icons({ compiler: 'astro' }),
        tailwindcss(),
    ],
  },
});