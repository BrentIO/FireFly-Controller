import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])
const relayModels = ref([])

export function useCircuits() {
  async function load() {
    items.value = await db.circuits.orderBy('name').toArray()
    relayModels.value = await db.relay_models.toArray()
  }

  async function create(data) {
    const id = await db.circuits.add(JSON.parse(JSON.stringify(data)))
    await load()
    return id
  }

  async function update(id, data) {
    await db.circuits.update(id, JSON.parse(JSON.stringify(data)))
    await load()
  }

  async function remove(id) {
    await db.circuits.delete(id)
    await deleteUnusedCustomRelayModels()
    await load()
  }

  async function createRelayModel(data) {
    return db.relay_models.add({ ...data, is_custom: 'true' })
  }

  async function deleteUnusedCustomRelayModels() {
    const customs = await db.relay_models.where('is_custom').equals('true').toArray()
    for (const rm of customs) {
      const count = await db.circuits.where('relay_model').equals(rm.id).count()
      if (count === 0) await db.relay_models.delete(rm.id)
    }
  }

  async function isInUse(id) {
    const controllers = await db.controllers.toArray()
    const assignedInControllers = controllers.some(c =>
      Object.values(c.outputs || {}).includes(id)
    )
    if (assignedInControllers) return true

    const clients = await db.clients.toArray()
    return clients.some(c =>
      c.hids?.some(h => h.actions?.some(a => a.circuit === id))
    )
  }

  return { items, relayModels, load, create, update, remove, createRelayModel, isInUse }
}
