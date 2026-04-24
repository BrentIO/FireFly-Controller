import { reactive } from 'vue'
import { apiFetch } from './useApi'

const state = reactive({
  apiVersion: '',
  uiVersion: import.meta.env.VITE_UI_VERSION ?? 'dev'
})

let fetched = false

export function useAppState() {
  async function loadAppState() {
    if (fetched) return
    try {
      const res = await apiFetch('/version')
      if (res.ok) {
        const data = await res.json()
        state.apiVersion = data.application ?? ''
        fetched = true
      }
    } catch (_) {}
  }

  return { state, loadAppState }
}
