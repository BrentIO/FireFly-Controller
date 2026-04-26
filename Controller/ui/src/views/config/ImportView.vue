<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-6 print:text-xl">Import / Export</h1>

    <div class="max-w-lg space-y-6">
      <!-- Export -->
      <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-6">
        <h2 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-2">Export Configuration</h2>
        <p class="text-sm text-gray-500 dark:text-gray-400 mb-4">Download a backup of all configuration data stored in your browser.</p>
        <button class="px-4 py-2.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors print:hidden" @click="doExport">
          Download Backup
        </button>
      </div>

      <!-- Import -->
      <div class="bg-white dark:bg-gray-900 rounded-xl border border-amber-200 dark:border-amber-800 p-6">
        <h2 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-2">Import Configuration</h2>
        <p class="text-sm text-amber-700 dark:text-amber-400 mb-4">
          Importing will replace all existing configuration data. This cannot be undone.
        </p>
        <div class="space-y-3">
          <input ref="fileInput" type="file" accept=".json" class="block text-sm text-gray-600 dark:text-gray-400" @change="onFileSelect" />
          <button
            :disabled="!selectedFile"
            class="px-4 py-2.5 text-sm font-medium text-white rounded-lg transition-colors disabled:opacity-40 print:hidden"
            :class="selectedFile ? 'bg-amber-600 hover:bg-amber-700' : 'bg-gray-400'"
            @click="doImport"
          >Import</button>
        </div>
      </div>
    </div>
  </AppLayout>
</template>

<script setup>
import { ref } from 'vue'
import Dexie from 'dexie'
import { exportDB, importDB } from 'dexie-export-import'
import { db } from '../../composables/useDatabase'
import AppLayout from '../../components/AppLayout.vue'
import { useToast } from '../../composables/useToast'

const { addToast } = useToast()
const fileInput = ref(null)
const selectedFile = ref(null)

function onFileSelect(e) {
  selectedFile.value = e.target.files[0] ?? null
}

async function doExport() {
  try {
    const blob = await exportDB(db)
    const url = URL.createObjectURL(blob)
    const a = document.createElement('a')
    a.href = url
    a.download = `FireFlyConfig_${Date.now()}.json`
    a.click()
    URL.revokeObjectURL(url)
    addToast('success', 'Export successful!')
  } catch (e) {
    addToast('error', `Export failed: ${e.message}`)
  }
}

async function doImport() {
  if (!selectedFile.value) return
  try {
    await db.delete()
    await importDB(selectedFile.value)
    addToast('success', 'Import successful! Reloading…')
    setTimeout(() => window.location.reload(), 1500)
  } catch (e) {
    addToast('error', `Import failed: ${e.message}`)
  }
}
</script>
