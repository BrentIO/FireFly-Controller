<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-6 print:text-xl">MQTT</h1>

    <div class="mb-6 p-4 bg-amber-50 dark:bg-amber-900/20 rounded-lg border border-amber-200 dark:border-amber-800 text-sm text-amber-800 dark:text-amber-300">
      <p class="font-medium mb-2">Wildcard substitutions available in host field:</p>
      <table class="text-xs">
        <tr><td class="font-mono pr-4">$$mac$$</td><td>MAC address without separators</td></tr>
        <tr><td class="font-mono pr-4">$$mac_dashes$$</td><td>MAC address with dashes</td></tr>
        <tr><td class="font-mono pr-4">$$mac_colons$$</td><td>MAC address with colons</td></tr>
        <tr><td class="font-mono pr-4">$$uuid$$</td><td>Device UUID</td></tr>
      </table>
    </div>

    <form class="max-w-md bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-6 space-y-4" @submit.prevent="save">
      <div>
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Broker Host / IP</label>
        <input v-model="form.host" type="text" required
          class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
      </div>
      <div>
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Port</label>
        <input v-model.number="form.port" type="number" min="1" max="65535" required
          class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
      </div>
      <div>
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Username <span class="font-normal text-gray-400">(optional)</span></label>
        <input v-model="form.username" type="text"
          class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
      </div>
      <div>
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Password <span class="font-normal text-gray-400">(optional)</span></label>
        <input v-model="form.password" type="password"
          class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
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
const form = ref({ host: '', port: 1883, username: '', password: '' })

onMounted(async () => {
  const saved = await getSetting('mqtt')
  if (saved) form.value = { host: saved.host ?? '', port: saved.port ?? 1883, username: saved.username ?? '', password: saved.password ?? '' }
})

async function save() {
  try {
    await setSetting('mqtt', form.value)
    addToast('success', 'MQTT settings saved.')
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}
</script>
