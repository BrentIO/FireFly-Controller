<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-6 print:text-xl">Certificates</h1>

    <!-- Info box -->
    <div class="mb-6 p-4 bg-amber-50 dark:bg-amber-900/20 rounded-lg border border-amber-200 dark:border-amber-800 text-sm text-amber-800 dark:text-amber-300">
      <p class="font-medium mb-1">Most users should not upload a certificate.</p>
      <p>By default, the system validates HTTPS OTA connections using the Mozilla root CA bundle (~130 CAs). Uploading a certificate <strong>replaces</strong> the bundle for the selected type — only uploaded certificates will be trusted for that purpose. Only upload if your OTA server or client devices use a CA not included in the Mozilla bundle.</p>
    </div>

    <!-- Certificate type selection -->
    <div class="mb-4 p-4 bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 print:hidden">
      <p class="text-sm font-medium text-gray-700 dark:text-gray-300 mb-3">Certificate type (select at least one before uploading):</p>
      <div class="flex flex-wrap gap-6">
        <label class="flex items-center gap-2 cursor-pointer">
          <input v-model="typeController" type="checkbox" class="w-4 h-4 rounded border-gray-300 text-blue-600 focus:ring-blue-500" />
          <span class="text-sm text-gray-700 dark:text-gray-300">Controller — trusted for HTTPS OTA validation</span>
        </label>
        <label class="flex items-center gap-2 cursor-pointer" :class="{ 'opacity-50 cursor-not-allowed': hasClientCert }">
          <input v-model="typeClient" type="checkbox" class="w-4 h-4 rounded border-gray-300 text-blue-600 focus:ring-blue-500" :disabled="hasClientCert" />
          <span class="text-sm text-gray-700 dark:text-gray-300">
            Client — distributed to clients for mutual TLS
            <span v-if="hasClientCert" class="ml-1 text-xs text-amber-600 dark:text-amber-400">(a client cert is already designated)</span>
          </span>
        </label>
      </div>
      <p v-if="!typeController && !typeClient" class="mt-2 text-xs text-red-600 dark:text-red-400">Select at least one type before uploading.</p>
    </div>

    <!-- Drop zone -->
    <div
      class="mb-6 flex flex-col items-center justify-center rounded-xl border-2 border-dashed border-gray-300 dark:border-gray-600 bg-gray-50 dark:bg-gray-800/50 p-8 text-center transition-colors print:hidden"
      :class="typeController || typeClient ? 'cursor-pointer hover:border-blue-400' : 'opacity-50 cursor-not-allowed'"
      @dragover.prevent="(typeController || typeClient) && (dragging = true)"
      @dragleave="dragging = false"
      @drop.prevent="(typeController || typeClient) && onDrop($event)"
      @click="(typeController || typeClient) && fileInput.click()"
    >
      <p class="text-sm text-gray-500 dark:text-gray-400">Drag and drop a certificate file (.pem, .crt) here, or click to browse.</p>
      <input ref="fileInput" type="file" accept=".pem,.crt,.cer" class="hidden" @change="onFileSelect" />
    </div>

    <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-x-auto">
      <table class="w-full text-sm">
        <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
          <tr>
            <th class="px-4 py-3 text-left">Common Name</th>
            <th class="px-4 py-3 text-left">Organization</th>
            <th class="px-4 py-3 text-left">Expires</th>
            <th class="px-4 py-3 text-center">Controller</th>
            <th class="px-4 py-3 text-center">Client</th>
            <th class="px-4 py-3 text-right print:hidden">Actions</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
          <tr v-if="items.length === 0">
            <td colspan="6" class="px-4 py-8 text-center text-gray-400 dark:text-gray-500">No certificates stored.</td>
          </tr>
          <tr v-for="cert in items" :key="cert.id" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
            <td class="px-4 py-3 text-gray-900 dark:text-gray-100 font-medium">{{ cert.commonName }}</td>
            <td class="px-4 py-3 text-gray-600 dark:text-gray-400">{{ cert.organization }}</td>
            <td class="px-4 py-3 text-gray-600 dark:text-gray-400 whitespace-nowrap">
              <span v-if="isExpired(cert)" class="inline-flex items-center gap-1 text-red-600 dark:text-red-400">
                <svg class="w-4 h-4 flex-shrink-0" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M10 18a8 8 0 100-16 8 8 0 000 16zM8.28 7.22a.75.75 0 00-1.06 1.06L8.94 10l-1.72 1.72a.75.75 0 101.06 1.06L10 11.06l1.72 1.72a.75.75 0 101.06-1.06L11.06 10l1.72-1.72a.75.75 0 00-1.06-1.06L10 8.94 8.28 7.22z" clip-rule="evenodd"/></svg>
                {{ cert.expiration }}
              </span>
              <span v-else-if="isExpiringSoon(cert)" class="inline-flex items-center gap-1 text-amber-600 dark:text-amber-400">
                <svg class="w-4 h-4 flex-shrink-0" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M8.485 2.495c.673-1.167 2.357-1.167 3.03 0l6.28 10.875c.673 1.167-.17 2.625-1.516 2.625H3.72c-1.347 0-2.189-1.458-1.515-2.625L8.485 2.495zM10 5a.75.75 0 01.75.75v3.5a.75.75 0 01-1.5 0v-3.5A.75.75 0 0110 5zm0 9a1 1 0 100-2 1 1 0 000 2z" clip-rule="evenodd"/></svg>
                {{ cert.expiration }}
              </span>
              <span v-else>{{ cert.expiration }}</span>
            </td>
            <td class="px-4 py-3 text-center">
              <span v-if="cert.isController" class="inline-flex items-center justify-center w-5 h-5 rounded-full bg-green-100 dark:bg-green-900/30 text-green-600 dark:text-green-400">
                <svg class="w-3.5 h-3.5" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z" clip-rule="evenodd"/></svg>
              </span>
            </td>
            <td class="px-4 py-3 text-center">
              <span v-if="cert.isClient" class="inline-flex items-center justify-center w-5 h-5 rounded-full bg-green-100 dark:bg-green-900/30 text-green-600 dark:text-green-400">
                <svg class="w-3.5 h-3.5" fill="currentColor" viewBox="0 0 20 20"><path fill-rule="evenodd" d="M16.707 5.293a1 1 0 010 1.414l-8 8a1 1 0 01-1.414 0l-4-4a1 1 0 011.414-1.414L8 12.586l7.293-7.293a1 1 0 011.414 0z" clip-rule="evenodd"/></svg>
              </span>
            </td>
            <td class="px-4 py-3 text-right print:hidden">
              <div class="flex justify-end gap-2">
                <button class="px-2.5 py-1 text-xs font-medium rounded border border-gray-300 dark:border-gray-600 text-gray-700 dark:text-gray-300 hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="doExport(cert)">Export</button>
                <button class="px-2.5 py-1 text-xs font-medium rounded border border-red-200 dark:border-red-800 text-red-600 dark:text-red-400 hover:bg-red-50 dark:hover:bg-red-900/20 transition-colors" @click="confirmDelete(cert)">Delete</button>
              </div>
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
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../../components/AppLayout.vue'
import ConfirmModal from '../../components/ConfirmModal.vue'
import { useCertificates, parseCert } from '../../composables/useCertificates'
import { useToast } from '../../composables/useToast'

