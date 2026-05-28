import { reactive } from 'vue'
import { apiFetch } from './useApi'

const state = reactive({
  apiVersion: '',
  uiVersion: '',
})

export function useAppState() {
  async function loadAppState() {
    try {
      const res = await apiFetch('/version')
      if (res.ok) {
        const data = await res.json()
        const app = data.application
        state.apiVersion = app ? `${app.version} (${app.commit})` : ''
        const ui = data.ui
        state.uiVersion = ui ? `${ui.version} (${ui.commit})` : ''
      }
    } catch (_) {}
  }

  return { state, loadAppState }
}
