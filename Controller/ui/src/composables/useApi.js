const TIMEOUT = 5000

export async function apiFetch(path, options = {}) {
  const ctrl = new AbortController()
  const id = setTimeout(() => ctrl.abort(), TIMEOUT)
  const url = '/api' + path
  try {
    const res = await fetch(url, { ...options, signal: ctrl.signal })
    clearTimeout(id)
    return res
  } catch (err) {
    clearTimeout(id)
    throw err
  }
}

export async function controllerFetch(ip, path, options = {}, token = '') {
  const ctrl = new AbortController()
  const id = setTimeout(() => ctrl.abort(), TIMEOUT)
  const url = path.startsWith('/auth') ? `http://${ip}${path}` : `http://${ip}/api${path}`
  try {
    const res = await fetch(url, {
      ...options,
      headers: { 'visual-token': token, ...options.headers },
      signal: ctrl.signal
    })
    clearTimeout(id)
    return res
  } catch (err) {
    clearTimeout(id)
    throw err
  }
}
