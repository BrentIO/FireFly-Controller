#!/usr/bin/env node
// Reads devices.yaml from the repo root and updates the defaultControllerProducts
// array in Controller/ui/src/composables/useDatabase.js in place.
// Includes ACTIVE and RETIRED devices; excludes INACTIVE.
// Run from Controller/ui as a prebuild step: node ../../scripts/gen-products.mjs

import { readFileSync, writeFileSync } from 'fs'
import { fileURLToPath } from 'url'
import { dirname, resolve } from 'path'

const __dirname = dirname(fileURLToPath(import.meta.url))
const repoRoot = resolve(__dirname, '..')

const yaml = readFileSync(resolve(repoRoot, 'devices.yaml'), 'utf8')

// Split into per-device sections (each starts with "- product_hex:")
const sections = yaml.split(/(?=\n\s{2}-\s+product_hex:)/).filter(s => s.includes('product_id:'))

const products = sections
  .map(section => {
    const pid = (section.match(/product_id:\s*(\S+)/) ?? [])[1]
    const status = (section.match(/status:\s*(\S+)/) ?? [])[1]
    const inputs = parseInt((section.match(/inputs_count:\s*(\d+)/) ?? [])[1] ?? '0', 10)
    const outputs = parseInt((section.match(/outputs_count:\s*(\d+)/) ?? [])[1] ?? '0', 10)
    return { pid, status, inputs, outputs }
  })
  .filter(d => d.pid && (d.status === 'ACTIVE' || d.status === 'RETIRED'))
  .map(d => ({ pid: d.pid, inputs: { count: d.inputs }, outputs: { count: d.outputs } }))
  .sort((a, b) => a.pid.localeCompare(b.pid))

const arrayLines = products.map(p =>
  `  { pid: '${p.pid}', inputs: { count: ${p.inputs.count} }, outputs: { count: ${p.outputs.count} } }`
).join(',\n')

const replacement = `const defaultControllerProducts = [\n${arrayLines}\n]`

const dbPath = resolve(repoRoot, 'Controller/ui/src/composables/useDatabase.js')
const original = readFileSync(dbPath, 'utf8')
const updated = original.replace(
  /const defaultControllerProducts = \[[\s\S]*?\]/,
  replacement
)

if (!/const defaultControllerProducts = \[[\s\S]*?\]/.test(original)) {
  console.error('ERROR: Could not find defaultControllerProducts array in useDatabase.js')
  process.exit(1)
}

writeFileSync(dbPath, updated)
console.log(`Updated defaultControllerProducts with ${products.length} products in useDatabase.js`)
products.forEach(p => console.log(`  ${p.pid} (${p.inputs.count} in / ${p.outputs.count} out)`))
