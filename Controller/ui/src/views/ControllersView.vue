<template>
  <AppLayout>
    <div class="flex flex-wrap items-center gap-3 justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl print:!text-black">Controllers</h1>
      <div class="flex gap-2 print:hidden flex-shrink-0">
        <button class="px-4 py-2 rounded-lg bg-gray-100 hover:bg-gray-200 dark:bg-gray-800 dark:hover:bg-gray-700 text-gray-700 dark:text-gray-200 text-sm font-medium transition-colors" @click="printLandscape">Print</button>
        <button class="px-4 py-2 rounded-lg bg-amber-600 text-white text-sm font-medium transition-colors" :class="hasConnectedControllers ? 'hover:bg-amber-700' : 'opacity-40 cursor-not-allowed'" :disabled="!hasConnectedControllers" @click="deployAll">Deploy All</button>
        <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors" @click="openAdd">Add Controller</button>
      </div>
    </div>

    <!-- Print-only table -->
    <div class="hidden print:block mb-6">
      <table class="w-full text-xs text-black">
        <thead class="uppercase tracking-wider border-b border-black">
          <tr>
            <th class="py-2 text-left font-semibold">Name</th>
            <th class="py-2 text-left font-semibold">Area</th>
            <th class="py-2 text-left font-semibold">Product ID</th>
            <th class="py-2 text-left font-semibold">UUID</th>
            <th class="py-2 text-left font-semibold">MAC Address</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-gray-300">
          <tr v-for="ctrl in items" :key="ctrl.id">
            <td class="py-2 font-semibold">{{ ctrl.name }}</td>
            <td class="py-2">{{ areaName(ctrl.area) }}</td>
            <td class="py-2">{{ ctrl.product }}</td>
            <td class="py-2 font-mono">{{ ctrl.uuid }}</td>
            <td class="py-2 font-mono">{{ ctrl.mac || '—' }}</td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="grid gap-4 grid-cols-1 sm:grid-cols-2 xl:grid-cols-3 print:hidden">
      <div v-if="items.length === 0" class="text-gray-400 dark:text-gray-500 py-8 col-span-full">No controllers defined.</div>

      <div v-for="ctrl in items" :key="ctrl.id"
        class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-4 break-inside-avoid">
        <!-- Header -->
        <div class="flex items-start justify-between gap-2 mb-3">
          <div>
            <p class="font-semibold text-gray-900 dark:text-gray-100">{{ ctrl.name }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400 font-mono">{{ ctrl.uuid }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400 font-mono">{{ ctrl.mac || '—' }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400">{{ ctrl.product }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400">{{ areaName(ctrl.area) }}</p>
            <p v-if="!ctrl.mac || ctrl.mac === 'ff:ff:ff:ff:ff:ff'" class="text-xs text-yellow-700 dark:text-yellow-500 mt-1 font-medium">MAC Address is invalid</p>
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
          <div v-else class="space-y-2">
            <div class="flex flex-wrap items-center gap-2">
              <span class="text-xs text-green-600 dark:text-green-400 font-medium">Connected · {{ sessions[ctrl.id].ip }}</span>
              <button class="text-xs text-gray-500 hover:text-gray-700 dark:hover:text-gray-300 underline" @click="logout(ctrl.id)">Disconnect</button>
              <button class="ml-auto px-2 py-1.5 text-xs font-medium text-white bg-amber-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-amber-700'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="deploy(ctrl)">Deploy</button>
              <button class="px-2 py-1.5 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="openEventLog(ctrl.id)">Events</button>
            </div>
            <div class="flex flex-wrap gap-2">
              <button class="px-2 py-1.5 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="openErrorLog(ctrl.id)">Errors</button>
              <button class="px-2 py-1.5 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmPullBackup(ctrl)">Pull Backup</button>
              <button class="px-2 py-1.5 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="pushCertificates(ctrl)">Push Certs</button>
              <button class="px-2 py-1.5 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="toggleProvisioning(ctrl.id)">{{ sessions[ctrl.id]?.provisioningModeEnabled ? 'Disable Provisioning' : 'Enable Provisioning' }}</button>
              <button class="px-2 py-1.5 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmOtaApp(ctrl)">Force App Update</button>
              <button class="px-2 py-1.5 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmOtaSpiffs(ctrl)">Force FS Update</button>
            </div>
            <div class="flex flex-wrap gap-2">
              <button class="px-2 py-1.5 text-xs font-medium text-blue-700 dark:text-blue-300 border border-blue-300 dark:border-blue-700 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-blue-50 dark:hover:bg-blue-900/20'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="pushCloudBackup(ctrl)">Push Cloud Backup</button>
              <button class="px-2 py-1.5 text-xs font-medium text-blue-700 dark:text-blue-300 border border-blue-300 dark:border-blue-700 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-blue-50 dark:hover:bg-blue-900/20'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmCloudRestore(ctrl)">Restore Cloud Backup</button>
              <button class="px-2 py-1.5 text-xs font-medium text-red-700 dark:text-red-300 border border-red-300 dark:border-red-700 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-red-50 dark:hover:bg-red-900/20'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmCloudDelete(ctrl)">Delete Cloud Backup</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Add/Edit modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
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
                  <label class="block text-sm font-medium text-gray-700 dark:text-gray-300">MAC Address</label>
                  <button type="button" class="text-xs text-blue-600 dark:text-blue-400 hover:underline" @click="generateMac">Generate</button>
                </div>
                <input v-model="form.mac" type="text" placeholder="aa:bb:cc:dd:ee:ff"
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base font-mono focus:outline-none focus:ring-2 focus:ring-blue-500"
                  :class="macError ? 'border-red-400 focus:ring-red-400' : ''" />
                <p v-if="macError" class="mt-1 text-xs text-red-600 dark:text-red-400">{{ macError }}</p>
                <div v-if="form.mac === 'ff:ff:ff:ff:ff:ff'" class="mt-1 flex items-start gap-1.5 text-xs text-yellow-600 dark:text-yellow-400">
                  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor" class="w-3.5 h-3.5 flex-shrink-0 mt-px"><path fill-rule="evenodd" d="M8.485 2.495c.673-1.167 2.357-1.167 3.03 0l6.28 10.875c.673 1.167-.17 2.625-1.516 2.625H3.72c-1.347 0-2.189-1.458-1.515-2.625L8.485 2.495zM10 5a.75.75 0 01.75.75v3.5a.75.75 0 01-1.5 0v-3.5A.75.75 0 0110 5zm0 9a1 1 0 100-2 1 1 0 000 2z" clip-rule="evenodd" /></svg>
                  <span>Placeholder MAC — update before provisioning.</span>
                </div>
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


    <!-- Event log modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showEventLog" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
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

    <!-- Error log modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showErrorLog" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
          <div class="w-full max-w-2xl bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6 max-h-[80vh] flex flex-col">
            <div class="flex items-center justify-between mb-4">
              <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100">Error Log</h3>
              <div class="flex items-center gap-3">
                <button class="px-3 py-1 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="refreshErrorLog">Refresh</button>
                <button class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 text-xl leading-none" @click="showErrorLog = false">&times;</button>
              </div>
            </div>
            <div class="overflow-y-auto flex-1">
              <table class="w-full text-xs">
                <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
                  <tr v-if="!errorLog.length">
                    <td class="py-4 text-center text-gray-400 dark:text-gray-500">No errors.</td>
                  </tr>
                  <tr v-for="(e, i) in errorLog" :key="i">
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

    <ConfirmModal :show="!!pullBackupTarget" title="Pull Backup" :message="`Pull backup from '${pullBackupTarget?.name}'? This will replace all local configuration data and cannot be undone.`"
      variant="warning" confirm-label="Pull Backup" @confirm="doPullBackup" @cancel="pullBackupTarget = null" />

    <ConfirmModal :show="!!cloudRestoreTarget" title="Restore Cloud Backup" :message="`Restore cloud backup to '${cloudRestoreTarget?.name}'? The device will retrieve and decrypt the backup from the cloud.`"
      variant="warning" confirm-label="Restore" @confirm="doCloudRestore" @cancel="cloudRestoreTarget = null" />

    <ConfirmModal :show="showLoadBackupPrompt" title="Load Backup Now?" message="Load the retrieved cloud backup into the app? This will replace all local configuration data and cannot be undone."
      variant="warning" confirm-label="Load Backup" @confirm="doLoadCloudBackup" @cancel="showLoadBackupPrompt = false" />

    <ConfirmModal :show="!!cloudDeleteTarget" title="Delete Cloud Backup" :message="`Delete the cloud backup for '${cloudDeleteTarget?.name}'? This is permanent and cannot be undone.`"
      variant="danger" confirm-label="Delete" @confirm="doCloudDelete" @cancel="cloudDeleteTarget = null" />

    <ConfirmModal :show="!!otaAppTarget" title="Force Application Update" :message="`Force firmware update on '${otaAppTarget?.name}'? The controller will reboot after downloading the update.`"
      variant="warning" confirm-label="Force Update" @confirm="doOtaApp" @cancel="otaAppTarget = null" />

    <ConfirmModal :show="!!otaSpiffsTarget" title="Force Filesystem Update" :message="`Force filesystem update on '${otaSpiffsTarget?.name}'? The controller will reboot after downloading the update.`"
      variant="warning" confirm-label="Force Update" @confirm="doOtaSpiffs" @cancel="otaSpiffsTarget = null" />
  </AppLayout>
</template>

<script setup>
import { ref, reactive, computed, onMounted } from 'vue'
import { importDB } from 'dexie-export-import'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useControllers } from '../composables/useControllers'
import { useAreas } from '../composables/useAreas'
import { useControllerSession } from '../composables/useControllerSession'
import { buildControllerPayload, buildClientPayload, getExtendedClientIds } from '../composables/usePayloads'
import { useToast } from '../composables/useToast'
import { randomUUID } from '../composables/useValidators'

const MAC_RE = /^[0-9A-Fa-f]{2}[:-]([0-9A-Fa-f]{2}[:-]){4}[0-9A-Fa-f]{2}$/
import { isCloudMode } from '../composables/useCloudMode'
import { db } from '../composables/useDatabase'

const { items, products, load, create, update, remove } = useControllers()
const { items: areas, load: loadAreas } = useAreas()
const { addToast } = useToast()

const showModal = ref(false)
const showEventLog = ref(false)
const showErrorLog = ref(false)
const editing = ref(null)
const deleteTarget = ref(null)
const pullBackupTarget = ref(null)
const otaAppTarget = ref(null)
const otaSpiffsTarget = ref(null)
const cloudRestoreTarget = ref(null)
const cloudRestorePayload = ref(null)
const showLoadBackupPrompt = ref(false)
const cloudDeleteTarget = ref(null)
const eventLog = ref([])
const errorLog = ref([])
const activeErrorLogId = ref(null)
const macError = ref('')
const emptyForm = () => ({ name: '', area: '', product: '', mac: '', uuid: '' })
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

function areaName(id) { return areas.value.find(a => a.id === id)?.name ?? '—' }

function printLandscape() {
  const prev = document.title
  document.title = 'Controllers'
  const style = document.createElement('style')
  style.textContent = '@page { size: landscape; }'
  document.head.appendChild(style)
  window.addEventListener('afterprint', () => {
    document.title = prev
    document.head.removeChild(style)
  }, { once: true })
  window.print()
}

function openAdd() {
  editing.value = null
  form.value = emptyForm()
  macError.value = ''
  showModal.value = true
}

function openEdit(c) {
  editing.value = c
  form.value = { name: c.name, area: c.area, product: c.product, mac: c.mac ?? '', uuid: c.uuid }
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
    if (editing.value) {
      await update(editing.value.id, { ...form.value, mac })
    } else {
      await create({ ...form.value, mac })
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
    // Fetch provisioning state after authenticating
    await fetchProvisioningState(id)
  } catch (e) {
    addToast('error', `Connection failed: ${e.message}`)
  }
}

function logout(id) {
  getSessionCtrl(id).logout()
}

async function deploy(ctrl) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const payload = await buildControllerPayload(ctrl.id)
    const res = await controllerFetch(sessionCtrl.session.ip, `/controllers/${ctrl.uuid}`, {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    }, sessionCtrl.session.visualToken)
    if (!res.ok && res.status !== 204) {
      addToast('error', `Deploy failed: HTTP ${res.status}`)
      return
    }

    const secondaryIds = new Set(await getExtendedClientIds())
    const allClients = await db.clients.toArray()
    for (const client of allClients.filter(c => !secondaryIds.has(c.id))) {
      const clientPayload = await buildClientPayload(client.id)
      const clientRes = await controllerFetch(sessionCtrl.session.ip, `/clients/${client.uuid}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(clientPayload)
      }, sessionCtrl.session.visualToken)
      if (!clientRes.ok && clientRes.status !== 204) {
        addToast('error', `Deploy: failed to push client '${client.name}' (HTTP ${clientRes.status})`)
        return
      }
    }

    addToast('success', `Deployed to ${ctrl.name}.`)
  } catch (e) {
    addToast('error', `Deploy error: ${e.message}`)
  }
}

async function deployAll() {
  const connected = items.value.filter(c => sessions[c.id]?.isAuthenticated)
  if (connected.length === 0) {
    addToast('warning', 'No controllers are connected.')
    return
  }
  for (const ctrl of connected) {
    await deploy(ctrl)
  }
}

const hasConnectedControllers = computed(() => items.value.some(c => sessions[c.id]?.isAuthenticated))

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

// --- Error Log ---

async function openErrorLog(id) {
  activeErrorLogId.value = id
  await fetchErrorLog(id)
  showErrorLog.value = true
}

async function fetchErrorLog(id) {
  const sessionCtrl = getSessionCtrl(id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/errors', {}, sessionCtrl.session.visualToken)
    errorLog.value = res.ok ? await res.json() : []
  } catch {
    errorLog.value = []
  }
}

async function refreshErrorLog() {
  if (activeErrorLogId.value) {
    await fetchErrorLog(activeErrorLogId.value)
  }
}

// --- Pull Backup ---

function confirmPullBackup(ctrl) {
  pullBackupTarget.value = ctrl
}

async function doPullBackup() {
  const ctrl = pullBackupTarget.value
  pullBackupTarget.value = null
  if (!ctrl) return

  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { session } = sessionCtrl
  try {
    const TIMEOUT = 5000
    const abortCtrl = new AbortController()
    const timerId = setTimeout(() => abortCtrl.abort(), TIMEOUT)
    const res = await fetch(`http://${session.ip}/backup`, {
      headers: { 'visual-token': session.visualToken },
      signal: abortCtrl.signal
    })
    clearTimeout(timerId)

    if (!res.ok) {
      addToast('error', `Pull backup failed: HTTP ${res.status}`)
      return
    }

    const data = await res.json()

    if (data.formatName !== 'dexie') {
      addToast('error', 'Pull backup failed: response is not a valid Dexie export.')
      return
    }
    if (data.data?.databaseName !== 'FireFly-Controller') {
      addToast('error', 'Pull backup failed: backup was not exported from FireFly Controller.')
      return
    }

    const blob = new Blob([JSON.stringify(data)], { type: 'application/json' })
    const file = new File([blob], 'backup.json', { type: 'application/json' })

    await db.delete()
    await importDB(file)
    addToast('success', `Backup pulled from ${ctrl.name}. Reloading…`)
    setTimeout(() => window.location.reload(), 1500)
  } catch (e) {
    addToast('error', `Pull backup error: ${e.message}`)
  }
}

