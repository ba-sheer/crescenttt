import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  site: 'https://ba-sheer.github.io',
  base: '/crescenttt',

  integrations: [
    starlight({
      title: 'Crescent TUI',
      description: 'Documentation for Crescent TUI',

      social: [
        {
          icon: 'github',
          label: 'GitHub',
          href: 'https://github.com/ba-sheer/crescenttt',
        },
      ],

      sidebar: [
        {
          label: 'Crescent TUI',
          items: ['index'],
        },
        {
          label: 'Getting Started',
          items: [
            'getting-started/installation',
            'getting-started/configuration',
            'getting-started/api-key',
          ],
        },
        {
          label: 'Usage',
          items: [
            'usage/projects',
            'usage/orders',
            'usage/notifications',
            'usage/announcements',
          ],
        },
        {
          label: 'Development',
          items: [
            'development/building',
            'development/linux',
            'development/windows',
          ],
        },
      ],
    }),
  ],
});