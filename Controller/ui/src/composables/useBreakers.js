import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function useBreakers() {
  async function load() {
    items.value = await db.breakers.orderBy('name').toArray()
  }

  async function create(data) {
    const id = await db.breakers.add(JSON.parse(JSON.stringify(data)))
    await load()
    return id
  }

  async function update(id, data) {
    await db.breakers.update(id, JSON.parse(JSON.stringify(data)))
    await load()
  }

  async function remove(id) {
    await db.breakers.delete(id)
    await load()
  }

  async function isInUse(id) {
    const count = await db.circuits.where('breaker').equals(id).count()
    return count > 0
  }

  async function getUtilization(id) {
    const circuits = await db.circuits.where('breaker').equals(id).toArray()
    const load = circuits.reduce((sum, c) => sum + Number(c.load_amperage || 0), 0)
    const breaker = await db.breakers.get(id)
    return { load, amperage: breaker?.amperage ?? 0 }
  }

  return { items, load, create, update, remove, isInUse, getUtilization }
}