// --- Push Certificates ---

async function pushCertificates(ctrl) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { session } = sessionCtrl
  const TIMEOUT = 10000

  try {
    const allCerts = await db.certificates.toArray()
    const certsToSync = allCerts.filter(c => c.isController || c.isClient)
    if (certsToSync.length === 0) {
      addToast('warning', 'No certificates with a type assigned to push.')
      return
    }

    // Fetch the current list of certs on the controller
    let abortCtrl = new AbortController()
    let timerId = setTimeout(() => abortCtrl.abort(), TIMEOUT)
    const listRes = await fetch(`http://${session.ip}/certs`, {
      headers: { 'visual-token': session.visualToken },
      signal: abortCtrl.signal
    })
    clearTimeout(timerId)

    if (!listRes.ok) {
      addToast('error', `Push certs failed: could not read controller cert list (HTTP ${listRes.status})`)
      return
    }

    const existing = await listRes.json()
    const existingNames = new Set(Array.isArray(existing) ? existing.map(c => c.file ?? c.filename ?? c.name ?? c) : [])

    let pushed = 0
    let skipped = 0

    for (const cert of certsToSync) {
      if (existingNames.has(cert.fileName)) {
        skipped++
        continue
      }

      const certType = cert.isController && cert.isClient ? 'both'
        : cert.isController ? 'controller'
        : 'client'

      const formData = new FormData()
      const blob = new Blob([cert.certificate], { type: 'application/x-x509-ca-cert' })
      formData.append('file', blob, cert.fileName)

      abortCtrl = new AbortController()
      timerId = setTimeout(() => abortCtrl.abort(), TIMEOUT)
      const uploadRes = await fetch(`http://${session.ip}/certs`, {
        method: 'POST',
        headers: { 'visual-token': session.visualToken, 'X-Cert-Type': certType },
        body: formData,
        signal: abortCtrl.signal
      })
      clearTimeout(timerId)

      if (uploadRes.status === 201 || uploadRes.status === 403) {
        // 403 means already exists — treat as success
        pushed++
      } else {
        addToast('error', `Push certs: failed to upload '${cert.fileName}' (HTTP ${uploadRes.status})`)
        return
      }
    }

    addToast('success', `Certificates pushed to ${ctrl.name}: ${pushed} uploaded, ${skipped} already present.`)
  } catch (e) {
    addToast('error', `Push certs error: ${e.message}`)
  }
}

