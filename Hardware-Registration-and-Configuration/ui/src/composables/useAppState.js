import { reactive } from 'vue'
import { apiFetch } from './useApi'

const state = reactive({
  apiVersion: '',
  identityLoaded: false,
  navErrors: {
    mcu: false,
    network: false,
    identity: false,
    partitions: false,
    peripherals: false,
    events: false,
    errors: false
  }
})

export function useAppState() {
  async function loadAppState() {
    try {
      const res = await apiFetch('/version')
      if (res.ok) {
        const data = await res.json()
        state.apiVersion = data.application ?? ''
      }
    } catch (_) {}

    try {
      const res = await apiFetch('/identity')
      state.identityLoaded = res.status === 200
    } catch (_) {}
  }

  function setIdentityLoaded(value) {
    state.identityLoaded = value
  }

  function setNavError(key, value) {
    state.navErrors[key] = value
  }

  return { state, loadAppState, setIdentityLoaded, setNavError }
}
