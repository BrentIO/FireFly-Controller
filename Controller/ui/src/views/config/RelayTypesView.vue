<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Relay Types</h1>
      <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors print:hidden" @click="openAdd">
        Add Relay Type
      </button>
    </div>

    <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-hidden">
      <table class="w-full text-sm">
        <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
          <tr>
            <th class="px-4 py-3 text-left">Manufacturer</th>
            <th class="px-4 py-3 text-left">Model</th>
            <th class="px-4 py-3 text-left">Description</th>
            <th class="px-4 py-3 text-left">Type</th>
            <th class="px-4 py-3 text-right print:hidden">Actions</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
          <tr v-if="items.length === 0">
            <td colspan="5" class="px-4 py-8 text-center text-gray-400 dark:text-gray-500">No relay types defined.</td>
          </tr>
          <tr v-for="r in sortedItems" :key="r.id" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
            <td class="px-4 py-3 text-gray-900 dark:text-gray-100 font-medium">{{ r.manufacturer }}</td>
            <td class="px-4 py-3 text-gray-900 dark:text-gray-100">{{ r.model }}</td>
            <td class="px-4 py-3 text-gray-600 dark:text-gray-400">{{ r.description }}</td>
            <td class="px-4 py-3">
              <span :class="r.type === 'BINARY'
                ? 'px-2 py-0.5 text-xs rounded-full bg-blue-100 dark:bg-blue-900/40 text-blue-700 dark:text-blue-300'
                : 'px-2 py-0.5 text-xs rounded-full bg-purple-100 dark:bg-purple-900/40 text-purple-700 dark:text-purple-300'">
                {{ r.type === 'BINARY' ? 'Binary' : 'Variable' }}
              </span>
            </td>
            <td class="px-4 py-3 text-right print:hidden">
              <template v-if="r.is_custom === 'true'">
                <div class="flex justify-end gap-2">
                  <button class="px-2.5 py-1 text-xs font-medium rounded border border-gray-300 dark:border-gray-600 text-gray-700 dark:text-gray-300 hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="openEdit(r)">Edit</button>
                  <button class="px-2.5 py-1 text-xs font-medium rounded border border-red-200 dark:border-red-800 text-red-600 dark:text-red-400 hover:bg-red-50 dark:hover:bg-red-900/20 transition-colors" @click="confirmDelete(r)">Delete</button>
                </div>
              </template>
              <span v-else class="text-xs text-gray-400 dark:text-gray-500">Built-in</span>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50" @click.self="showModal = false">
          <div class="w-full max-w-md bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6">
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">{{ editing ? 'Edit Relay Type' : 'Add Relay Type' }}</h3>
            <form @submit.prevent="save" class="space-y-4">
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Manufacturer</label>
                <input v-model="form.manufacturer" type="text" maxlength="30" required
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Model</label>
                <input v-model="form.model" type="text" maxlength="30" required
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Description</label>
                <input v-model="form.description" type="text" maxlength="40" required
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Type</label>
                <select v-model="form.type" required
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="BINARY">Binary — Relay / Contactor (Toggle only)</option>
                  <option value="VARIABLE">Variable — Dimmer / Proportional (Increase / Decrease)</option>
                </select>
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

    <ConfirmModal
      :show="!!deleteTarget"
      title="Delete Relay Type"
      :message="`Delete '${deleteTarget?.manufacturer} ${deleteTarget?.model}'? This cannot be undone.`"
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
import { useRelayModels } from '../../composables/useRelayModels'
import { useToast } from '../../composables/useToast'

const { items, load, create, update, remove, isInUse } = useRelayModels()
const { addToast } = useToast()

const showModal = ref(false)
const editing = ref(null)
const deleteTarget = ref(null)
const emptyForm = () => ({ manufacturer: '', model: '', description: '', type: 'BINARY' })
const form = ref(emptyForm())

const sortedItems = computed(() =>
  [...items.value].sort((a, b) => {
    const ma = `${a.manufacturer} ${a.model}`.toLowerCase()
    const mb = `${b.manufacturer} ${b.model}`.toLowerCase()
    return ma.localeCompare(mb)
  })
)

onMounted(load)

function openAdd() {
  editing.value = null
  form.value = emptyForm()
  showModal.value = true
}

function openEdit(r) {
  editing.value = r
  form.value = { manufacturer: r.manufacturer, model: r.model, description: r.description, type: r.type }
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

async function confirmDelete(r) {
  if (await isInUse(r.id)) {
    addToast('warning', `"${r.manufacturer} ${r.model}" is used by one or more circuits and cannot be deleted.`)
    return
  }
  deleteTarget.value = r
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
