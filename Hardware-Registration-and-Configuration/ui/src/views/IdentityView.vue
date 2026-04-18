<template>
  <AppLayout>
    <div class="max-w-2xl">
      <h2 class="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-6">Identity</h2>

      <div v-if="loading" class="text-gray-500 dark:text-gray-400 text-sm">Loading…</div>

      <form
        v-else
        class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 p-6 space-y-5"
        @submit.prevent="save"
      >
        <div
          v-if="hasIdentity"
          class="bg-amber-50 dark:bg-amber-900/20 border border-amber-200 dark:border-amber-700 text-amber-800 dark:text-amber-300 text-sm rounded-lg px-4 py-3"
        >
          This device has a device identity stored in eFuse. Identity is permanent and cannot be changed.
        </div>

        <!-- UUID -->
        <div>
          <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">UUID</label>
          <div class="flex gap-2">
            <input
              v-model="form.uuid"
              type="text"
              minlength="36"
              maxlength="36"
              :disabled="hasIdentity"
              class="block flex-1 border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 rounded-lg px-3 py-2 text-sm font-mono disabled:opacity-60 focus:outline-none focus:ring-2 focus:ring-blue-500"
              placeholder="xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
              required
            />
            <button
              v-if="!hasIdentity"
              type="button"
              class="px-3 py-2 text-xs font-medium bg-gray-100 dark:bg-gray-700 text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-200 dark:hover:bg-gray-600 transition-colors whitespace-nowrap"
              @click="generateUUID"
            >
              Generate
            </button>
          </div>
        </div>

        <!-- Product ID -->
        <div>
          <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Product ID</label>
          <select
            v-model="form.product_hex"
            :disabled="hasIdentity"
            class="block w-full border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 rounded-lg px-3 py-2 text-sm disabled:opacity-60 focus:outline-none focus:ring-2 focus:ring-blue-500"
            required
          >
            <option value="" disabled>Select a product</option>
            <option
              v-for="d in sortedDevices"
              :key="d.product_hex"
              :value="d.product_hex"
            >
              {{ d.product_id }}
            </option>
          </select>
        </div>

        <div class="flex gap-3 pt-2">
          <button
            v-if="!hasIdentity"
            type="submit"
            :disabled="saving"
            class="px-4 py-2 bg-blue-600 text-white text-sm font-medium rounded-lg hover:bg-blue-700 disabled:opacity-50 transition-colors"
          >
            {{ saving ? 'Saving…' : 'Save' }}
          </button>
        </div>
      </form>
    </div>
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import { apiFetch } from '../composables/useApi'
import { useToast } from '../composables/useToast'
import { useAppState } from '../composables/useAppState'

/* global __DEVICES__ */
const devices = __DEVICES__

const { addToast } = useToast()
const { setNavError, setIdentityLoaded } = useAppState()

const loading = ref(true)
const saving = ref(false)
const hasIdentity = ref(false)

const form = ref({ uuid: '', product_hex: '' })

const sortedDevices = computed(() =>
  [...devices].sort((a, b) => a.product_id.localeCompare(b.product_id))
)

function generateUUID() {
  // crypto.randomUUID() requires a secure context (HTTPS); device serves HTTP.
  // Build v4 UUID manually using getRandomValues(), which works in all contexts.
  const bytes = new Uint8Array(16)
  crypto.getRandomValues(bytes)
  bytes[6] = (bytes[6] & 0x0f) | 0x40  // version 4
  bytes[8] = (bytes[8] & 0x3f) | 0x80  // variant 1
  const hex = Array.from(bytes).map(b => b.toString(16).padStart(2, '0')).join('')
  form.value.uuid = `${hex.slice(0, 8)}-${hex.slice(8, 12)}-${hex.slice(12, 16)}-${hex.slice(16, 20)}-${hex.slice(20)}`
}

async function load() {
  loading.value = true
  try {
    const res = await apiFetch('/identity')
    if (res.status === 200) {
      const data = await res.json()
      form.value = { uuid: data.uuid, product_hex: data.product_hex }
      hasIdentity.value = true
      setNavError('identity', false)
    } else if (res.status === 404) {
      hasIdentity.value = false
      setNavError('identity', false)
    } else {
      addToast('error', `Identity fetch failed (${res.status})`)
      setNavError('identity', true)
    }
  } catch (_) {
    addToast('error', 'Identity fetch error')
    setNavError('identity', true)
  } finally {
    loading.value = false
  }
}

async function save() {
  saving.value = true
  const selected = devices.find(d => d.product_hex === form.value.product_hex)
  try {
    const res = await apiFetch('/identity', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({
        uuid: form.value.uuid,
        product_id: selected?.product_id ?? '',
        product_hex: form.value.product_hex
      })
    })
    if (res.status === 201) {
      addToast('success', 'Identity burned to eFuse')
      setIdentityLoaded(true)
      await load()
    } else {
      const body = await res.json().catch(() => ({}))
      addToast('error', body.message ?? `Save failed (${res.status})`)
      setNavError('identity', true)
    }
  } catch (_) {
    addToast('error', 'Save error')
    setNavError('identity', true)
  } finally {
    saving.value = false
  }
}

onMounted(load)
</script>
