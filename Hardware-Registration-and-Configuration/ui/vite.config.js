import { defineConfig } from 'vite'
import vue from '@vitejs/plugin-vue'
import { readFileSync } from 'fs'
import { resolve } from 'path'
import { fileURLToPath } from 'url'
import yaml from 'js-yaml'

const __dirname = fileURLToPath(new URL('.', import.meta.url))

const devicesYaml = yaml.load(readFileSync(resolve(__dirname, '../../devices.yaml'), 'utf8'))
const activeDevices = devicesYaml.devices.filter(d => d.status === 'ACTIVE')

export default defineConfig({
  plugins: [vue()],
  define: {
    __DEVICES__: JSON.stringify(activeDevices)
  },
  build: {
    outDir: '../www',
    emptyOutDir: true
  }
})
