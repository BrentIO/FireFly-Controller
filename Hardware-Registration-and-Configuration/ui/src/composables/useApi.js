const TIMEOUT = 5000
const token = () => sessionStorage.getItem('visual_token')

export async function apiFetch(path, options = {}) {
  const ctrl = new AbortController()
  const id = setTimeout(() => ctrl.abort(), TIMEOUT)
  const url = path.startsWith('/auth') ? path : '/api' + path
  try {
    const res = await fetch(url, {
      ...options,
      headers: { 'visual-token': token(), ...options.headers },
      signal: ctrl.signal
    })
    clearTimeout(id)
    if (res.status === 401) {
      sessionStorage.removeItem('visual_token')
      const { default: router } = await import('../router')
      router.push('/login')
    }
    return res
  } catch (err) {
    clearTimeout(id)
    throw err
  }
}
