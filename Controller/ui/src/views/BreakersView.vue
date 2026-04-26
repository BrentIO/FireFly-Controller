<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Breakers</h1>
      <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors print:hidden" @click="openAdd">
        Add Breaker
      </button>
    </div>

    <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-x-auto">
      <table class="w-full text-sm">
        <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
          <tr>
            <th class="px-4 py-3 text-left">Name</th>
            <th class="px-4 py-3 text-right">Rating (Amps)</th>
            <th class="px-4 py-3 text-right">Utilization</th>
            <th class="px-4 py-3 text-right print:hidden">Actions</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
          <tr v-if="items.length === 0">
            <td colspan="4" class="px-4 py-8 text-center text-gray-400 dark:text-gray-500">No breakers defined.</td>
          </tr>
          <tr v-for="b in enrichedBreakers" :key="b.id" class="hover:bg-gray-50 dark:hover:bg-gray-800/50 break-inside-avoid">
            <td class="px-4 py-3 text-gray-900 dark:text-gray-100 font-medium">{{ b.name }}</td>
            <td class="px-4 py-3 text-right text-gray-600 dark:text-gray-400">{{ b.amperage }}</td>
            <td class="px-4 py-3 text-right">
              <span :class="utilizationClass(b.pct)" class="inline-flex items-center justify-end gap-1">
                <svg v-if="b.pct > 60 && b.pct <= 80" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor" class="w-4 h-4 shrink-0">
                  <path fill-rule="evenodd" d="M8.485 2.495c.673-1.167 2.357-1.167 3.03 0l6.28 10.875c.673 1.167-.17 2.625-1.516 2.625H3.72c-1.347 0-2.189-1.458-1.515-2.625L8.485 2.495zM10 5a.75.75 0 01.75.75v3.5a.75.75 0 01-1.5 0v-3.5A.75.75 0 0110 5zm0 9a1 1 0 100-2 1 1 0 000 2z" clip-rule="evenodd" />
                </svg>
                <svg v-else-if="b.pct > 80" xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor" class="w-4 h-4 shrink-0">
                  <path fill-rule="evenodd" d="M18 10a8 8 0 11-16 0 8 8 0 0116 0zm-8-5a.75.75 0 01.75.75v4.5a.75.75 0 01-1.5 0v-4.5A.75.75 0 0110 5zm0 10a1 1 0 100-2 1 1 0 000 2z" clip-rule="evenodd" />
                </svg>
                {{ b.loadA }}A / {{ b.pct }}%
              </span>
            </td>
            <td class="px-4 py-3 text-right print:hidden">
              <div class="flex justify-end gap-2">
                <button class="px-2.5 py-1 text-xs font-medium rounded border border-gray-300 dark:border-gray-600 text-gray-700 dark:text-gray-300 hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="openEdit(b)">Edit</button>
                <button class="px-2.5 py-1 text-xs font-medium rounded border border-red-200 dark:border-red-800 text-red-600 dark:text-red-400 hover:bg-red-50 dark:hover:bg-red-900/20 transition-colors" @click="confirmDelete(b)">Delete</button>
              </div>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50" @click.self="showModal = false">
          <div class="w-full max-w-sm bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6">
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">{{ editing ? 'Edit Breaker' : 'Add Breaker' }}</h3>
            <form @submit.prevent="save">
              <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Name</label>
              <input v-model="form.name" type="text" maxlength="30" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500 mb-4" />
              <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Amperage Rating</label>
              <input v-model.number="form.amperage" type="number" min="1" max="100" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500 mb-4" />
              <div class="flex gap-3 justify-end">
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
      title="Delete Breaker"
      :message="`Delete breaker '${deleteTarget?.name}'? This cannot be undone.`"
      confirm-label="Delete"
      @confirm="doDelete"
      @cancel="deleteTarget = null"
    />
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useBreakers } from '../composables/useBreakers'
import { useCircuits } from '../composables/useCircuits'
import { useToast } from '../composables/useToast'

const { items, load, create, update, remove, isInUse } = useBreakers()
const { items: circuits, load: loadCircuits } = useCircuits()
const { addToast } = useToast()

const showModal = ref(false)
const editing = ref(null)
const form = ref({ name: '', amperage: 20 })
const deleteTarget = ref(null)

const enrichedBreakers = computed(() => items.value.map(b => {
  const assigned = circuits.value.filter(c => c.breaker === b.id)
  const loadA = assigned.reduce((sum, c) => sum + Number(c.load_amperage || 0), 0)
  const pct = b.amperage ? Math.round((loadA / b.amperage) * 100) : 0
  return { ...b, loadA, pct }
}))

onMounted(async () => {
  await Promise.all([load(), loadCircuits()])
})

function openAdd() {
  editing.value = null
  form.value = { name: '', amperage: 20 }
  showModal.value = true
}

function openEdit(b) {
  editing.value = b
  form.value = { name: b.name, amperage: b.amperage }
  showModal.value = true
}

function utilizationClass(pct) {
  if (pct >= 100) return 'text-fuchsia-600 dark:text-fuchsia-400 font-semibold'
  if (pct > 80) return 'text-red-600 dark:text-red-400 font-semibold'
  if (pct > 60) return 'text-yellow-500 dark:text-yellow-400 font-semibold'
  return 'text-gray-600 dark:text-gray-400'
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

async function confirmDelete(b) {
  if (await isInUse(b.id)) {
    addToast('warning', `Breaker "${b.name}" has circuits assigned and cannot be deleted.`)
    return
  }
  deleteTarget.value = b
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
