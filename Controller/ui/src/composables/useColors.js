import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function useColors() {
  async function load() {
    items.value = await db.colors.orderBy('name').toArray()
  }

  async function create(data) {
    const id = await db.colors.add(JSON.parse(JSON.stringify(data)))
    await load()
    return id
  }

  async function update(id, data) {
    await db.colors.update(id, JSON.parse(JSON.stringify(data)))
    await load()
  }

  async function remove(id) {
    await db.colors.delete(id)
    await load()
  }

  return { items, load, create, update, remove }
}
