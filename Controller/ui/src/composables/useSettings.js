import { db } from './useDatabase'

export function useSettings() {
  async function getSetting(key) {
    const record = await db.settings.where({ setting: key }).first()
    return record?.value ?? null
  }

  async function setSetting(key, value) {
    await db.settings.put({ setting: key, value })
  }

  return { getSetting, setSetting }
}
