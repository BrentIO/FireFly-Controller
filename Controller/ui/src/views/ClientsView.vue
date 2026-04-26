<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Clients</h1>
      <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors print:hidden" @click="openAdd">
        Add Client
      </button>
    </div>

    <div class="grid gap-4 grid-cols-1 sm:grid-cols-2 xl:grid-cols-3">
      <div v-if="sortedItems.length === 0" class="text-gray-400 dark:text-gray-500 py-8 col-span-full">No clients defined.</div>

      <div v-for="client in sortedItems" :key="client.id"
        class="rounded-xl border p-4 break-inside-avoid"
        :class="client.mac === 'ff:ff:ff:ff:ff:ff'
          ? 'bg-yellow-50 dark:bg-yellow-900/10 border-yellow-300 dark:border-yellow-700'
          : 'bg-white dark:bg-gray-900 border-gray-200 dark:border-gray-700'">
        <div class="flex items-start justify-between gap-2 mb-3">
          <div>
            <p class="font-mono text-xs text-gray-500 dark:text-gray-400">{{ client.name }}</p>
            <p class="font-semibold text-gray-900 dark:text-gray-100">{{ client.description }}</p>
            <div class="flex items-center gap-1 mt-0.5">
              <svg v-if="client.mac === 'ff:ff:ff:ff:ff:ff'" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" class="w-3.5 h-3.5 flex-shrink-0">
                <path fill="black" d="M8.485 2.495c.673-1.167 2.357-1.167 3.03 0l6.28 10.875c.673 1.167-.17 2.625-1.516 2.625H3.72c-1.347 0-2.189-1.458-1.515-2.625L8.485 2.495z"/>
                <rect x="9.25" y="5.5" width="1.5" height="4" rx="0.75" fill="#eab308"/>
                <circle cx="10" cy="14" r="1" fill="#eab308"/>
              </svg>
              <p class="text-xs text-gray-500 dark:text-gray-400 font-mono">{{ client.mac }}</p>
            </div>
          </div>
          <div class="flex gap-2 print:hidden flex-shrink-0">
            <button class="text-blue-600 hover:text-blue-700 dark:text-blue-400 text-sm" @click="router.push(`/clients/${client.id}`)">Edit</button>
            <button class="text-red-600 hover:text-red-700 dark:text-red-400 text-sm" @click="confirmDelete(client)">Delete</button>
          </div>
        </div>
        <p class="text-xs text-gray-500 dark:text-gray-400">{{ hidSummary(client.hids) }}</p>
        <p v-if="client.mac === 'ff:ff:ff:ff:ff:ff'" class="text-xs text-yellow-700 dark:text-yellow-500 mt-1 font-medium">MAC Address is invalid.</p>
      </div>
    </div>

    <!-- Add modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50" @click.self="showModal = false">
          <div class="w-full max-w-md bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6">
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">Add Client</h3>
            <form @submit.prevent="save" class="space-y-4">
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Short ID <span class="text-xs font-normal text-gray-400">(max 8 chars)</span></label>
                <input v-model="form.name" type="text" maxlength="8" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Description <span class="text-xs font-normal text-gray-400">(max 20 chars)</span></label>
                <input v-model="form.description" type="text" maxlength="20" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Area</label>
                <select v-model.number="form.area" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select area…</option>
                  <option v-for="a in areas" :key="a.id" :value="a.id">{{ a.name }}</option>
                </select>
              </div>
              <div>
                <div class="flex items-center justify-between mb-1">
                  <label class="block text-sm font-medium text-gray-700 dark:text-gray-300">MAC Address</label>
                  <button type="button" class="text-xs text-blue-600 dark:text-blue-400 hover:underline" @click="generateMac">Generate</button>
                </div>
                <input v-model="form.mac" type="text" placeholder="aa:bb:cc:dd:ee:ff"
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base font-mono focus:outline-none focus:ring-2 focus:ring-blue-500"
                  :class="macError ? 'border-red-400 focus:ring-red-400' : ''" />
                <p v-if="macError" class="mt-1 text-xs text-red-600 dark:text-red-400">{{ macError }}</p>
                <div v-if="form.mac === 'ff:ff:ff:ff:ff:ff'" class="mt-1 flex items-start gap-1.5 text-xs text-yellow-600 dark:text-yellow-400">
                  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor" class="w-4 h-4 flex-shrink-0 mt-px">
                    <path fill-rule="evenodd" d="M8.485 2.495c.673-1.167 2.357-1.167 3.03 0l6.28 10.875c.673 1.167-.17 2.625-1.516 2.625H3.72c-1.347 0-2.189-1.458-1.515-2.625L8.485 2.495zM10 5a.75.75 0 01.75.75v3.5a.75.75 0 01-1.5 0v-3.5A.75.75 0 0110 5zm0 9a1 1 0 100-2 1 1 0 000 2z" clip-rule="evenodd" />
                  </svg>
                  <span>Placeholder MAC — update before provisioning.</span>
                </div>
              </div>
              <div>
                <div class="flex items-center justify-between mb-1">
                  <label class="block text-sm font-medium text-gray-700 dark:text-gray-300">UUID</label>
                  <button type="button" class="text-xs text-blue-600 dark:text-blue-400 hover:underline" @click="form.uuid = randomUUID()">Generate</button>
                </div>
                <input v-model="form.uuid" type="text" required pattern="[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div class="flex gap-3 justify-end pt-2">
                <button type="button" class="px-4 py-2.5 text-sm text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="showModal = false">Cancel</button>
                <button type="submit" class="px-4 py-2.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors">Save</button>
              </div>
            </form>
          </div>
        </div>
      </Transition>
    </Teleport>

    <ConfirmModal :show="!!deleteTarget" title="Delete Client"
      :message="`Delete client '${deleteTarget?.name}'? This cannot be undone.`"
      confirm-label="Delete" @confirm="doDelete" @cancel="deleteTarget = null" />
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRouter } from 'vue-router'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useClients } from '../composables/useClients'
import { useAreas } from '../composables/useAreas'
import { useToast } from '../composables/useToast'
import { randomUUID } from '../composables/useValidators'

