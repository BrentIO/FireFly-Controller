import { reactive } from 'vue'
import { apiFetch } from './useApi'
import { isCloudMode } from './useCloudMode'

const state = reactive({
  apiVersion: '',
  uiVersion: isCloudMode ? (import.meta.env.VITE_UI_VERSION || '') : '',
})

export function useAppState() {
  async function loadAppState() {
    if (isCloudMode) return
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
