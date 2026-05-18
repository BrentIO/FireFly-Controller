import { ref } from 'vue'
import { db } from './useDatabase'

const items = ref([])

export function parseCert(pem) {
  try {
    const b64 = pem.replace(/-----[^-]+-----/g, '').replace(/\s+/g, '')
    const der = Uint8Array.from(atob(b64), c => c.charCodeAt(0))
    let i = 0

    const readLen = () => {
      const b = der[i++]
      if (b < 0x80) return b
      let n = 0
      for (let k = b & 0x7f; k > 0; k--) n = (n << 8) | der[i++]
      return n
    }
    const expectTag = t => { if (der[i++] !== t) throw new Error('unexpected tag') }
    const readOIDBytes = () => { expectTag(0x06); const len = readLen(); return der.slice(i, i += len) }
    const readString = () => { i++; const len = readLen(); return new TextDecoder().decode(der.slice(i, i += len)) }
    const readTime = () => {
      const tag = der[i++]
      const len = readLen()
      const s = String.fromCharCode(...der.slice(i, i += len))
      if (tag === 0x17) {
        const y = parseInt(s.slice(0, 2), 10)
        return new Date(`${y >= 50 ? 1900 + y : 2000 + y}-${s.slice(2, 4)}-${s.slice(4, 6)}T${s.slice(6, 8)}:${s.slice(8, 10)}:${s.slice(10, 12)}Z`)
      }
      return new Date(`${s.slice(0, 4)}-${s.slice(4, 6)}-${s.slice(6, 8)}T${s.slice(8, 10)}:${s.slice(10, 12)}:${s.slice(12, 14)}Z`)
    }
    const parseName = () => {
      expectTag(0x30)
      const nameEnd = i + readLen()
      const result = { cn: '', org: '' }
      while (i < nameEnd) {
        expectTag(0x31)
        const setEnd = i + readLen()
        while (i < setEnd) {
          expectTag(0x30)
          const avaEnd = i + readLen()
          const oid = readOIDBytes()
          const val = readString()
          if (oid.length === 3 && oid[0] === 0x55 && oid[1] === 0x04) {
            if (oid[2] === 0x03) result.cn = val
            if (oid[2] === 0x0a) result.org = val
          }
          i = avaEnd
        }
        i = setEnd
      }
      i = nameEnd
      return result
    }

    expectTag(0x30); readLen()
    expectTag(0x30); readLen()
    if (der[i] === 0xa0) { i++; i += readLen() }
    expectTag(0x02); i += readLen()
    expectTag(0x30); i += readLen()
    parseName()
    expectTag(0x30); readLen()
    readTime()
    const notAfter = readTime()
    const subject = parseName()

    return {
      commonName: subject.cn,
      organization: subject.org,
      expirationDate: notAfter.toISOString(),
      expiration: notAfter.toLocaleDateString(undefined, { year: 'numeric', month: 'short', day: 'numeric' })
    }
  } catch {
    return { commonName: '', organization: '', expirationDate: null, expiration: '' }
  }
}

export function useCertificates() {
  async function load() {
    const all = await db.certificates.orderBy('commonName').toArray()

    const unparsed = all.filter(c => c.certificate && c.expirationDate === undefined)
    if (unparsed.length > 0) {
      await Promise.all(unparsed.map(c => {
        const parsed = parseCert(c.certificate)
        return db.certificates.update(c.id, {
          commonName: parsed.commonName,
          organization: parsed.organization,
          expiration: parsed.expiration,
          expirationDate: parsed.expirationDate
        })
      }))
      items.value = await db.certificates.orderBy('commonName').toArray()
    } else {
      items.value = all
    }
  }

  async function store(fileName, certificate, parsed) {
    const existing = await db.certificates.where({ certificate }).count()
    if (existing > 0) return

    await db.certificates.put({
      fileName,
      certificate,
      commonName: parsed.commonName ?? '',
      organization: parsed.organization ?? '',
      expiration: parsed.expiration ?? '',
      expirationDate: parsed.expirationDate ?? null,
      organizationalUnitName: parsed.organizationalUnitName ?? ''
    })
    await load()
  }

  async function remove(id) {
    await db.certificates.delete(id)
    await load()
  }

  async function exportCert(id) {
    const result = await db.certificates.where({ id }).toArray()
    if (result.length !== 1) throw new Error('Certificate not found')
    return result[0]
  }

  async function isInUse(id) {
    const count = await db.settings
      .where('setting').anyOf(['ota_controller', 'ota_client'])
      .and(entry => entry.value?.certificate === id)
      .count()
    return count > 0
  }

  return { items, load, store, remove, exportCert, isInUse }
}