const MAC_RE = /^[0-9A-Fa-f]{2}[:-]([0-9A-Fa-f]{2}[:-]){4}[0-9A-Fa-f]{2}$/

const router = useRouter()
const { items, load, create, remove, isInUse } = useClients()
const { items: areas, load: loadAreas } = useAreas()
const { addToast } = useToast()

const showModal = ref(false)
const deleteTarget = ref(null)
const macError = ref('')
const emptyForm = () => ({ name: '', description: '', area: '', mac: '', uuid: '' })
const form = ref(emptyForm())

const sortedItems = computed(() =>
  [...items.value].sort((a, b) => a.name.localeCompare(b.name, undefined, { sensitivity: 'base' }))
)

onMounted(() => Promise.all([load(), loadAreas()]))

function hidSummary(hids) {
  const all = hids || []
  const buttons = all.filter(h => h.type !== 'switch').length
  const switches = all.filter(h => h.type === 'switch').length
  const parts = []
  if (buttons > 0) parts.push(`${buttons} ${buttons === 1 ? 'button' : 'buttons'}`)
  if (switches > 0) parts.push(`${switches} ${switches === 1 ? 'switch' : 'switches'}`)
  return parts.length ? parts.join(', ') : 'No buttons or switches'
}

function openAdd() {
  form.value = emptyForm()
  macError.value = ''
  showModal.value = true
}

function generateMac() {
  form.value.mac = 'ff:ff:ff:ff:ff:ff'
  macError.value = ''
}

async function save() {
  macError.value = ''
  const mac = form.value.mac.toLowerCase()
  if (!MAC_RE.test(mac)) {
    macError.value = 'Enter a valid MAC address (e.g. aa:bb:cc:dd:ee:ff)'
    return
  }
  try {
    await create({ ...form.value, mac })
    showModal.value = false
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}

async function confirmDelete(client) {
  try {
    if (await isInUse(client.id)) {
      addToast('warning', `Client "${client.name}" is assigned to a controller input and cannot be deleted.`)
      return
    }
    deleteTarget.value = client
  } catch (e) {
    addToast('error', `Could not check usage: ${e.message}`)
  }
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
