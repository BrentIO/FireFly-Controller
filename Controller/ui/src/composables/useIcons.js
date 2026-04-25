import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function useIcons() {
  async function load() {
    items.value = await db.circuit_icons.orderBy('name').toArray()
  }

  async function create(data) {
    const id = await db.circuit_icons.add(JSON.parse(JSON.stringify(data)))
    await load()
    return id
  }

  async function update(id, data) {
    await db.circuit_icons.update(id, JSON.parse(JSON.stringify(data)))
    await load()
  }

  async function remove(id) {
    await db.circuit_icons.delete(id)
    await load()
  }

  async function isInUse(id) {
    const count = await db.circuits.where('icon').equals(id).count()
    return count > 0
  }

  return { items, load, create, update, remove, isInUse }
}
