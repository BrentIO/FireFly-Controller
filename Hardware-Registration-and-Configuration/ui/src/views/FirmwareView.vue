<template>
  <AppLayout>
    <div class="max-w-2xl">
      <div class="flex items-center justify-between mb-6">
        <h2 class="text-xl font-semibold text-gray-900 dark:text-gray-100">Firmware</h2>
        <button
          @click="load()"
          :disabled="loading"
          class="px-3 py-1.5 text-sm font-medium text-gray-700 dark:text-gray-300 bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 disabled:opacity-50 disabled:cursor-not-allowed transition-colors"
        >Refresh</button>
      </div>

      <div v-if="loading" class="text-gray-500 dark:text-gray-400 text-sm">Loading firmware list…</div>

      <div v-else-if="flashing" class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 p-6 space-y-3">
        <p class="text-sm font-medium text-gray-900 dark:text-gray-100">Flashing {{ flashingVersion }}…</p>
        <p class="text-sm text-gray-500 dark:text-gray-400">
          The device is downloading and installing the firmware. It will reboot automatically when complete.
          Once rebooted, connect to the Controller application.
        </p>
      </div>

      <div v-else class="space-y-4">
        <div v-if="versions.length === 0" class="text-gray-500 dark:text-gray-400 text-sm">
          No released firmware found for this device.
        </div>

        <div
          v-for="item in versions"
          :key="item.version"
          class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 px-6 py-4 flex items-center justify-between gap-4"
        >
          <div class="min-w-0">
            <p class="text-sm font-semibold text-gray-900 dark:text-gray-100 font-mono">{{ item.version }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400 mt-0.5">{{ item.application_name }}</p>
          </div>
          <div class="flex items-center gap-2 flex-shrink-0">
            <a
              v-if="item.release_url"
              :href="item.release_url"
              target="_blank"
              rel="noopener"
              class="text-xs text-blue-600 dark:text-blue-400 hover:underline"
            >Release notes</a>
            <button
              @click="confirmFlash(item)"
              class="px-3 py-1.5 bg-blue-600 text-white text-xs font-medium rounded-lg hover:bg-blue-700 transition-colors"
            >
              Flash via OTA
            </button>
          </div>
        </div>
      </div>
    </div>

    <!-- Confirmation modal -->
    <Transition
      enter-active-class="ease-out duration-200"
      enter-from-class="opacity-0"
      enter-to-class="opacity-100"
      leave-active-class="ease-in duration-150"
      leave-from-class="opacity-100"
      leave-to-class="opacity-0"
    >
      <div
        v-if="confirmItem"
        class="fixed inset-0 bg-black/40 flex items-center justify-center z-50 px-4"
        @click.self="confirmItem = null"
      >
        <Transition
          enter-active-class="ease-out duration-200"
          enter-from-class="opacity-0 scale-95"
          enter-to-class="opacity-100 scale-100"
          leave-active-class="ease-in duration-150"
          leave-from-class="opacity-100 scale-100"
          leave-to-class="opacity-0 scale-95"
        >
          <div
            v-if="confirmItem"
            class="bg-white dark:bg-gray-900 rounded-xl shadow-xl ring-1 ring-black/10 dark:ring-white/10 w-full max-w-sm p-6 space-y-4"
          >
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100">Confirm Firmware Flash</h3>
            <p class="text-sm text-gray-600 dark:text-gray-400">
              Flash <span class="font-mono font-semibold text-gray-900 dark:text-gray-100">{{ confirmItem.version }}</span> via OTA?
              The device will reboot after the download completes.
            </p>
            <div class="flex justify-end gap-2 pt-2">
              <button
                @click="confirmItem = null"
                class="px-4 py-2 text-sm font-medium text-gray-700 dark:text-gray-300 bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors"
              >
                Cancel
              </button>
              <button
                @click="doFlash"
                class="px-4 py-2 text-sm font-medium text-white bg-blue-600 rounded-lg hover:bg-blue-700 transition-colors"
              >
                Confirm
              </button>
            </div>
          </div>
        </Transition>
      </div>
    </Transition>
  </AppLayout>
</template>

<script setup>
import { ref, onMounted, onUnmounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import { apiFetch } from '../composables/useApi'
import { useToast } from '../composables/useToast'

const { addToast } = useToast()

const versions   = ref([])
const loading    = ref(true)
const flashing   = ref(false)
const flashingVersion = ref('')
const confirmItem = ref(null)

let pollTimer = null

async function load() {
  loading.value = true
  try {
    const res = await apiFetch('/firmware')
    if (res.status === 200) {
      const data = await res.json()
      versions.value = data.versions ?? []
      loading.value = false
    } else if (res.status === 202) {
      /* Firmware list is being fetched — poll until ready */
      pollTimer = setTimeout(load, 2000)
    } else {
      addToast('error', `Firmware list fetch failed (${res.status})`)
      loading.value = false
    }
  } catch (_) {
    addToast('error', 'Firmware list fetch error')
    loading.value = false
  }
}

function confirmFlash(item) {
  confirmItem.value = item
}

async function doFlash() {
  const item = confirmItem.value
  confirmItem.value = null

  flashingVersion.value = item.version
  flashing.value = true

  try {
    const res = await apiFetch('/ota', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(item)
    })
    if (res.status !== 202) {
      const body = await res.json().catch(() => ({}))
      addToast('error', body.message ?? `Flash failed (${res.status})`)
      flashing.value = false
    }
    /* On 202, stay in the flashing state — device will reboot */
  } catch (_) {
    addToast('error', 'Flash request failed')
    flashing.value = false
  }
}

onMounted(load)
onUnmounted(() => { if (pollTimer) clearTimeout(pollTimer) })
</script>
