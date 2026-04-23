import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function useClients() {
  async function load() {
    items.value = await db.clients.orderBy('name').toArray()
  }

  async function get(id) {
    return db.clients.get(id)
  }

  async function create(data) {
    const id = await db.clients.add({ ...data, hids: [] })
    await load()
    return id
  }

  async function update(id, data) {
    await db.clients.update(id, data)
    await load()
  }

  async function remove(id) {
    await db.clients.delete(id)
    await load()
  }

  async function isInUse(id) {
    const controllers = await db.controllers.toArray()
    const assignedAsInput = controllers.some(c =>
      Object.values(c.inputs || {}).includes(id)
    )
    if (assignedAsInput) return true

    const extendedCount = await db.clients.where('extends').equals(id).count()
    return extendedCount > 0
  }

  async function getExtendedClientIds() {
    const all = await db.clients.orderBy('name').toArray()
    return all.filter(c => c.extends !== undefined).map(c => c.extends)
  }

  return { items, load, get, create, update, remove, isInUse, getExtendedClientIds }
}
