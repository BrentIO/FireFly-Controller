import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])
const products = ref([])

export function useControllers() {
  async function load() {
    items.value = await db.controllers.orderBy('name').toArray()
    products.value = await db.controller_products.toArray()
  }

  async function create(data) {
    const id = await db.controllers.add({ ...data, inputs: {}, outputs: {} })
    await load()
    return id
  }

  async function update(id, data) {
    await db.controllers.update(id, data)
    await load()
  }

  async function remove(id) {
    await db.controllers.delete(id)
    await load()
  }

  async function assignInput(controllerId, port, clientId) {
    const controller = await db.controllers.get(controllerId)
    const inputs = { ...controller.inputs }
    if (clientId === null) {
      delete inputs[port]
    } else {
      inputs[port] = clientId
    }
    await db.controllers.update(controllerId, { inputs })
    await load()
  }

  async function assignOutput(controllerId, port, circuitId) {
    const controller = await db.controllers.get(controllerId)
    const outputs = { ...controller.outputs }
    if (circuitId === null) {
      delete outputs[port]
    } else {
      outputs[port] = circuitId
    }
    await db.controllers.update(controllerId, { outputs })
    await load()
  }

  async function isInUse(id) {
    const controller = await db.controllers.get(id)
    if (!controller) return false
    return (
      Object.keys(controller.outputs || {}).length > 0 ||
      Object.keys(controller.inputs || {}).length > 0
    )
  }

  return { items, products, load, create, update, remove, assignInput, assignOutput, isInUse }
}
