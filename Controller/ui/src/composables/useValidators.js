export function randomUUID() {
  if (typeof self.crypto.randomUUID === 'function') {
    return self.crypto.randomUUID()
  }
  return 'xxxxxxxx-xxxx-4xxx-yxxx-xxxxxxxxxxxx'.replace(/[xy]/g, (c) => {
    const r = (Math.random() * 16) | 0
    const v = c === 'x' ? r : (r & 0x3) | 0x8
    return v.toString(16)
  })
}

export function isValidUUID(value) {
  return /^[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}$/.test(value)
}

export function isValidIPv4(value) {
  return /^((25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])\.){3}(25[0-5]|2[0-4][0-9]|[0-1]?[0-9]?[0-9])(?::\d{0,4})?$/.test(value)
}

export function isValidDomainName(value) {
  return /^(((?!-))(xn--|_)?[a-z0-9-]{0,61}[a-z0-9]{1,1}\.)*(xn--)?([a-z0-9][a-z0-9\-]{0,60}|[a-z0-9-]{1,30}\.[a-z]{2,})(?::\d{0,4})??$/.test(value)
}

export function isValidURL(value) {
  return /^[-a-zA-Z0-9@:%._+~#=]{1,256}\.[a-zA-Z0-9()]{1,6}\b(?:[-a-zA-Z0-9()@:%_+.~#?&//=]*)$/.test(value)
}

export function isValidMacAddress(value) {
  return /^(?:[0-9A-Fa-f]{2}[:-]){5}(?:[0-9A-Fa-f]{2})$/.test(value)
}

export function isValidClientName(value) {
  return /^[A-Za-z0-9~!@#$%^&*()_+\-=|]{1,8}$/.test(value)
}

export function isValidCircuitName(value) {
  return /^[A-Za-z0-9~!@#$%^&*()_+\-=|]{1,8}$/.test(value)
}

export function moveArrayItem(arr, from, to) {
  const copy = [...arr]
  const [item] = copy.splice(from, 1)
  copy.splice(to, 0, item)
  return copy
}
