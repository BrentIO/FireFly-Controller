<template>
  <AppLayout>
    <div class="max-w-2xl">
      <h2 class="text-xl font-semibold text-gray-900 mb-6">Identity</h2>

      <div v-if="loading" class="text-gray-500 text-sm">Loading…</div>

      <form v-else class="bg-white rounded-lg shadow p-6 space-y-5" @submit.prevent="save">
        <div
          v-if="hasIdentity"
          class="bg-amber-50 border border-amber-200 text-amber-800 text-sm rounded-md px-4 py-3"
        >
          This device has a device identity. Fields are read-only.
        </div>

        <div>
          <label class="block text-sm font-medium text-gray-700 mb-1">UUID</label>
          <input
            v-model="form.uuid"
            type="text"
            minlength="36"
            maxlength="36"
            :disabled="hasIdentity"
            class="block w-full border border-gray-300 rounded-md px-3 py-2 text-sm font-mono disabled:bg-gray-100 disabled:text-gray-500"
            placeholder="xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx"
            required
          />
        </div>

        <div>
          <label class="block text-sm font-medium text-gray-700 mb-1">Product ID</label>
          <select
            v-model="form.product_hex"
            :disabled="hasIdentity"
            class="block w-full border border-gray-300 rounded-md px-3 py-2 text-sm disabled:bg-gray-100 disabled:text-gray-500"
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

        <div>
          <label class="block text-sm font-medium text-gray-700 mb-1">Key</label>
          <input
            v-model="form.key"
            type="text"
            minlength="64"
            maxlength="64"
            :disabled="hasIdentity"
            class="block w-full border border-gray-300 rounded-md px-3 py-2 text-sm font-mono disabled:bg-gray-100 disabled:text-gray-500"
            placeholder="64-character alphanumeric key"
            required
          />
        </div>

        <div class="flex gap-3 pt-2">
          <button
            v-if="!hasIdentity"
            type="submit"
            :disabled="saving"
            class="px-4 py-2 bg-blue-600 text-white text-sm rounded-md hover:bg-blue-700 disabled:opacity-50"
          >
            {{ saving ? 'Saving…' : 'Save' }}
          </button>
          <button
            v-if="hasIdentity"
            type="button"
            :disabled="deleting"
            class="px-4 py-2 bg-red-600 text-white text-sm rounded-md hover:bg-red-700 disabled:opacity-50"
            @click="showConfirm = true"
          >
            {{ deleting ? 'Deleting…' : 'Delete Identity' }}
          </button>
        </div>
      </form>
    </div>

    <ConfirmModal
      :show="showConfirm"
      title="Delete Device Identity"
      :message="`Delete identity for UUID ${form.uuid} (${currentProductId})?`"
      confirm-label="Delete"
      @confirm="deleteIdentity"
      @cancel="showConfirm = false"
    />
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { apiFetch } from '../composables/useApi'
import { useToast } from '../composables/useToast'
import { useAppState } from '../composables/useAppState'

/* global __DEVICES__ */
const devices = __DEVICES__

const { addToast } = useToast()
const { setNavError, setIdentityLoaded } = useAppState()

const loading = ref(true)
const saving = ref(false)
const deleting = ref(false)
const hasIdentity = ref(false)
const showConfirm = ref(false)

const form = ref({ uuid: '', product_hex: '', key: '' })

const sortedDevices = computed(() =>
  [...devices].sort((a, b) => a.product_id.localeCompare(b.product_id))
)

const currentProductId = computed(() => {
  const d = devices.find(d => d.product_hex === form.value.product_hex)
  return d?.product_id ?? form.value.product_hex
})

async function load() {
  loading.value = true
  try {
    const res = await apiFetch('/identity')
    if (res.status === 200) {
      const data = await res.json()
      form.value = { uuid: data.uuid, product_hex: data.product_hex, key: data.key }
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
        product_hex: form.value.product_hex,
        key: form.value.key
      })
    })
    if (res.status === 201) {
      addToast('success', 'Identity saved')
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

async function deleteIdentity() {
  showConfirm.value = false
  deleting.value = true
  try {
    const res = await apiFetch('/identity', { method: 'DELETE' })
    if (res.status === 204) {
      addToast('success', 'Identity deleted')
      setIdentityLoaded(false)
      form.value = { uuid: '', product_hex: '', key: '' }
      hasIdentity.value = false
      setNavError('identity', false)
    } else {
      const body = await res.json().catch(() => ({}))
      addToast('error', body.message ?? `Delete failed (${res.status})`)
      setNavError('identity', true)
    }
  } catch (_) {
    addToast('error', 'Delete error')
    setNavError('identity', true)
  } finally {
    deleting.value = false
  }
}

onMounted(load)
</script>
