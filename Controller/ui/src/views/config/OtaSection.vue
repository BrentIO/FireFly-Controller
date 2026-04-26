<template>
  <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-6">
    <div class="flex items-center gap-3 mb-4">
      <input v-model="form.enabled" type="checkbox" :id="`enabled-${settingKey}`" class="rounded border-gray-300 text-blue-600 focus:ring-blue-500" />
      <label :for="`enabled-${settingKey}`" class="text-base font-semibold text-gray-900 dark:text-gray-100">{{ title }}</label>
    </div>

    <div v-if="form.enabled" class="space-y-4">
      <div class="flex gap-3">
        <div class="w-28">
          <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Protocol</label>
          <select v-model="form.protocol" class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
            <option value="http">http</option>
            <option value="https">https</option>
          </select>
        </div>
        <div class="flex-1">
          <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">URL</label>
          <input v-model="form.url" type="text" maxlength="128" required
            class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500" />
        </div>
      </div>

      <div v-if="form.protocol === 'https'">
        <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Certificate</label>
        <select v-model="form.certificate"
          class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
          <option value="">Built-in root CA bundle</option>
          <option v-for="cert in certificates" :key="cert.id" :value="cert.id">{{ cert.commonName }}</option>
        </select>
      </div>
    </div>

    <div class="flex justify-end mt-4">
      <button type="button" class="px-4 py-2.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors print:hidden" @click="save">Save</button>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { useSettings } from '../../composables/useSettings'
import { useToast } from '../../composables/useToast'

const props = defineProps({
  title: String,
  settingKey: String,
  certificates: Array
})

const { getSetting, setSetting } = useSettings()
const { addToast } = useToast()
const DEFAULT_URL = 'api.fireflylx.com/ota/$$pid$$/$$app$$'
const form = ref({ enabled: false, protocol: 'https', url: DEFAULT_URL, certificate: '' })

onMounted(async () => {
  const saved = await getSetting(props.settingKey)
  if (saved) form.value = { enabled: saved.enabled ?? false, protocol: saved.protocol ?? 'https', url: saved.url ?? DEFAULT_URL, certificate: saved.certificate ?? '' }
})

async function save() {
  try {
    await setSetting(props.settingKey, { ...form.value })
    addToast('success', `${props.title} OTA settings saved.`)
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}
</script>
