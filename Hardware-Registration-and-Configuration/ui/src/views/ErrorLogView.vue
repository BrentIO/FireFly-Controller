<template>
  <AppLayout>
    <div class="max-w-2xl">
      <h2 class="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-6">Error Log</h2>

      <div v-if="loading" class="text-gray-500 dark:text-gray-400 text-sm">Loading…</div>

      <p v-else-if="rows.length === 0" class="text-gray-500 dark:text-gray-400 text-sm">
        No errors.
      </p>

      <div
        v-else
        class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 overflow-hidden"
      >
        <SortableTable :columns="columns" :rows="rows" :page-size="10" />
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

const columns = [{ key: 'text', label: 'Text', sortable: false }]

async function load() {
  loading.value = true
  try {
    const res = await apiFetch('/errors')
    if (res.ok) {
      rows.value = await res.json()
      setNavError('errors', rows.value.length > 0)
    } else {
      addToast('error', `Error log fetch failed (${res.status})`)
      setNavError('errors', true)
    }
  } catch (_) {
    addToast('error', 'Error log fetch error')
    setNavError('errors', true)
  } finally {
    loading.value = false
  }
}

onMounted(load)
</script>
