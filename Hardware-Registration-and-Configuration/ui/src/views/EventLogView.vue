<template>
  <AppLayout>
    <div>
      <div class="flex items-center justify-between mb-6">
        <h2 class="text-xl font-semibold text-gray-900">Event Log</h2>
        <button
          class="px-3 py-1.5 text-sm bg-white border border-gray-300 rounded-md hover:bg-gray-50"
          @click="load"
        >
          Refresh
        </button>
      </div>
      <div v-if="loading" class="text-gray-500 text-sm">Loading…</div>
      <div v-else class="bg-white rounded-lg shadow overflow-hidden">
        <SortableTable
          :columns="columns"
          :rows="rows"
          :default-sort="{ key: 'time', dir: 'desc' }"
          :page-size="10"
        >
          <template #level="{ row }">
            <span
              class="inline-flex items-center px-2 py-0.5 rounded text-xs font-medium"
              :class="levelClass(row.level)"
            >
              {{ levelLabel(row.level) }}
            </span>
          </template>
          <template #time="{ row }">
            {{ new Date(row.time * 1000).toLocaleString() }}
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
  { key: 'time',  label: 'Time' },
  { key: 'level', label: 'Level' },
  { key: 'text',  label: 'Text', sortable: false }
]

const LEVEL_CLASSES = {
  info:    'bg-blue-100 text-blue-800',
  notify:  'bg-amber-100 text-amber-800',
  error:   'bg-red-100 text-red-800',
  unknown: 'bg-gray-100 text-gray-800'
}
const LEVEL_LABELS = {
  info: 'Info', notify: 'Notification', error: 'Error', unknown: 'Unknown'
}

function levelClass(l) { return LEVEL_CLASSES[l] ?? LEVEL_CLASSES.unknown }
function levelLabel(l) { return LEVEL_LABELS[l] ?? 'Unknown' }

async function load() {
  loading.value = true
  try {
    const res = await apiFetch('/events')
    if (res.ok) {
      rows.value = await res.json()
      setNavError('events', false)
    } else {
      addToast('error', `Event log fetch failed (${res.status})`)
      setNavError('events', true)
    }
  } catch (_) {
    addToast('error', 'Event log fetch error')
    setNavError('events', true)
  } finally {
    loading.value = false
  }
}

onMounted(load)
</script>
