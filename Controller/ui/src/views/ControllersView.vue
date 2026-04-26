<template>
  <AppLayout>
    <div class="flex flex-wrap items-center gap-3 justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Controllers</h1>
      <div class="flex gap-2 print:hidden flex-shrink-0">
        <button class="px-4 py-2 rounded-lg bg-green-600 hover:bg-green-700 text-white text-sm font-medium transition-colors" @click="checkConfig">
          Validate
        </button>
        <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors" @click="openAdd">
          Add Controller
        </button>
      </div>
    </div>

    <div class="grid gap-4 grid-cols-1 sm:grid-cols-2 xl:grid-cols-3">
      <div v-if="items.length === 0" class="text-gray-400 dark:text-gray-500 py-8 col-span-full">No controllers defined.</div>

      <div v-for="ctrl in items" :key="ctrl.id"
        class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-4 break-inside-avoid">
        <!-- Header -->
        <div class="flex items-start justify-between gap-2 mb-3">
          <div>
            <p class="font-semibold text-gray-900 dark:text-gray-100">{{ ctrl.name }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400 font-mono">{{ ctrl.uuid }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400">{{ ctrl.product }}</p>
          </div>
          <div class="flex gap-2 print:hidden flex-shrink-0">
            <button class="px-2.5 py-1 text-xs font-medium rounded border border-gray-300 dark:border-gray-600 text-gray-700 dark:text-gray-300 hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="openEdit(ctrl)">Edit</button>
            <button class="px-2.5 py-1 text-xs font-medium rounded border border-red-200 dark:border-red-800 text-red-600 dark:text-red-400 hover:bg-red-50 dark:hover:bg-red-900/20 transition-colors" @click="confirmDelete(ctrl)">Delete</button>
          </div>
        </div>

        <!-- Auth / deploy section -->
        <div class="print:hidden border-t border-gray-100 dark:border-gray-800 pt-3 mt-3 space-y-2">
          <div v-if="!sessions[ctrl.id]?.isAuthenticated" class="space-y-2">
            <input v-model="ipInputs[ctrl.id]" type="text" placeholder="192.168.1.x"
              class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-gray-50 dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-2 py-1.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500" />
            <div class="flex gap-2">
              <input v-model="tokenInputs[ctrl.id]" type="text" placeholder="Visual token" maxlength="8"
                class="flex-1 min-w-0 rounded-lg border border-gray-300 dark:border-gray-600 bg-gray-50 dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-2 py-1.5 text-sm font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
              <button class="px-3 py-1.5 text-sm font-medium text-white bg-blue-600 rounded-lg transition-colors flex-shrink-0" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-blue-700'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="authenticate(ctrl.id)">Connect</button>
            </div>
          </div>
          <div v-else class="flex flex-wrap items-center gap-2">
            <span class="text-xs text-green-600 dark:text-green-400 font-medium">Connected · {{ sessions[ctrl.id].ip }}</span>
            <button class="text-xs text-gray-500 hover:text-gray-700 dark:hover:text-gray-300 underline" @click="logout(ctrl.id)">Disconnect</button>
            <button class="ml-auto px-2 py-1.5 text-xs font-medium text-white bg-amber-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-amber-700'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="deploy(ctrl)">Deploy</button>
            <button class="px-2 py-1.5 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="openEventLog(ctrl.id)">Events</button>
          </div>
        </div>
      </div>
    </div>

    <!-- Add/Edit modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50" @click.self="showModal = false">
          <div class="w-full max-w-md bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6">
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">{{ editing ? 'Edit Controller' : 'Add Controller' }}</h3>
            <form @submit.prevent="save" class="space-y-4">
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Name <span class="text-xs font-normal text-gray-400">(max 20 chars)</span></label>
                <input v-model="form.name" type="text" maxlength="20" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Area</label>
                <select v-model.number="form.area" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select area…</option>
                  <option v-for="a in areas" :key="a.id" :value="a.id">{{ a.name }}</option>
                </select>
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Product</label>
                <select v-model="form.product" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select product…</option>
                  <option v-for="p in products" :key="p.pid" :value="p.pid">{{ p.pid }}</option>
                </select>
              </div>
              <div>
                <div class="flex items-center justify-between mb-1">
                  <label class="block text-sm font-medium text-gray-700 dark:text-gray-300">UUID</label>
                  <button type="button" class="text-xs text-blue-600 dark:text-blue-400 hover:underline" @click="form.uuid = randomUUID()">Generate</button>
                </div>
                <input v-model="form.uuid" type="text" required pattern="[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
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

    <!-- Validate results modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showValidateModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50" @click.self="showValidateModal = false">
          <div class="w-full max-w-lg bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6 max-h-[80vh] flex flex-col">
            <div class="flex items-center justify-between mb-4">
              <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100">Validation Results</h3>
              <button class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 text-xl leading-none" @click="showValidateModal = false">&times;</button>
            </div>
            <div class="overflow-y-auto flex-1 space-y-2">
              <div v-for="(r, i) in validateResults" :key="i"
                class="p-3 rounded-lg text-sm"
                :class="r.type === 'error' ? 'bg-red-50 dark:bg-red-900/20 text-red-800 dark:text-red-300' : 'bg-amber-50 dark:bg-amber-900/20 text-amber-800 dark:text-amber-300'">
                <span class="font-medium mr-1">{{ r.type === 'error' ? '✗ Error:' : '⚠ Warning:' }}</span>{{ r.message }}
              </div>
            </div>
            <div class="mt-4 flex justify-end">
              <button class="px-4 py-2 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors" @click="showValidateModal = false">Close</button>
            </div>
          </div>
        </div>
      </Transition>
    </Teleport>

    <!-- Event log modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showEventLog" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50" @click.self="showEventLog = false">
          <div class="w-full max-w-2xl bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6 max-h-[80vh] flex flex-col">
            <div class="flex items-center justify-between mb-4">
              <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100">Event Log</h3>
              <button class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 text-xl leading-none" @click="showEventLog = false">&times;</button>
            </div>
            <div class="overflow-y-auto flex-1">
              <table class="w-full text-xs">
                <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
                  <tr v-if="!eventLog.length">
                    <td class="py-4 text-center text-gray-400 dark:text-gray-500">No events.</td>
                  </tr>
                  <tr v-for="(e, i) in eventLog" :key="i">
                    <td class="py-1.5 pr-4 text-gray-500 dark:text-gray-400 whitespace-nowrap">{{ e.time || e.timestamp || '' }}</td>
                    <td class="py-1.5 text-gray-900 dark:text-gray-100">{{ e.message || e }}</td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
        </div>
      </Transition>
    </Teleport>

    <ConfirmModal :show="!!deleteTarget" title="Delete Controller" :message="`Delete controller '${deleteTarget?.name}'? All assignments will be lost.`"
      confirm-label="Delete" @confirm="doDelete" @cancel="deleteTarget = null" />
  </AppLayout>
</template>

<script setup>
import { ref, reactive, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useControllers } from '../composables/useControllers'
import { useAreas } from '../composables/useAreas'
import { useControllerSession } from '../composables/useControllerSession'
import { buildControllerPayload, checkConfiguration } from '../composables/usePayloads'
import { useToast } from '../composables/useToast'
import { randomUUID } from '../composables/useValidators'
import { isCloudMode } from '../composables/useCloudMode'

const { items, products, load, create, update, remove } = useControllers()
const { items: areas, load: loadAreas } = useAreas()
const { addToast } = useToast()

const showModal = ref(false)
const showEventLog = ref(false)
const showValidateModal = ref(false)
const editing = ref(null)
const deleteTarget = ref(null)
const eventLog = ref([])
const validateResults = ref([])
const emptyForm = () => ({ name: '', area: '', product: '', uuid: '' })
const form = ref(emptyForm())

const ipInputs = reactive({})
const tokenInputs = reactive({})
const sessions = reactive({})

// Separate non-reactive store for session controllers
const sessionCtrls = {}

function initSession(id) {
  if (!sessionCtrls[id]) {
    const ctrl = useControllerSession(id)
    sessionCtrls[id] = ctrl
    sessions[id] = ctrl.session
    // Pre-populate IP input from stored session
    if (ctrl.session.ip) ipInputs[id] = ctrl.session.ip
  }
}

function getSessionCtrl(id) {
  initSession(id)
  return sessionCtrls[id]
}

onMounted(async () => {
  await Promise.all([load(), loadAreas()])
  items.value.forEach(c => initSession(c.id))
})

function openAdd() {
  editing.value = null
  form.value = emptyForm()
  showModal.value = true
}

function openEdit(c) {
  editing.value = c
  form.value = { name: c.name, area: c.area, product: c.product, uuid: c.uuid }
  showModal.value = true
}

async function save() {
  try {
    if (editing.value) {
      await update(editing.value.id, form.value)
      addToast('success', 'Controller updated.')
    } else {
      await create(form.value)
      addToast('success', 'Controller added.')
    }
    showModal.value = false
    await load()
    items.value.forEach(c => initSession(c.id))
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}

function confirmDelete(c) {
  const inputCount = Object.keys(c.inputs || {}).length
  const outputCount = Object.keys(c.outputs || {}).length
  if (inputCount > 0 || outputCount > 0) {
    addToast('warning', `Cannot delete '${c.name}': remove all input and output assignments first.`)
    return
  }
  deleteTarget.value = c
}

async function doDelete() {
  try {
    await remove(deleteTarget.value.id)
    addToast('success', 'Controller deleted.')
  } catch (e) {
    addToast('error', `Failed to delete: ${e.message}`)
  } finally {
    deleteTarget.value = null
  }
}

async function authenticate(id) {
  const ctrl = getSessionCtrl(id)
  ctrl.setIp(ipInputs[id] ?? '')
  ctrl.setVisualToken(tokenInputs[id] ?? '')
  try {
    await ctrl.authenticate()
    addToast('success', 'Connected.')
  } catch (e) {
    addToast('error', `Connection failed: ${e.message}`)
  }
}

function logout(id) {
  getSessionCtrl(id).logout()
}

async function deploy(ctrl) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  try {
    const payload = await buildControllerPayload(ctrl.id)
    const { controllerFetch } = await import('../composables/useApi')
    const res = await controllerFetch(sessionCtrl.session.ip, `/controllers/${ctrl.uuid}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    }, sessionCtrl.session.visualToken)
    if (res.ok || res.status === 204) {
      addToast('success', `Deployed to ${ctrl.name}.`)
    } else {
      addToast('error', `Deploy failed: HTTP ${res.status}`)
    }
  } catch (e) {
    addToast('error', `Deploy error: ${e.message}`)
  }
}

async function openEventLog(id) {
  const sessionCtrl = getSessionCtrl(id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/events', {}, sessionCtrl.session.visualToken)
    eventLog.value = res.ok ? await res.json() : []
  } catch {
    eventLog.value = []
  }
  showEventLog.value = true
}

async function checkConfig() {
  validateResults.value = []
  const errors = await checkConfiguration()
  if (!errors.length) {
    addToast('success', 'Configuration is valid.')
    return
  }
  validateResults.value = errors.map(e => {
    if (typeof e === 'string') {
      return { type: e.toLowerCase().includes('error') ? 'error' : 'warning', message: e }
    }
    return { type: e.type || 'warning', message: e.message || String(e) }
  })
  showValidateModal.value = true
}
</script>
