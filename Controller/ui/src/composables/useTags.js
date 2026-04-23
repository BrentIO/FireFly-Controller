import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function useTags() {
  async function load() {
    items.value = await db.tags.orderBy('name').toArray()
  }

  async function create(data) {
    const id = await db.tags.add(data)
    await load()
    return id
  }

  async function update(id, data) {
    await db.tags.update(id, data)
    await load()
  }

  async function remove(id) {
    await db.tags.delete(id)
    await load()
  }

  async function isInUse(id) {
    const clients = await db.clients.toArray()
    return clients.some(c => c.hids?.some(h => h.tags?.includes(id)))
  }

  return { items, load, create, update, remove, isInUse }
}
