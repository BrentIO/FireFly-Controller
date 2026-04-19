import { reactive } from 'vue'
import { apiFetch } from './useApi'

const state = reactive({
  apiVersion: '',
  identityLoaded: false,
  registrationState: { registered: false, checkedAt: 0 },
  navErrors: {
    mcu: false,
    network: false,
    identity: false,
    registration: false,
    partitions: false,
    peripherals: false,
    events: false,
    errors: false
  }
})

export function useAppState() {
  async function loadRegistrationState() {
    try {
      const res = await apiFetch('/registration')
      if (res.ok) {
        const data = await res.json()
        state.registrationState.registered = data.registered ?? false
        state.registrationState.checkedAt = data.checked_at ?? 0
        state.navErrors.registration = !data.registered
      }
    } catch (_) {}
  }

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

    if (state.identityLoaded) {
      await loadRegistrationState()
    }
  }

  function setIdentityLoaded(value) {
    state.identityLoaded = value
  }

  function setNavError(key, value) {
    state.navErrors[key] = value
  }

  return { state, loadAppState, loadRegistrationState, setIdentityLoaded, setNavError }
}
