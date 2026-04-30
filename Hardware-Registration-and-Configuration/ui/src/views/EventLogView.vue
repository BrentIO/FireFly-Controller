<template>
  <AppLayout>
    <div>
      <div class="flex items-center justify-between mb-6">
        <h2 class="text-xl font-semibold text-gray-900 dark:text-gray-100">Event Log</h2>
        <button
          class="p-1.5 bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 text-gray-700 dark:text-gray-300 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors"
          title="Refresh"
          @click="load"
        >
          <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-4 h-4">
            <path stroke-linecap="round" stroke-linejoin="round" d="M16.023 9.348h4.992v-.001M2.985 19.644v-4.992m0 0h4.992m-4.993 0 3.181 3.183a8.25 8.25 0 0 0 13.803-3.7M4.031 9.865a8.25 8.25 0 0 1 13.803-3.7l3.181 3.182m0-4.991v4.99" />
          </svg>
        </button>
      </div>
      <div v-if="loading" class="text-gray-500 dark:text-gray-400 text-sm">Loading…</div>
      <div
        v-else
        class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 overflow-hidden"
      >
        <SortableTable
          :columns="columns"
          :rows="rows"
          :default-sort="{ key: 'time', dir: 'desc' }"
          :page-size="10"
        >
          <template #level="{ row }">
            <span
              class="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium"
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
  info:    'bg-blue-100 dark:bg-blue-900/40 text-blue-800 dark:text-blue-300',
  notify:  'bg-amber-100 dark:bg-amber-900/40 text-amber-800 dark:text-amber-300',
  error:   'bg-red-100 dark:bg-red-900/40 text-red-800 dark:text-red-300',
  unknown: 'bg-gray-100 dark:bg-gray-700 text-gray-700 dark:text-gray-300'
}
const LEVEL_LABELS = { info: 'Info', notify: 'Notification', error: 'Error', unknown: 'Unknown' }

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
