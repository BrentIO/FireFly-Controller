<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-6 print:text-xl">WiFi</h1>
    <p class="text-sm text-gray-500 dark:text-gray-400 mb-6">WiFi credentials used to provision client devices.</p>

    <form class="max-w-md bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-6 space-y-4" @submit.prevent="save">
      <div>
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">SSID</label>
        <input v-model="form.ssid" type="text" maxlength="32" required
          class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
      </div>
      <div>
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Password</label>
        <div class="relative">
          <input v-model="form.password" :type="showPassword ? 'text' : 'password'" minlength="8" maxlength="63" required
            class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 pr-16 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
          <button type="button" class="absolute right-3 top-1/2 -translate-y-1/2 text-xs text-blue-600 dark:text-blue-400"
            @click="showPassword = !showPassword">{{ showPassword ? 'Hide' : 'Show' }}</button>
        </div>
      </div>
      <div class="flex justify-end pt-2">
        <button type="submit" class="px-4 py-2.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors print:hidden">Save</button>
      </div>
    </form>
  </AppLayout>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import AppLayout from '../../components/AppLayout.vue'
import { useSettings } from '../../composables/useSettings'
import { useToast } from '../../composables/useToast'

const { getSetting, setSetting } = useSettings()
const { addToast } = useToast()
const form = ref({ ssid: '', password: '' })
const showPassword = ref(false)

onMounted(async () => {
  const saved = await getSetting('wifi')
  if (saved) form.value = { ssid: saved.ssid ?? '', password: saved.password ?? '' }
})

async function save() {
  try {
    await setSetting('wifi', form.value)
    addToast('success', 'WiFi settings saved.')
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}
</script>