// --- Provisioning Mode ---

async function fetchProvisioningState(id) {
  const sessionCtrl = getSessionCtrl(id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/provisioning', {}, sessionCtrl.session.visualToken)
    if (res.ok) {
      const data = await res.json()
      sessionCtrl.session.provisioningModeEnabled = !!data.enabled
      sessionCtrl.save()
    }
  } catch {
    // Non-fatal — leave current state unchanged
  }
}

async function toggleProvisioning(id) {
  const sessionCtrl = getSessionCtrl(id)
  const { controllerFetch } = await import('../composables/useApi')
  const currentlyEnabled = sessionCtrl.session.provisioningModeEnabled
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/provisioning', {
      method: currentlyEnabled ? 'DELETE' : 'PUT'
    }, sessionCtrl.session.visualToken)
    if (res.status === 202) {
      sessionCtrl.session.provisioningModeEnabled = !currentlyEnabled
      sessionCtrl.save()
      addToast('success', `Provisioning mode ${!currentlyEnabled ? 'enabled' : 'disabled'}.`)
    } else {
      addToast('error', `Provisioning toggle failed: HTTP ${res.status}`)
    }
  } catch (e) {
    addToast('error', `Provisioning error: ${e.message}`)
  }
}

// --- OTA ---

function confirmOtaApp(ctrl) {
  otaAppTarget.value = ctrl
}

