import { reactive } from 'vue'
import { controllerFetch } from './useApi'

const LOGIN_MAX_MS = 30 * 60 * 1000

function storageKey(id) {
  return `controller_${id}`
}

function defaultSession() {
  return {
    ip: '',
    visualToken: '',
    isAuthenticated: false,
    authenticationTime: null,
    firmwareVersion: '',
    uiVersion: '',
    uuid: '',
    certificates: [],
    eventLog: [],
    errorLog: [],
    lastDeploymentTime: 0,
    provisioningModeEnabled: false
  }
}

function loadFromStorage(id) {
  try {
    const raw = localStorage.getItem(storageKey(id))
    if (!raw) return defaultSession()
    const record = JSON.parse(raw)
    const session = { ...defaultSession(), ...record }

    if (session.isAuthenticated && session.authenticationTime) {
      const elapsed = Date.now() - new Date(session.authenticationTime).getTime()
      if (elapsed > LOGIN_MAX_MS) {
        Object.assign(session, defaultSession())
        localStorage.setItem(storageKey(id), JSON.stringify(session))
      }
    }
    return session
  } catch {
    return defaultSession()
  }
}

function saveToStorage(id, session) {
  localStorage.setItem(storageKey(id), JSON.stringify(session))
}

export function useControllerSession(id) {
  const session = reactive(loadFromStorage(id))

  function save() {
    saveToStorage(id, session)
  }

  function setIp(value) {
    session.ip = value
    save()
  }

  function setVisualToken(value) {
    session.visualToken = value.trim()
    save()
  }

  function logout() {
    Object.assign(session, defaultSession())
    save()
  }

  async function authenticate() {
    if (!session.ip) throw new Error('IP address not set.')
    if (!session.visualToken) throw new Error('Visual token not set.')

    const res = await controllerFetch(session.ip, '/auth', { method: 'POST' }, session.visualToken)

    if (res.status === 204) {
      session.isAuthenticated = true
      session.authenticationTime = new Date().getTime()
      save()
    } else {
      session.isAuthenticated = false
      session.visualToken = ''
      save()
      throw new Error(`Authentication failure (${res.status})`)
    }
  }

  return { session, setIp, setVisualToken, logout, authenticate, save }
}
