import { defineConfig } from 'vitepress'

// https://vitepress.dev/reference/site-config
export default defineConfig({
  title: "FireFly Controller",
  description: "Software-defined lighting",
  base: '/FireFly-Controller/',
  themeConfig: {
    // https://vitepress.dev/reference/default-theme-config
    nav: [
      { text: 'Home', link: '/' }
    ],

    sidebar: [
      {
        text: 'Getting Started', link: '/getting_started#features'
      },
      {
        text: 'Hardware', link: '/hardware',
        items: [
          { text: 'Reference Designs (TODO)' },
          { text: '3D Printed Cases (TODO)'},
          { text: 'High Voltage Relays', link: 'relays'},

        ]
      },
      {
        text: 'Software',
        items: [
          { text: 'Hardware Registration and Configuration Firmware', link: 'https://github.com/BrentIO/FireFly-Controller/tree/main/Hardware-Registration-and-Configuration' },
          { text: 'Controller Firmware', link: 'https://github.com/BrentIO/FireFly-Controller/tree/main/Controller' }
        ]
      },
      {
        text: 'Documentation and Support',
        items: [
          { text: 'Abbreviations' },
          { text: 'OLED Screens' }
        ]
      },
      {
        text: 'Development Environment',
        items: [
          { text: 'IDE Configuration', link: '/ide_configuration' }
        ]
      },
    ],

    socialLinks: [
      { icon: 'github', link: 'https://github.com/BrentIO/FireFly-Controller' }
    ]
  }
})
