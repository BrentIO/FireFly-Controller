<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100">Validate Configuration</h1>
      <button
        class="px-4 py-2 rounded-lg bg-green-600 hover:bg-green-700 text-white text-sm font-medium transition-colors"
        :disabled="running"
        @click="run"
      >{{ running ? 'Validating…' : 'Re-validate' }}</button>
    </div>

    <div v-if="running" class="text-gray-500 dark:text-gray-400 text-sm">Validating…</div>

    <div v-else-if="results.length === 0 && ran" class="flex items-center gap-3 p-4 rounded-xl bg-green-50 dark:bg-green-900/20 border border-green-200 dark:border-green-800 text-green-800 dark:text-green-300">
      <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor" class="w-5 h-5 flex-shrink-0">
        <path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zm3.857-9.809a.75.75 0 00-1.214-.882l-3.483 4.79-1.88-1.88a.75.75 0 10-1.06 1.061l2.5 2.5a.75.75 0 001.137-.089l4-5.5z" clip-rule="evenodd" />
      </svg>
      <span class="text-sm font-medium">Configuration is valid. No issues found.</span>
    </div>

    <div v-else-if="results.length" class="space-y-2">
      <div
        v-for="(r, i) in results"
        :key="i"
        class="p-3 rounded-lg text-sm"
        :class="r.type === 'error'
          ? 'bg-red-50 dark:bg-red-900/20 text-red-800 dark:text-red-300'
          : 'bg-amber-50 dark:bg-amber-900/20 text-amber-800 dark:text-amber-300'"
      >{{ r.message }}</div>
    </div>
  </AppLayout>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import { checkConfiguration } from '../composables/usePayloads'

const results = ref([])
const running = ref(false)
const ran = ref(false)

async function run() {
  running.value = true
  results.value = []
  try {
    const errors = await checkConfiguration()
    results.value = errors.map(e => {
      if (typeof e === 'string') {
        return { type: e.toLowerCase().includes('error') ? 'error' : 'warning', message: e }
      }
      return { type: e.type || 'warning', message: e.message || String(e) }
    })
  } finally {
    running.value = false
    ran.value = true
  }
}

onMounted(run)
</script>
