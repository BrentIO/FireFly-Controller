<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Tags</h1>
      <button
        class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors print:hidden"
        @click="openAdd"
      >
        Add Tag
      </button>
    </div>

    <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-x-auto">
      <table class="w-full text-sm">
        <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
          <tr>
            <th class="px-4 py-3 text-left">Name</th>
            <th class="px-4 py-3 text-right print:hidden">Actions</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
          <tr v-if="items.length === 0">
            <td colspan="2" class="px-4 py-8 text-center text-gray-400 dark:text-gray-500">No tags defined.</td>
          </tr>
          <tr v-for="tag in items" :key="tag.id" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
            <td class="px-4 py-3 text-gray-900 dark:text-gray-100 font-medium">{{ tag.name }}</td>
            <td class="px-4 py-3 text-right print:hidden">
              <div class="flex justify-end gap-2">
                <button class="px-2.5 py-1 text-xs font-medium rounded border border-gray-300 dark:border-gray-600 text-gray-700 dark:text-gray-300 hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="openEdit(tag)">Edit</button>
                <button class="px-2.5 py-1 text-xs font-medium rounded border border-red-200 dark:border-red-800 text-red-600 dark:text-red-400 hover:bg-red-50 dark:hover:bg-red-900/20 transition-colors" @click="confirmDelete(tag)">Delete</button>
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
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">{{ editing ? 'Edit Tag' : 'Add Tag' }}</h3>
            <form @submit.prevent="save">
              <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Name</label>
              <input v-model="form.name" type="text" maxlength="20" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500 mb-4" />
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
      title="Delete Tag"
      :message="`Delete tag '${deleteTarget?.name}'? This cannot be undone.`"
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
import { useTags } from '../../composables/useTags'
import { useToast } from '../../composables/useToast'

const { items, load, create, update, remove, isInUse } = useTags()
const { addToast } = useToast()

const showModal = ref(false)
const editing = ref(null)
const form = ref({ name: '' })
const deleteTarget = ref(null)

onMounted(load)

function openAdd() {
  editing.value = null
  form.value = { name: '' }
  showModal.value = true
}

function openEdit(tag) {
  editing.value = tag
  form.value = { name: tag.name }
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

async function confirmDelete(tag) {
  if (await isInUse(tag.id)) {
    addToast('warning', `Tag "${tag.name}" is assigned to one or more client buttons and cannot be deleted.`)
    return
  }
  deleteTarget.value = tag
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
