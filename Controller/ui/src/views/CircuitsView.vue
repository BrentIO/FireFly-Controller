<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Circuits</h1>
      <div class="flex gap-2 print:hidden">
        <button class="px-4 py-2 rounded-lg bg-gray-100 hover:bg-gray-200 dark:bg-gray-800 dark:hover:bg-gray-700 text-gray-700 dark:text-gray-200 text-sm font-medium transition-colors" onclick="window.print()">Print</button>
        <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors" @click="openAdd">Add Circuit</button>
      </div>
    </div>

    <div class="grid gap-4 grid-cols-1 sm:grid-cols-[repeat(auto-fill,minmax(22rem,1fr))]">
      <div v-if="enriched.length === 0" class="text-gray-400 dark:text-gray-500 py-8">No circuits defined.</div>
      <div v-for="c in enriched" :key="c.id"
        class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-4 break-inside-avoid">
        <div class="flex items-start justify-between gap-2 mb-2">
          <div>
            <p class="font-mono text-xs text-gray-500 dark:text-gray-400">{{ c.name }}</p>
            <p class="font-semibold text-gray-900 dark:text-gray-100">{{ c.description }}</p>
          </div>
          <div class="flex gap-2 print:hidden flex-shrink-0">
            <button class="text-blue-600 hover:text-blue-700 dark:text-blue-400 text-sm" @click="openEdit(c)">Edit</button>
            <button class="text-red-600 hover:text-red-700 dark:text-red-400 text-sm" @click="confirmDelete(c)">Delete</button>
          </div>
        </div>
        <dl class="grid grid-cols-2 gap-x-4 gap-y-1 text-sm mt-3">
          <dt class="text-gray-500 dark:text-gray-400 print:text-black">Area</dt><dd class="text-gray-900 dark:text-gray-100 print:text-black">{{ c.areaName }}</dd>
          <dt class="text-gray-500 dark:text-gray-400 print:text-black">Breaker</dt><dd class="text-gray-900 dark:text-gray-100 truncate print:text-black">{{ c.breakerName }}</dd>
          <dt class="text-gray-500 dark:text-gray-400 print:text-black">Relay</dt><dd class="text-gray-900 dark:text-gray-100 print:text-black">{{ c.relayName }}</dd>
          <dt class="text-gray-500 dark:text-gray-400 print:text-black">Load</dt><dd class="text-gray-900 dark:text-gray-100 print:text-black">{{ c.load_amperage }}A</dd>
        </dl>
      </div>
    </div>

    <!-- Add/Edit modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50" @click.self="showModal = false">
          <div class="w-full max-w-md bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6 max-h-[90vh] overflow-y-auto">
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">{{ editing ? 'Edit Circuit' : 'Add Circuit' }}</h3>
            <form @submit.prevent="save" class="space-y-4">
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Short ID <span class="text-xs font-normal text-gray-400">(max 8 chars, no spaces)</span></label>
                <input v-model="form.name" type="text" maxlength="8" required pattern="[A-Za-z0-9~!@#$%^&*()_+=|]+"
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Description <span class="text-xs font-normal text-gray-400">(max 20 chars)</span></label>
                <input v-model="form.description" type="text" maxlength="20" required
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Area</label>
                <select v-model.number="form.area" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select area…</option>
                  <option v-for="a in areas" :key="a.id" :value="a.id">{{ a.name }}</option>
                </select>
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Breaker</label>
                <select v-model.number="form.breaker" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select breaker…</option>
                  <option v-for="b in breakers" :key="b.id" :value="b.id">{{ b.name }}</option>
                </select>
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Icon</label>
                <select v-model.number="form.icon" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select icon…</option>
                  <option v-for="i in icons" :key="i.id" :value="i.id">{{ i.name }}</option>
                </select>
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Relay Model</label>
                <select v-model.number="form.relay_model" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select relay…</option>
                  <option v-for="r in relayModels" :key="r.id" :value="r.id">{{ r.manufacturer }} {{ r.model }} — {{ r.description }}</option>
                </select>
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Load (Amps)</label>
                <input v-model.number="form.load_amperage" type="number" min="1" max="100" step="0.5" required
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
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

    <ConfirmModal :show="!!deleteTarget" title="Delete Circuit" :message="`Delete circuit '${deleteTarget?.name}'?`"
      confirm-label="Delete" @confirm="doDelete" @cancel="deleteTarget = null" />
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useCircuits } from '../composables/useCircuits'
import { useAreas } from '../composables/useAreas'
import { useBreakers } from '../composables/useBreakers'
import { useIcons } from '../composables/useIcons'
import { db } from '../composables/useDatabase'
import { useToast } from '../composables/useToast'

const { items, relayModels, load, create, update, remove, isInUse } = useCircuits()
const { items: areas, load: loadAreas } = useAreas()
const { items: breakers, load: loadBreakers } = useBreakers()
const { items: icons, load: loadIcons } = useIcons()
const { addToast } = useToast()

const showModal = ref(false)
const editing = ref(null)
const deleteTarget = ref(null)
const emptyForm = () => ({ name: '', description: '', area: '', breaker: '', icon: '', relay_model: '', load_amperage: 0 })
const form = ref(emptyForm())

const areaMap = computed(() => Object.fromEntries(areas.value.map(a => [a.id, a.name])))
const breakerMap = computed(() => Object.fromEntries(breakers.value.map(b => [b.id, b.name])))
const relayMap = computed(() => Object.fromEntries(relayModels.value.map(r => [r.id, `${r.manufacturer} ${r.model}`])))

const enriched = computed(() => items.value.map(c => ({
  ...c,
  areaName: areaMap.value[c.area] ?? '—',
  breakerName: breakerMap.value[c.breaker] ?? '—',
  relayName: relayMap.value[c.relay_model] ?? '—'
})))

onMounted(() => Promise.all([load(), loadAreas(), loadBreakers(), loadIcons()]))

function openAdd() {
  editing.value = null
  form.value = emptyForm()
  showModal.value = true
}

function openEdit(c) {
  editing.value = c
  form.value = { name: c.name, description: c.description, area: c.area, breaker: c.breaker, icon: c.icon, relay_model: c.relay_model, load_amperage: c.load_amperage }
  showModal.value = true
}

async function save() {
  try {
    if (editing.value) {
      await update(editing.value.id, form.value)
    } else {
      await create(form.value)
    }
    showModal.value = false
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}

async function confirmDelete(c) {
  if (await isInUse(c.id)) {
    addToast('warning', `Circuit "${c.name}" is assigned to a controller output or client action and cannot be deleted.`)
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
</script>
