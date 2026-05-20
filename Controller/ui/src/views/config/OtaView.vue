<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-6 print:text-xl">OTA Updates</h1>

    <div class="mb-6 p-4 bg-amber-50 dark:bg-amber-900/20 rounded-lg border border-amber-200 dark:border-amber-800 text-sm text-amber-800 dark:text-amber-300">
      <p class="font-medium mb-2">Wildcard substitutions in URL:</p>
      <table class="text-xs">
        <tr><td class="font-mono pr-4">$$mac$$</td><td>MAC address without separators</td></tr>
        <tr><td class="font-mono pr-4">$$mac_dashes$$</td><td>MAC address with dashes</td></tr>
        <tr><td class="font-mono pr-4">$$mac_colons$$</td><td>MAC address with colons</td></tr>
        <tr><td class="font-mono pr-4">$$uuid$$</td><td>Device UUID</td></tr>
        <tr><td class="font-mono pr-4">$$class$$</td><td>hardware class</td></tr>
        <tr><td class="font-mono pr-4">$$product_hex$$</td><td>product ID</td></tr>
        <tr><td class="font-mono pr-4">$$application$$</td><td>application name</td></tr>
        <tr><td class="font-mono pr-4">$$current_version$$</td><td>currently installed firmware version</td></tr>
      </table>
    </div>

    <div class="space-y-6 max-w-lg">
      <OtaSection title="Controllers" setting-key="ota_controller" cert-type="controller" :certificates="certList" />
      <OtaSection title="Clients" setting-key="ota_client" cert-type="client" :certificates="certList" />
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
