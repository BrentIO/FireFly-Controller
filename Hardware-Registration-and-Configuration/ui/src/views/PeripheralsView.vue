<template>
  <AppLayout>
    <div class="max-w-2xl">
      <h2 class="text-xl font-semibold text-gray-900 mb-6">Peripherals</h2>
      <div v-if="loading" class="text-gray-500 text-sm">Loading…</div>
      <div v-else class="bg-white rounded-lg shadow overflow-hidden">
        <SortableTable
          :columns="columns"
          :rows="rows"
          :default-sort="{ key: 'address', dir: 'asc' }"
        >
          <template #status="{ row }">
            <span
              class="inline-flex items-center px-2 py-0.5 rounded text-xs font-medium"
              :class="row.online ? 'bg-green-100 text-green-800' : 'bg-red-100 text-red-800'"
            >
              {{ row.online ? 'Online' : 'Offline' }}
            </span>
          </template>
        </SortableTable>
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

const columns = [
  { key: 'address', label: 'Address' },
  { key: 'type',    label: 'Type' },
  { key: 'status',  label: 'Status', sortable: false }
]

async function load() {
  loading.value = true
  try {
    const res = await apiFetch('/peripherals')
    if (res.ok) {
      rows.value = await res.json()
      const anyOffline = rows.value.some(r => !r.online)
      setNavError('peripherals', anyOffline)
    } else {
      addToast('error', `Peripherals fetch failed (${res.status})`)
      setNavError('peripherals', true)
    }
  } catch (_) {
    addToast('error', 'Peripherals fetch error')
    setNavError('peripherals', true)
  } finally {
    loading.value = false
  }
}

onMounted(load)
</script>
