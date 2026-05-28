/* global __DEVICES__ */
import { reactive } from 'vue'
import { apiFetch } from './useApi'

const state = reactive({
  apiVersion: '',
  uiVersion: '',
  productHex: '',
  identityLoaded: false,
  registrationState: { registered: false, checkedAt: 0, error: false, message: '', cloudApiRoot: '' },
  navErrors: {
    mcu: false,
    network: false,
    identity: false,
    registration: false,
    firmware: false,
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
        state.registrationState.registered   = data.registered ?? false
        state.registrationState.checkedAt    = data.checked_at ?? 0
        state.registrationState.error        = false
        state.registrationState.message      = ''
        state.registrationState.cloudApiRoot = data.cloud_api_root ?? ''
        state.navErrors.registration       = !data.registered
      } else {
        const body = await res.json().catch(() => ({}))
        state.registrationState.error   = true
        state.registrationState.message = body.message ?? `Error ${res.status}`
        state.navErrors.registration    = true
      }
    } catch (_) {}
  }

  async function loadPartitionsState() {
    if (!state.productHex) return
    try {
      const res = await apiFetch('/partitions')
      if (!res.ok) { state.navErrors.partitions = true; return }
      const partitions = await res.json()
      const device = __DEVICES__.find(d => d.product_hex.toLowerCase() === state.productHex.toLowerCase())
      if (!device) { state.navErrors.partitions = true; return }
      const expected = device.partition_scheme
      if (partitions.length !== expected.length) { state.navErrors.partitions = true; return }
      state.navErrors.partitions = partitions.some((p, i) => {
        const exp = expected[i]
        return !exp
          || p.type    !== exp.type
          || p.subtype !== exp.subtype
          || p.address !== parseInt(exp.address, 16)
          || p.size    !== parseInt(exp.size, 16)
          || p.label   !== exp.label
      })
    } catch (_) {
      state.navErrors.partitions = true
    }
  }

  async function loadAppState() {
    try {
      const res = await apiFetch('/version')
      if (res.ok) {
        const data = await res.json()
        const app = data.application
        state.apiVersion  = app ? `${app.version} (${app.commit})` : ''
        const ui = data.ui
        state.uiVersion   = ui ? `${ui.version} (${ui.commit})` : ''
        state.productHex  = data.product_hex ?? ''
      }
    } catch (_) {}

    try {
      const res = await apiFetch('/identity')
      state.identityLoaded = res.status === 200
    } catch (_) {}

    const proactive = [
      apiFetch('/mcu').then(res => {
        if (res.ok) return res.json().then(d => { state.navErrors.mcu = d.psram_size === 0 })
        state.navErrors.mcu = true
      }).catch(() => { state.navErrors.mcu = true }),

      apiFetch('/errors').then(res => {
        if (res.ok) return res.json().then(d => { state.navErrors.errors = d.length > 0 })
        state.navErrors.errors = true
      }).catch(() => { state.navErrors.errors = true }),

      loadPartitionsState(),
    ]

    if (state.identityLoaded) {
      proactive.push(
        loadRegistrationState(),
        apiFetch('/peripherals').then(res => {
          if (res.ok) return res.json().then(d => { state.navErrors.peripherals = d.some(r => !r.online) })
          state.navErrors.peripherals = true
        }).catch(() => { state.navErrors.peripherals = true })
      )
    }

    await Promise.all(proactive)
  }

  function setIdentityLoaded(value) {
    state.identityLoaded = value
  }

  function setNavError(key, value) {
    state.navErrors[key] = value
  }

  return { state, loadAppState, loadRegistrationState, setIdentityLoaded, setNavError }
}
