<template>
  <RouterView />
  <ToastContainer />

  <!-- Cloud mode welcome modal (once per session) -->
  <Teleport to="body">
    <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
      <div v-if="showCloudNotice" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
        <div class="w-full max-w-md bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6">
          <h2 class="text-lg font-semibold text-gray-900 dark:text-gray-100 mb-3">Data is stored locally</h2>
          <div class="text-sm text-gray-700 dark:text-gray-300 space-y-3">
            <p>All configuration is stored locally in your browser and is not automatically sent to your controllers.</p>
            <p>To apply changes, <strong>export</strong> your configuration from this site and <strong>import</strong> it using the on-device controller interface.</p>
          </div>
          <div class="flex justify-end mt-5">
            <button class="px-4 py-2.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors" @click="dismissCloudNotice">Got it</button>
          </div>
        </div>
      </div>
    </Transition>
  </Teleport>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import { RouterView } from 'vue-router'
import ToastContainer from './components/ToastContainer.vue'
import { useTheme } from './composables/useTheme'
import { isCloudMode } from './composables/useCloudMode'

const { applyTheme } = useTheme()
const showCloudNotice = ref(false)

onMounted(() => {
  applyTheme()
  if (isCloudMode && !sessionStorage.getItem('cloud_notice_seen')) {
    showCloudNotice.value = true
  }
})

function dismissCloudNotice() {
  sessionStorage.setItem('cloud_notice_seen', '1')
  showCloudNotice.value = false
}
</script>