function confirmOtaSpiffs(ctrl) {
  otaSpiffsTarget.value = ctrl
}

async function doOtaApp() {
  const ctrl = otaAppTarget.value
  otaAppTarget.value = null
  if (!ctrl) return
  await triggerOta(ctrl, '/ota/app')
}

async function doOtaSpiffs() {
  const ctrl = otaSpiffsTarget.value
  otaSpiffsTarget.value = null
  if (!ctrl) return
  await triggerOta(ctrl, '/ota/spiffs')
}

async function triggerOta(ctrl, endpoint) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')

  // Read OTA URL from settings
  const settingKey = endpoint.includes('app') ? 'ota_controller' : 'ota_controller'
  const otaSetting = await db.settings.where({ setting: settingKey }).first()
  if (!otaSetting || !otaSetting.value?.url) {
    addToast('error', 'OTA URL is not configured. Set it in Settings > OTA.')
    return
  }

  const url = `${otaSetting.value.protocol ?? 'https'}://${otaSetting.value.url}`

  try {
    const res = await controllerFetch(sessionCtrl.session.ip, endpoint, {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify({ url })
    }, sessionCtrl.session.visualToken)

    if (res.status === 202) {
      addToast('success', `OTA update initiated on ${ctrl.name}.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `OTA failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `OTA error: ${e.message}`)
  }
}

