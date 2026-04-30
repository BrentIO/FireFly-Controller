<template>
  <AppLayout>
    <div class="max-w-2xl">
      <h2 class="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-6">Network</h2>
      <div v-if="loading" class="text-gray-500 dark:text-gray-400 text-sm">Loading…</div>
      <div
        v-else
        class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 overflow-hidden"
      >
        <SortableTable
          :columns="columns"
          :rows="rows"
          :default-sort="{ key: 'interfaceLabel', dir: 'asc' }"
        />
      </div>
    </div>
  </AppLayout>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import SortableTable from '../components/SortableTable.vue'
import { apiFetch } from '../composables/useApi'
import { useToast } from '../composables/useToast'
import { useAppState } from '../composables/useAppState'

const { addToast } = useToast()
const { setNavError } = useAppState()

const rows = ref([])
const loading = ref(true)

const INTERFACE_LABELS = {
  bluetooth: 'Bluetooth',
  ethernet:  'Ethernet',
  wifi:      'WiFi',
  wifi_ap:   'SoftAP'
}

const columns = [
  { key: 'interfaceLabel', label: 'Interface' },
  { key: 'mac_address',    label: 'MAC Address' }
]

async function load() {
  loading.value = true
  try {
    const res = await apiFetch('/network')
    if (res.ok) {
      const data = await res.json()
      rows.value = data.map(r => ({
        ...r,
        interfaceLabel: INTERFACE_LABELS[r.interface] ?? r.interface
      }))
      setNavError('network', false)
    } else {
      addToast('error', `Network fetch failed (${res.status})`)
      setNavError('network', true)
    }
  } catch (_) {
    addToast('error', 'Network fetch error')
    setNavError('network', true)
  } finally {
    loading.value = false
  }
}

onMounted(load)
</script>
