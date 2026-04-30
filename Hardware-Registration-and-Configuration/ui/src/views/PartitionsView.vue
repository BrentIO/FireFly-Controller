<template>
  <AppLayout>
    <div>
      <h2 class="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-6">Partition Table</h2>

      <div v-if="loading" class="text-gray-500 dark:text-gray-400 text-sm">Loading…</div>

      <div
        v-else-if="errorMsg"
        class="bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-700 text-red-800 dark:text-red-300 text-sm rounded-lg px-4 py-3"
      >
        {{ errorMsg }}
      </div>

      <div
        v-else
        class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 overflow-hidden"
      >
        <SortableTable :columns="columns" :rows="rows">
          <template #sizeLabel="{ row }">
            {{ showRawSize ? row.sizeRaw : row.sizeLabel }}
          </template>
          <template #status="{ row }">
            <span
              v-if="row.errors.length === 0"
              class="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium bg-green-100 dark:bg-green-900/40 text-green-800 dark:text-green-300"
            >OK</span>
            <span v-else class="flex flex-col gap-0.5">
              <span
                v-for="e in row.errors"
                :key="e"
                class="inline-flex items-center px-2 py-0.5 rounded-full text-xs font-medium bg-red-100 dark:bg-red-900/40 text-red-800 dark:text-red-300"
              >{{ e }}</span>
            </span>
          </template>
        </SortableTable>
      </div>
    </div>
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import SortableTable from '../components/SortableTable.vue'
import { apiFetch } from '../composables/useApi'
import { useToast } from '../composables/useToast'
import { useAppState } from '../composables/useAppState'

/* global __DEVICES__ */
const devices = __DEVICES__

const { addToast } = useToast()
const { setNavError } = useAppState()

const loading = ref(true)
const errorMsg = ref('')
const rows = ref([])
const showRawSize = ref(false)

const columns = computed(() => [
  { key: 'label',        label: 'Label' },
  { key: 'typeLabel',    label: 'Type' },
  { key: 'subtypeLabel', label: 'Subtype' },
  { key: 'addressLabel', label: 'Address' },
  { key: 'sizeLabel',    label: showRawSize.value ? 'Size (raw)' : 'Size', sortable: false, onClick: () => { showRawSize.value = !showRawSize.value } },
  { key: 'status',       label: 'Status', sortable: false }
])

const APP_SUBTYPES = {
  0: 'Factory', 16: 'OTA_0', 17: 'OTA_1', 18: 'OTA_2', 19: 'OTA_3',
  20: 'OTA_4', 21: 'OTA_5', 22: 'OTA_6', 23: 'OTA_7', 24: 'OTA_8',
  25: 'OTA_9', 26: 'OTA_10', 27: 'OTA_11', 28: 'OTA_12', 29: 'OTA_13',
  30: 'OTA_14', 31: 'OTA_15', 32: 'Test'
}
const DATA_SUBTYPES = {
  0: 'OTA', 1: 'PHY', 2: 'NVS', 3: 'CoreDump', 4: 'NVS Keys',
  5: 'eFuse EM', 6: 'Undefined', 128: 'ESPHTTPD', 129: 'FAT',
  130: 'SPIFFS/LittleFS', 131: 'LittleFS'
}
const TYPE_NAMES = { 0: 'App', 1: 'Data', 2: 'Bootloader', 3: 'Partition Table' }

function formatType(type) {
  const name = TYPE_NAMES[type] ?? 'Unknown'
  return `${name} (0x${type.toString(16).padStart(2, '0').toUpperCase()})`
}
function formatSubtype(type, subtype) {
  const map = type === 0 ? APP_SUBTYPES : type === 1 ? DATA_SUBTYPES : {}
  const name = map[subtype]
  const hex = `0x${subtype.toString(16).padStart(2, '0').toUpperCase()}`
  return name ? `${name} (${hex})` : hex
}
function formatAddress(addr) {
  return `0x${addr.toString(16).padStart(6, '0').toUpperCase()}`
}
function formatSize(bytes) {
  if (bytes >= 1048576) return `${(bytes / 1048576).toFixed(1)} MB`
  return `${(bytes / 1024).toFixed(1)} KB`
}
function formatSizeRaw(bytes) {
  return `0x${bytes.toString(16).padStart(6, '0').toUpperCase()}`
}
function checkPartition(api, expected) {
  const errors = []
  if (api.type !== expected.type) errors.push('Unexpected Type')
  if (api.subtype !== expected.subtype) errors.push('Unexpected Subtype')
  if (api.address !== parseInt(expected.address, 16)) errors.push('Unexpected Address')
  if (api.size !== parseInt(expected.size, 16)) errors.push('Unexpected Size')
  if (api.label !== expected.label) errors.push('Unexpected Label')
  return errors
}

async function load() {
  loading.value = true
  errorMsg.value = ''
  try {
    const [versionRes, partitionsRes] = await Promise.all([
      apiFetch('/version'),
      apiFetch('/partitions')
    ])
    if (!versionRes.ok || !partitionsRes.ok) {
      const failed = !versionRes.ok ? 'version' : 'partitions'
      addToast('error', `Failed to fetch ${failed}`)
      setNavError('partitions', true)
      return
    }
    const versionData = await versionRes.json()
    const partitions = await partitionsRes.json()
    const productHex = versionData.product_hex?.toLowerCase()
    const device = devices.find(d => d.product_hex.toLowerCase() === productHex)
    if (!device) {
      errorMsg.value = `No matching device found for product_hex ${productHex}`
      setNavError('partitions', true)
      return
    }
    const expected = device.partition_scheme
    if (partitions.length !== expected.length) {
      errorMsg.value = `Partition count mismatch: expected ${expected.length}, got ${partitions.length}`
      setNavError('partitions', true)
      return
    }
    rows.value = partitions.map((p, i) => {
      const exp = expected[i]
      const errors = exp ? checkPartition(p, exp) : ['No expected partition']
      return {
        label:        p.label,
        typeLabel:    formatType(p.type),
        subtypeLabel: formatSubtype(p.type, p.subtype),
        addressLabel: formatAddress(p.address),
        sizeLabel:    formatSize(p.size),
        sizeRaw:      formatSizeRaw(p.size),
        errors
      }
    })
    setNavError('partitions', rows.value.some(r => r.errors.length > 0))
  } catch (_) {
    addToast('error', 'Partition fetch error')
    setNavError('partitions', true)
  } finally {
    loading.value = false
  }
}

onMounted(load)
</script>