// --- Cloud Backup ---

async function pushCloudBackup(ctrl) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/cloud-backup', {
      method: 'POST'
    }, sessionCtrl.session.visualToken)
    if (res.ok) {
      let statusCode = ''
      try { const body = await res.json(); statusCode = ` (cloud: ${body.status})` } catch { /* ignore */ }
      addToast('success', `Cloud backup pushed for ${ctrl.name}.${statusCode}`)
    } else if (res.status === 404) {
      addToast('warning', `No local backup.json found on ${ctrl.name}.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `Push cloud backup failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `Push cloud backup error: ${e.message}`)
  }
}

function confirmCloudRestore(ctrl) {
  cloudRestoreTarget.value = ctrl
}

async function doCloudRestore() {
  const ctrl = cloudRestoreTarget.value
  cloudRestoreTarget.value = null
  if (!ctrl) return
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/cloud-backup', {
      method: 'GET'
    }, sessionCtrl.session.visualToken)
    if (res.ok) {
      const payload = await res.text()
      cloudRestorePayload.value = payload
      showLoadBackupPrompt.value = true
    } else if (res.status === 404) {
      addToast('warning', `No cloud backup found for ${ctrl.name}.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `Restore cloud backup failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `Restore cloud backup error: ${e.message}`)
  }
}

