<template>
  <AppLayout>
    <div v-if="!client" class="text-gray-400 dark:text-gray-500 py-8">Loading…</div>
    <template v-else>
      <div class="flex items-center justify-between mb-6 print:mb-4">
        <div>
          <RouterLink to="/clients" class="text-sm text-blue-600 dark:text-blue-400 hover:underline print:hidden">← Clients</RouterLink>
          <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mt-1 print:text-xl">
            {{ client.name }} — {{ client.description }}
          </h1>
          <p class="text-xs text-gray-500 dark:text-gray-400 font-mono">{{ client.mac }}</p>
        </div>
        <div class="flex gap-2 print:hidden">
          <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors" @click="openAddHid">
            Add Button/Switch
          </button>
        </div>
      </div>

      <!-- HID table -->
      <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-hidden mb-6">
        <table class="w-full text-sm">
          <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
            <tr>
              <th class="px-4 py-3 text-center w-10">#</th>
              <th class="px-4 py-3 text-left">Type</th>
              <th class="px-4 py-3 text-left">Color</th>
              <th class="px-4 py-3 text-left">Switch</th>
              <th class="px-4 py-3 text-left">Tags</th>
              <th class="px-4 py-3 text-center print:hidden">Enabled</th>
              <th class="px-4 py-3 text-right print:hidden">Actions</th>
            </tr>
          </thead>
          <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
            <tr v-if="!client.hids?.length">
              <td colspan="7" class="px-4 py-8 text-center text-gray-400 dark:text-gray-500">No buttons or switches defined.</td>
            </tr>
            <tr v-for="(hid, idx) in client.hids" :key="idx" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
              <td class="px-4 py-3 text-center text-gray-500 dark:text-gray-400">{{ idx + 1 }}</td>
              <td class="px-4 py-3 text-gray-900 dark:text-gray-100">{{ hid.type === 'switch' ? 'Switch' : 'Button' }}</td>
              <td class="px-4 py-3">
                <div v-if="hid.color" class="flex items-center gap-2">
                  <div class="w-5 h-5 rounded border border-gray-200 dark:border-gray-600 flex-shrink-0"
                    :style="{ backgroundColor: colorHex(hid.color) }"></div>
                  <span class="text-gray-700 dark:text-gray-300 text-xs">{{ colorName(hid.color) }}</span>
                </div>
                <span v-else class="text-gray-400 dark:text-gray-500 text-xs">—</span>
              </td>
              <td class="px-4 py-3 text-gray-600 dark:text-gray-400 text-xs">{{ hid.switch_type || 'NO' }}</td>
              <td class="px-4 py-3">
                <div class="flex flex-wrap gap-1">
                  <span v-for="tagId in (hid.tags || [])" :key="tagId"
                    class="px-1.5 py-0.5 bg-gray-100 dark:bg-gray-700 text-gray-700 dark:text-gray-300 text-xs rounded">
                    {{ tagName(tagId) }}
                  </span>
                </div>
              </td>
              <td class="px-4 py-3 text-center print:hidden">
                <input type="checkbox" :checked="hid.enabled !== false" class="rounded" @change="toggleEnabled(idx)" />
              </td>
              <td class="px-4 py-3 text-right print:hidden">
                <button v-if="idx > 0" class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 mr-1 text-xs" title="Move up" @click="moveHid(idx, idx - 1)">↑</button>
                <button v-if="idx < client.hids.length - 1" class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 mr-3 text-xs" title="Move down" @click="moveHid(idx, idx + 1)">↓</button>
                <button class="text-red-600 hover:text-red-700 dark:text-red-400 text-sm" @click="confirmRemoveHid(idx)">Remove</button>
              </td>
            </tr>
          </tbody>
        </table>
      </div>
    </template>

    <!-- Add HID modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showHidModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50" @click.self="showHidModal = false">
          <div class="w-full max-w-md bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6">
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">Add Button / Switch</h3>
            <form @submit.prevent="addHid" class="space-y-4">
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Type</label>
                <div class="flex gap-4">
                  <label class="flex items-center gap-2 cursor-pointer">
                    <input v-model="hidForm.type" type="radio" value="button" class="text-blue-600 focus:ring-blue-500" />
                    <span class="text-sm text-gray-700 dark:text-gray-300">Button</span>
                  </label>
                  <label class="flex items-center gap-2 cursor-pointer">
                    <input v-model="hidForm.type" type="radio" value="switch" class="text-blue-600 focus:ring-blue-500" />
                    <span class="text-sm text-gray-700 dark:text-gray-300">Switch</span>
                  </label>
                </div>
              </div>
              <div v-if="hidForm.type === 'button'">
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Color <span class="font-normal text-gray-400">(optional)</span></label>
                <select v-model.number="hidForm.color" class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">No color</option>
                  <option v-for="c in colors" :key="c.id" :value="c.id">{{ c.name }}</option>
                </select>
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Switch Contact</label>
                <select v-model="hidForm.switch_type" class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="NORMALLY_OPEN">Normally Open (NO)</option>
                  <option value="NORMALLY_CLOSED">Normally Closed (NC)</option>
                </select>
              </div>
              <div class="flex gap-3 justify-end pt-2">
                <button type="button" class="px-4 py-2.5 text-sm text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="showHidModal = false">Cancel</button>
                <button type="submit" class="px-4 py-2.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors">Add</button>
              </div>
            </form>
          </div>
        </div>
      </Transition>
    </Teleport>

    <ConfirmModal :show="removeHidIdx !== null" title="Remove"
      :message="`Remove HID #${(removeHidIdx ?? 0) + 1}? This cannot be undone.`"
      confirm-label="Remove" @confirm="doRemoveHid" @cancel="removeHidIdx = null" />
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import { useRoute, RouterLink } from 'vue-router'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useClients } from '../composables/useClients'
import { useColors } from '../composables/useColors'
import { useTags } from '../composables/useTags'
import { useToast } from '../composables/useToast'
import { moveArrayItem } from '../composables/useValidators'

