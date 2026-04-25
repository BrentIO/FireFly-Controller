import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function useRelayModels() {
  async function load() {
    items.value = await db.relay_models.toArray()
  }

  async function create(data) {
    const id = await db.relay_models.add(JSON.parse(JSON.stringify({ ...data, is_custom: 'true' })))
    await load()
    return id
  }

  async function update(id, data) {
    await db.relay_models.update(id, JSON.parse(JSON.stringify(data)))
    await load()
  }

  async function remove(id) {
    await db.relay_models.delete(id)
    await load()
  }

  async function isInUse(id) {
    const count = await db.circuits.where('relay_model').equals(id).count()
    return count > 0
  }

  return { items, load, create, update, remove, isInUse }
}
