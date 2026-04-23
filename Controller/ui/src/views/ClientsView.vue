<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Clients</h1>
      <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors print:hidden" @click="openAdd">
        Add Client
      </button>
    </div>

    <div class="grid gap-4 grid-cols-[repeat(auto-fill,minmax(20rem,1fr))]">
      <div v-if="items.length === 0" class="text-gray-400 dark:text-gray-500 py-8">No clients defined.</div>

      <div v-for="client in items" :key="client.id"
        class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-4 break-inside-avoid">
        <div class="flex items-start justify-between gap-2 mb-3">
          <div>
            <p class="font-mono text-xs text-gray-500 dark:text-gray-400">{{ client.name }}</p>
            <p class="font-semibold text-gray-900 dark:text-gray-100">{{ client.description }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400 font-mono mt-0.5">{{ client.mac }}</p>
          </div>
          <div class="flex gap-2 print:hidden flex-shrink-0">
            <RouterLink :to="`/clients/${client.id}`" class="text-blue-600 hover:text-blue-700 dark:text-blue-400 text-sm">Edit</RouterLink>
            <button class="text-red-600 hover:text-red-700 dark:text-red-400 text-sm" @click="confirmDelete(client)">Delete</button>
          </div>
        </div>
        <p class="text-xs text-gray-500 dark:text-gray-400">{{ client.hids?.length ?? 0 }} button(s)/switch(es)</p>
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
                <input v-model="form.name" type="text" maxlength="8" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Description <span class="text-xs font-normal text-gray-400">(max 20 chars)</span></label>
                <input v-model="form.description" type="text" maxlength="20" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Area</label>
                <select v-model.number="form.area" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select area…</option>
                  <option v-for="a in areas" :key="a.id" :value="a.id">{{ a.name }}</option>
                </select>
              </div>
              <div>
                <div class="flex items-center justify-between mb-1">
                  <label class="block text-sm font-medium text-gray-700 dark:text-gray-300">MAC Address</label>
                  <button type="button" class="text-xs text-blue-600 dark:text-blue-400 hover:underline" @click="generateMac">Generate</button>
                </div>
                <input v-model="form.mac" type="text" required pattern="[0-9A-Fa-f]{2}[:-]([0-9A-Fa-f]{2}[:-]){4}[0-9A-Fa-f]{2}"
                  placeholder="AA:BB:CC:DD:EE:FF"
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
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
import { ref, onMounted } from 'vue'
import { RouterLink } from 'vue-router'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useClients } from '../composables/useClients'
import { useAreas } from '../composables/useAreas'
import { useToast } from '../composables/useToast'
import { randomUUID } from '../composables/useValidators'

const { items, load, create, remove, isInUse } = useClients()
const { items: areas, load: loadAreas } = useAreas()
const { addToast } = useToast()

const showModal = ref(false)
const deleteTarget = ref(null)
const emptyForm = () => ({ name: '', description: '', area: '', mac: '', uuid: '' })
const form = ref(emptyForm())

onMounted(() => Promise.all([load(), loadAreas()]))

function openAdd() {
  form.value = emptyForm()
  showModal.value = true
}

function generateMac() {
  const hex = () => Math.floor(Math.random() * 256).toString(16).padStart(2, '0').toUpperCase()
  form.value.mac = [hex(), hex(), hex(), hex(), hex(), hex()].join(':')
}

async function save() {
  try {
    await create(form.value)
    addToast('success', 'Client added.')
    showModal.value = false
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}

async function confirmDelete(client) {
  if (await isInUse(client.id)) {
    addToast('warning', `Client "${client.name}" is assigned to a controller input and cannot be deleted.`)
    return
  }
  deleteTarget.value = client
}

async function doDelete() {
  try {
    await remove(deleteTarget.value.id)
    addToast('success', 'Client deleted.')
  } catch (e) {
    addToast('error', `Failed to delete: ${e.message}`)
  } finally {
    deleteTarget.value = null
  }
}
</script>