async function doLoadCloudBackup() {
  showLoadBackupPrompt.value = false
  const payload = cloudRestorePayload.value
  cloudRestorePayload.value = null
  if (!payload) return
  try {
    let parsed
    try { parsed = JSON.parse(payload) } catch {
      addToast('error', 'Import failed: retrieved backup is not valid JSON.')
      return
    }
    if (parsed.formatName !== 'dexie') {
      addToast('error', 'Import failed: retrieved backup is not a valid Dexie export.')
      return
    }
    await db.delete()
    const blob = new Blob([payload], { type: 'application/json' })
    await importDB(blob)
    addToast('success', 'Cloud backup loaded! Reloading…')
    setTimeout(() => window.location.reload(), 1500)
  } catch (e) {
    addToast('error', `Import failed: ${e.message}`)
  }
}

function confirmCloudDelete(ctrl) {
  cloudDeleteTarget.value = ctrl
}

async function doCloudDelete() {
  const ctrl = cloudDeleteTarget.value
  cloudDeleteTarget.value = null
  if (!ctrl) return
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/cloud-backup', {
      method: 'DELETE'
    }, sessionCtrl.session.visualToken)
    if (res.status === 204) {
      addToast('success', `Cloud backup deleted for ${ctrl.name}.`)
    } else if (res.status === 404) {
      addToast('warning', `No cloud backup found for ${ctrl.name}.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `Delete cloud backup failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `Delete cloud backup error: ${e.message}`)
  }
}

</script>
