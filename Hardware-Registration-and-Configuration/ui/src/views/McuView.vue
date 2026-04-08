<template>
  <AppLayout>
    <div class="max-w-2xl">
      <h2 class="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-6">MCU</h2>

      <div v-if="loading" class="text-gray-500 dark:text-gray-400 text-sm">Loading…</div>

      <div
        v-else-if="mcu"
        class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 divide-y divide-gray-100 dark:divide-gray-800"
      >
        <div v-for="row in rows" :key="row.label" class="flex px-6 py-4">
          <span class="w-40 text-sm font-medium text-gray-500 dark:text-gray-400">{{ row.label }}</span>
          <span class="text-sm text-gray-900 dark:text-gray-100">{{ row.value }}</span>
        </div>
      </div>

      <div class="mt-6">
        <button
          class="px-4 py-2 bg-red-600 text-white text-sm font-medium rounded-lg hover:bg-red-700 disabled:opacity-50 transition-colors"
          :disabled="rebooting"
          @click="showConfirm = true"
        >
          {{ rebooting ? 'Rebooting…' : 'Reboot' }}
        </button>
      </div>
    </div>

    <ConfirmModal
      :show="showConfirm"
      title="Confirm Reboot"
      message="Are you sure you want to reboot the device? This will immediately restart the MCU."
      confirm-label="Reboot"
      variant="danger"
      @confirm="reboot"
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

const { addToast } = useToast()
const { setNavError } = useAppState()

const mcu = ref(null)
const loading = ref(true)
const rebooting = ref(false)
const showConfirm = ref(false)

const rows = computed(() => {
  if (!mcu.value) return []
  const psram = mcu.value.psram_size > 0
    ? `${(mcu.value.psram_size / 1048576).toFixed(1)} MB`
    : 'No PSRAM'
  return [
    { label: 'Chip Model',  value: mcu.value.chip_model },
    { label: 'Revision',    value: mcu.value.revision },
    { label: 'Flash Size',  value: `${(mcu.value.flash_chip_size / 1048576).toFixed(1)} MB` },
    { label: 'PSRAM',       value: psram },
    { label: 'Boot Time',   value: new Date(mcu.value.boot_time * 1000).toLocaleString() }
  ]
})

async function load() {
  loading.value = true
  try {
    const res = await apiFetch('/mcu')
    if (res.ok) {
      mcu.value = await res.json()
      setNavError('mcu', false)
    } else {
      addToast('error', `MCU fetch failed (${res.status})`)
      setNavError('mcu', true)
    }
  } catch (_) {
    addToast('error', 'MCU fetch error')
    setNavError('mcu', true)
  } finally {
    loading.value = false
  }
}

async function reboot() {
  showConfirm.value = false
  rebooting.value = true
  try {
    const res = await apiFetch('/mcu/reboot', { method: 'POST' })
    if (!res.ok) {
      addToast('error', `Reboot failed (${res.status})`)
      setNavError('mcu', true)
    }
  } catch (_) {
    addToast('error', 'Reboot error')
    setNavError('mcu', true)
  } finally {
    rebooting.value = false
  }
}

onMounted(load)
</script>
