import Dexie from 'dexie'
import { controllerProducts } from '../generated/controller_products.js'

export const db = new Dexie('FireFly-Controller')

const defaultColors = [
  { name: 'Blue', hex: '#2a36e5' },
  { name: 'Green', hex: '#46a046' },
  { name: 'Red', hex: '#db1a1a' },
  { name: 'White', hex: '#ffffff' },
  { name: 'Yellow', hex: '#ffbc47' }
]

const defaultRelayModels = [
  { type: 'BINARY', manufacturer: 'Crydom', model: 'DR2260D20V', description: 'Contactor' },
  { type: 'BINARY', manufacturer: 'Crydom', model: 'DR2220D20U', description: 'Relay' },
  { type: 'VARIABLE', manufacturer: 'Crydom', model: 'PMP2425W', description: 'Proportional Control' }
]

const defaultCircuitIcons = [
  { name: 'Ceiling Fan', icon: 'mdi:ceiling-fan' },
  { name: 'Ceiling Fan Light', icon: 'mdi:ceiling-fan-light' },
  { name: 'Chandelier', icon: 'mdi:chandelier' },
  { name: 'Exhaust Fan', icon: 'mdi:fan' },
  { name: 'Floor Lamp', icon: 'mdi:floor-lamp' },
  { name: 'Power Socket', icon: 'mdi:power-socket-us' },
  { name: 'Pump', icon: 'mdi:pump' },
  { name: 'Recessed Light', icon: 'mdi:light-recessed' },
  { name: 'Vanity Light', icon: 'mdi:vanity-light' },
  { name: 'Wall Sconce', icon: 'mdi:wall-sconce' }
]

const additionalDefaultCircuitIcons = [
  { name: 'Ceiling Fan', icon: 'mdi:ceiling-fan' },
  { name: 'Ceiling Fan Light', icon: 'mdi:ceiling-fan-light' },
  { name: 'Exhaust Fan', icon: 'mdi:fan' },
  { name: 'Power Socket', icon: 'mdi:power-socket-us' },
  { name: 'Pump', icon: 'mdi:pump' },
  { name: 'Vanity Light', icon: 'mdi:vanity-light' }
]

const defaultAreas = [
  { name: 'Living Room' },
  { name: 'Garage' },
  { name: 'Kitchen' },
  { name: 'Hallway' },
  { name: 'Primary Bedroom' },
  { name: 'Guest Bedroom' }
]

const schema = {
  colors: '++id, name',
  controllers: '++id, &uuid, name, product',
  areas: '++id, name',
  circuit_icons: '++id, name',
  controller_products: '++id, pid',
  tags: '++id, &name',
  certificates: '++id, &certificate, commonName',
  breakers: '++id, name',
  circuits: '++id, name, breaker, area, icon, relay_model',
  relay_models: '++id, [manufacturer+model], is_custom',
  clients: '++id, name, &uuid',
  settings: '&setting, value'
}

db.version(20240405).stores(schema)

db.version(20260520).stores(schema).upgrade(async (tx) => {
  const existing = await tx.circuit_icons.toArray()
  const existingNames = new Set(existing.map(i => i.name))
  const toAdd = additionalDefaultCircuitIcons.filter(i => !existingNames.has(i.name))
  if (toAdd.length > 0) await tx.circuit_icons.bulkAdd(toAdd)
})

db.version(20260521).stores(schema).upgrade(async (tx) => {
  await tx.controller_products.clear()
  await tx.controller_products.bulkAdd(controllerProducts)
})

db.on('populate', (transaction) => {
  transaction.controller_products.bulkAdd(controllerProducts)
  transaction.colors.bulkAdd(defaultColors)
  transaction.relay_models.bulkAdd(defaultRelayModels)
  transaction.circuit_icons.bulkAdd(defaultCircuitIcons)
  transaction.areas.bulkAdd(defaultAreas)
})

db.open()
