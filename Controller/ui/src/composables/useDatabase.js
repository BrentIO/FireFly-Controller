import Dexie from 'dexie'

const dbVersion = 20240405

export const db = new Dexie('FireFly-Controller')

const defaultControllerProducts = [
  { pid: 'FFC3232-2305', inputs: { count: 32 }, outputs: { count: 32 } },
  { pid: 'FFC0806-2305', inputs: { count: 8 }, outputs: { count: 6 } },
  { pid: 'FFC3232-2505', inputs: { count: 32 }, outputs: { count: 32 } },
  { pid: 'FFC0806-2505', inputs: { count: 8 }, outputs: { count: 6 } }
]

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
  { name: 'Chandelier', icon: 'mdi:chandelier' },
  { name: 'Recessed Light', icon: 'mdi:light-recessed' },
  { name: 'Floor Lamp', icon: 'mdi:floor-lamp' },
  { name: 'Wall Sconce', icon: 'mdi:wall-sconce' }
]

const defaultAreas = [
  { name: 'Living Room' },
  { name: 'Garage' },
  { name: 'Kitchen' },
  { name: 'Hallway' },
  { name: 'Primary Bedroom' },
  { name: 'Guest Bedroom' }
]

db.version(dbVersion).stores({
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
})

db.on('populate', (transaction) => {
  transaction.controller_products.bulkAdd(defaultControllerProducts)
  transaction.colors.bulkAdd(defaultColors)
  transaction.relay_models.bulkAdd(defaultRelayModels)
  transaction.circuit_icons.bulkAdd(defaultCircuitIcons)
  transaction.areas.bulkAdd(defaultAreas)
})

db.open()
