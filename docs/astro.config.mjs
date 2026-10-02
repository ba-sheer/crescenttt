import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

export default defineConfig({
  site: 'https://ba-sheer.github.io',
  base: '/crescenttt',

  integrations: [
    starlight({
      title: 'Crescent TUI',
      description: 'Documentation for Crescent TUI',
      social: {
        github: 'https://github.com/ba-sheer/crescenttt',
      },
    }),
  ],
});