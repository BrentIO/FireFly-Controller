import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function useCertificates() {
  async function load() {
    items.value = await db.certificates.orderBy('commonName').toArray()
  }

  async function store(fileName, certificate, parsed) {
    const existing = await db.certificates.where({ certificate }).count()
    if (existing > 0) return

    await db.certificates.put({
      fileName,
      certificate,
      commonName: parsed.commonName ?? '',
      expiration: parsed.expiration ?? '',
      organization: parsed.organization ?? '',
      organizationalUnitName: parsed.organizationalUnitName ?? ''
    })
    await load()
  }

  async function remove(id) {
    await db.certificates.delete(id)
    await load()
  }

  async function exportCert(id) {
    const result = await db.certificates.where({ id }).toArray()
    if (result.length !== 1) throw new Error('Certificate not found')
    return result[0]
  }

  async function isInUse(id) {
    const count = await db.settings
      .where('setting').anyOf(['ota_controller', 'ota_client'])
      .and(entry => entry.value?.certificate === id)
      .count()
    return count > 0
  }

  return { items, load, store, remove, exportCert, isInUse }
}