const { items, load, store, remove, isInUse } = useCertificates()
const { addToast } = useToast()
const fileInput = ref(null)
const dragging = ref(false)
const deleteTarget = ref(null)
const typeController = ref(false)
const typeClient = ref(false)

const hasClientCert = computed(() => items.value.some(c => c.isClient))

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
  if (!typeController.value && !typeClient.value) {
    addToast('error', 'Select at least one certificate type before uploading.')
    return
  }
  const text = await file.text()
  if (!text.includes('-----BEGIN CERTIFICATE-----')) {
    addToast('error', 'File does not appear to be a valid PEM certificate.')
    return
  }
  const parsed = parseCert(text)
  if (parsed.expirationDate && new Date(parsed.expirationDate) < new Date()) {
    addToast('error', `Certificate "${parsed.commonName || file.name}" has already expired on ${parsed.expiration}.`)
  }
  try {
    await store(file.name, text, parsed, typeController.value, typeClient.value)
  } catch (e) {
    addToast('error', `Failed to store certificate: ${e.message}`)
  }
}

function isExpired(cert) {
  return cert.expirationDate && new Date(cert.expirationDate) < new Date()
}

function isExpiringSoon(cert) {
  if (!cert.expirationDate) return false
  const exp = new Date(cert.expirationDate)
  const now = new Date()
  return exp >= now && exp < new Date(now.getTime() + 90 * 24 * 60 * 60 * 1000)
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
  } catch (e) {
    addToast('error', `Failed to delete: ${e.message}`)
  } finally {
    deleteTarget.value = null
  }
}
</script>
