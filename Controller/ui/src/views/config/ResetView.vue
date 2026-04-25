<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-6">Reset</h1>

    <div class="max-w-lg bg-white dark:bg-gray-900 rounded-xl border border-red-200 dark:border-red-900 p-6">
      <h2 class="text-base font-semibold text-red-700 dark:text-red-400 mb-2">Delete All Configuration</h2>
      <p class="text-sm text-gray-600 dark:text-gray-400 mb-6">
        This will permanently delete all controllers, clients, circuits, breakers, areas, certificates, and settings stored in your browser. This action cannot be undone.
      </p>

      <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">
        Type <strong>reset</strong> to confirm
      </label>
      <input
        v-model="confirmation"
        type="text"
        class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-red-500 mb-4"
        placeholder="reset"
      />

      <button
        :disabled="confirmation !== 'reset'"
        class="px-4 py-2.5 rounded-lg text-sm font-medium text-white transition-colors min-h-[44px] disabled:opacity-40"
        :class="confirmation === 'reset' ? 'bg-red-600 hover:bg-red-700' : 'bg-gray-400 cursor-not-allowed'"
        @click="doReset"
      >
        Delete All Data
      </button>
    </div>
  </AppLayout>
</template>

<script setup>
import { ref } from 'vue'
import { db } from '../../composables/useDatabase'
import AppLayout from '../../components/AppLayout.vue'
import { useToast } from '../../composables/useToast'

const { addToast } = useToast()
const confirmation = ref('')

async function doReset() {
  try {
    await db.delete()
    localStorage.clear()
    addToast('success', 'All data deleted. Reloading…')
    setTimeout(() => window.location.reload(), 1500)
  } catch (e) {
    addToast('error', `Reset failed: ${e.message}`)
  }
}
</script>
