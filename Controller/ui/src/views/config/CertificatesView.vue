<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-6 print:text-xl">Certificates</h1>

    <!-- Info box -->
    <div class="mb-6 p-4 bg-amber-50 dark:bg-amber-900/20 rounded-lg border border-amber-200 dark:border-amber-800 text-sm text-amber-800 dark:text-amber-300">
      <p class="font-medium mb-1">Most users should not upload a certificate.</p>
      <p>By default, the system validates HTTPS OTA connections using the Mozilla root CA bundle (~130 CAs). Uploading a certificate <strong>replaces</strong> the entire bundle — only uploaded certificates will be trusted. Only upload if your OTA server uses a CA not included in the Mozilla bundle.</p>
    </div>

    <!-- Drop zone -->
    <div
      class="mb-6 flex flex-col items-center justify-center rounded-xl border-2 border-dashed border-gray-300 dark:border-gray-600 bg-gray-50 dark:bg-gray-800/50 p-8 text-center cursor-pointer hover:border-blue-400 transition-colors print:hidden"
      :class="{ 'border-blue-500 bg-blue-50 dark:bg-blue-900/20': dragging }"
      @dragover.prevent="dragging = true"
      @dragleave="dragging = false"
      @drop.prevent="onDrop"
      @click="fileInput.click()"
    >
      <p class="text-sm text-gray-500 dark:text-gray-400">Drag and drop a certificate file (.pem, .crt) here, or click to browse.</p>
      <input ref="fileInput" type="file" accept=".pem,.crt,.cer" class="hidden" @change="onFileSelect" />
    </div>

    <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-hidden">
      <table class="w-full text-sm">
        <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
          <tr>
            <th class="px-4 py-3 text-left">Common Name</th>
            <th class="px-4 py-3 text-left">Organization</th>
            <th class="px-4 py-3 text-left">Expires</th>
            <th class="px-4 py-3 text-right print:hidden">Actions</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
          <tr v-if="items.length === 0">
            <td colspan="4" class="px-4 py-8 text-center text-gray-400 dark:text-gray-500">No certificates stored.</td>
          </tr>
          <tr v-for="cert in items" :key="cert.id" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
            <td class="px-4 py-3 text-gray-900 dark:text-gray-100 font-medium">{{ cert.commonName }}</td>
            <td class="px-4 py-3 text-gray-600 dark:text-gray-400">{{ cert.organization }}</td>
            <td class="px-4 py-3 text-gray-600 dark:text-gray-400">{{ cert.expiration }}</td>
            <td class="px-4 py-3 text-right print:hidden">
              <button class="text-blue-600 hover:text-blue-700 dark:text-blue-400 mr-3 text-sm" @click="doExport(cert)">Export</button>
              <button class="text-red-600 hover:text-red-700 dark:text-red-400 text-sm" @click="confirmDelete(cert)">Delete</button>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <ConfirmModal
      :show="!!deleteTarget"
      title="Delete Certificate"
      :message="`Delete certificate '${deleteTarget?.commonName}'? This cannot be undone.`"
      confirm-label="Delete"
      @confirm="doDelete"
      @cancel="deleteTarget = null"
    />
  </AppLayout>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import AppLayout from '../../components/AppLayout.vue'
import ConfirmModal from '../../components/ConfirmModal.vue'
import { useCertificates } from '../../composables/useCertificates'
import { useToast } from '../../composables/useToast'

const { items, load, store, remove, isInUse } = useCertificates()
const { addToast } = useToast()
const fileInput = ref(null)
const dragging = ref(false)
const deleteTarget = ref(null)

onMounted(load)

function onDrop(e) {
  dragging.value = false
  const file = e.dataTransfer.files[0]
  if (file) processFile(file)
}

function onFileSelect(e) {
  const file = e.target.files[0]
  if (file) processFile(file)
  e.target.value = ''
}

async function processFile(file) {
  const text = await file.text()
  if (!text.includes('-----BEGIN CERTIFICATE-----')) {
    addToast('error', 'File does not appear to be a valid PEM certificate.')
    return
  }
  try {
    const parsed = parseCertBasic(text)
    await store(file.name, text, parsed)
    addToast('success', `Certificate "${parsed.commonName || file.name}" stored.`)
  } catch (e) {
    addToast('error', `Failed to store certificate: ${e.message}`)
  }
}

function parseCertBasic(pem) {
  const cnMatch = pem.match(/CN=([^,\n/]+)/)
  const orgMatch = pem.match(/O=([^,\n/]+)/)
  return {
    commonName: cnMatch?.[1]?.trim() ?? '',
    organization: orgMatch?.[1]?.trim() ?? '',
    expiration: ''
  }
}

function doExport(cert) {
  const blob = new Blob([cert.certificate], { type: 'application/x-x509-ca-cert' })
  const url = URL.createObjectURL(blob)
  const a = document.createElement('a')
  a.href = url
  a.download = cert.fileName
  a.click()
  URL.revokeObjectURL(url)
}

async function confirmDelete(cert) {
  if (await isInUse(cert.id)) {
    addToast('warning', `Certificate "${cert.commonName}" is used by OTA configuration and cannot be deleted.`)
    return
  }
  deleteTarget.value = cert
}

async function doDelete() {
  try {
    await remove(deleteTarget.value.id)
    addToast('success', 'Certificate deleted.')
  } catch (e) {
    addToast('error', `Failed to delete: ${e.message}`)
  } finally {
    deleteTarget.value = null
  }
}
</script>
