<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-6 print:text-xl">OTA Updates</h1>

    <div class="mb-6 p-4 bg-amber-50 dark:bg-amber-900/20 rounded-lg border border-amber-200 dark:border-amber-800 text-sm text-amber-800 dark:text-amber-300">
      <p class="font-medium mb-1">Wildcard substitutions in URL:</p>
      <span class="font-mono text-xs">$$pid$$</span> — product ID &nbsp;&nbsp;
      <span class="font-mono text-xs">$$app$$</span> — application name
    </div>

    <div class="space-y-6 max-w-lg">
      <OtaSection title="Controllers" setting-key="ota_controller" :certificates="certList" />
      <OtaSection title="Clients" setting-key="ota_client" :certificates="certList" />
    </div>
  </AppLayout>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import AppLayout from '../../components/AppLayout.vue'
import OtaSection from './OtaSection.vue'
import { useCertificates } from '../../composables/useCertificates'

const { items: certList, load } = useCertificates()
onMounted(load)
</script>
