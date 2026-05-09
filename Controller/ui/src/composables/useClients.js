import { ref } from 'vue'
import { db } from './useDatabase'
import { randomUUID } from './useValidators'

const items = ref([])

export function useClients() {
  async function load() {
    items.value = await db.clients.orderBy('name').toArray()
  }

  async function get(id) {
    return db.clients.get(id)
  }

  async function create(data) {
    const id = await db.clients.add(JSON.parse(JSON.stringify({ ...data, hids: [] })))
    await load()
    return id
  }

  async function update(id, data) {
    await db.clients.update(id, JSON.parse(JSON.stringify(data)))
    // If name or description is being updated and this client has an extends, sync secondary
    if (data.name !== undefined || data.description !== undefined) {
      const client = await db.clients.get(id)
      if (client?.extends) {
        const patch = {}
        if (data.name !== undefined) patch.name = data.name
        if (data.description !== undefined) patch.description = data.description
        await db.clients.update(client.extends, patch)
      }
    }
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

    const all = await db.clients.toArray()
    return all.some(c => c.extends === id)
  }

  async function getExtendedClientIds() {
    const all = await db.clients.orderBy('name').toArray()
    return all.filter(c => c.extends !== undefined).map(c => c.extends)
  }

  // Returns the id of the primary client whose `extends` points to clientId, or null
  async function getPrimaryId(clientId) {
    const all = await db.clients.toArray()
    const primary = all.find(c => c.extends === clientId)
    return primary ? primary.id : null
  }

  // Returns the secondary client record for a given primary id, or null
  async function getSecondaryClient(primaryId) {
    const primary = await db.clients.get(primaryId)
    if (!primary?.extends) return null
    return db.clients.get(primary.extends)
  }

  // Saves all HIDs for a client, auto-creating or auto-deleting the secondary as needed
  async function saveHids(primaryId, allHids) {
    const primary = await db.clients.get(primaryId)

    if (allHids.length <= 4) {
      // Check if secondary has a controller port assignment before deleting
      if (primary.extends) {
        const controllers = await db.controllers.toArray()
        const secondaryAssigned = controllers.some(c =>
          Object.values(c.inputs || {}).includes(primary.extends)
        )
        if (secondaryAssigned) {
          throw new Error('Remove the secondary controller port assignment before removing extended buttons/switches')
        }
        await db.clients.delete(primary.extends)
        await db.clients.update(primaryId, { extends: null, hids: JSON.parse(JSON.stringify(allHids)) })
      } else {
        await db.clients.update(primaryId, { hids: JSON.parse(JSON.stringify(allHids)) })
      }
    } else {
      // Need secondary — get or create
      let secondaryId = primary.extends
      if (!secondaryId) {
        secondaryId = await db.clients.add({
          name: primary.name,
          description: primary.description,
          area: primary.area,
          mac: 'ff:ff:ff:ff:ff:ff',
          uuid: randomUUID(),
          hids: []
        })
        await db.clients.update(primaryId, { extends: secondaryId })
      }
      await db.clients.update(primaryId, { hids: JSON.parse(JSON.stringify(allHids.slice(0, 4))) })
      await db.clients.update(secondaryId, { hids: JSON.parse(JSON.stringify(allHids.slice(4))) })
    }

    await load()
  }

  return { items, load, get, create, update, remove, isInUse, getExtendedClientIds, getPrimaryId, getSecondaryClient, saveHids }
}
