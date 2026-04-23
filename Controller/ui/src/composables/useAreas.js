import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function useAreas() {
  async function load() {
    items.value = await db.areas.orderBy('name').toArray()
  }

  async function create(data) {
    const id = await db.areas.add(data)
    await load()
    return id
  }

  async function update(id, data) {
    await db.areas.update(id, data)
    await load()
  }

  async function remove(id) {
    await db.areas.delete(id)
    await load()
  }

  async function isInUse(id) {
    const count = await db.circuits.where('area').equals(id).count()
    return count > 0
  }

  return { items, load, create, update, remove, isInUse }
}