const route = useRoute()
const { get, update } = useClients()
const { items: colors, load: loadColors } = useColors()
const { items: tags, load: loadTags } = useTags()
const { addToast } = useToast()

const client = ref(null)
const showHidModal = ref(false)
const removeHidIdx = ref(null)
const hidForm = ref({ type: 'button', color: '', switch_type: 'NORMALLY_OPEN', tags: [] })

onMounted(async () => {
  const id = Number(route.params.id)
  client.value = await get(id)
  if (client.value && !client.value.hids) client.value.hids = []
  await Promise.all([loadColors(), loadTags()])
})

function colorHex(id) {
  return colors.value.find(c => c.id === id)?.hex ?? '#ccc'
}

function colorName(id) {
  return colors.value.find(c => c.id === id)?.name ?? '?'
}

function tagName(id) {
  return tags.value.find(t => t.id === id)?.name ?? '?'
}

function openAddHid() {
  hidForm.value = { type: 'button', color: '', switch_type: 'NORMALLY_OPEN', tags: [] }
  showHidModal.value = true
}

async function addHid() {
  const hid = {
    type: hidForm.value.type,
    switch_type: hidForm.value.switch_type,
    enabled: true,
    actions: [],
    tags: []
  }
  if (hidForm.value.type === 'button' && hidForm.value.color) {
    hid.color = Number(hidForm.value.color)
  }
  client.value.hids.push(hid)
  await saveHids()
  showHidModal.value = false
  addToast('success', 'Added.')
}

async function toggleEnabled(idx) {
  client.value.hids[idx].enabled = client.value.hids[idx].enabled === false ? true : false
  await saveHids()
}

async function moveHid(from, to) {
  client.value.hids = moveArrayItem(client.value.hids, from, to)
  await saveHids()
}

function confirmRemoveHid(idx) {
  removeHidIdx.value = idx
}

async function doRemoveHid() {
  client.value.hids.splice(removeHidIdx.value, 1)
  await saveHids()
  removeHidIdx.value = null
  addToast('success', 'Removed.')
}

async function saveHids() {
  await update(client.value.id, { hids: client.value.hids })
}
</script>
