<template>
  <AppLayout>
    <div v-if="!client" class="text-gray-400 dark:text-gray-500 py-8">Loading…</div>
    <template v-else>
      <div class="flex flex-wrap items-start justify-between gap-3 mb-6 print:mb-4">
        <div>
          <RouterLink to="/clients" class="text-sm text-blue-600 dark:text-blue-400 hover:underline print:hidden">← Clients</RouterLink>
          <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mt-1 print:text-xl">
            {{ client.name }} — {{ client.description }}
          </h1>
          <p class="text-xs text-gray-500 dark:text-gray-400 font-mono">{{ client.mac }}</p>
        </div>
        <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors print:hidden flex-shrink-0" @click="openAdd">
          Add Button/Switch
        </button>
      </div>

      <div class="flex flex-col lg:flex-row gap-6">

        <!-- Switch plate SVG preview -->
        <div class="flex-shrink-0 flex flex-col items-center lg:items-start print:hidden">
          <p class="text-xs text-center text-gray-500 dark:text-gray-400 mb-2">Preview</p>
          <ClientSvg :hids="client.hids || []" :colors="colors" />
        </div>

        <!-- HID table -->
        <div class="flex-1 bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-x-auto">
          <table class="w-full text-sm">
            <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
              <tr>
                <th class="px-4 py-3 text-center w-10">#</th>
                <th class="px-4 py-3 text-left">Type</th>
                <th class="px-4 py-3 text-left">Color</th>
                <th class="px-4 py-3 text-left">Contact</th>
                <th class="px-4 py-3 text-left">Tags</th>
                <th class="px-4 py-3 text-center">Actions</th>
                <th class="px-4 py-3 text-center print:hidden">Enabled</th>
                <th class="px-4 py-3 print:hidden"></th>
              </tr>
            </thead>
            <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
              <tr v-if="!client.hids?.length">
                <td colspan="8" class="px-4 py-8 text-center text-gray-400 dark:text-gray-500">No buttons or switches defined.</td>
              </tr>
              <tr v-for="(hid, idx) in client.hids" :key="idx" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
                <td class="px-4 py-3 text-center text-gray-500 dark:text-gray-400">{{ idx + 1 }}</td>
                <td class="px-4 py-3 text-gray-900 dark:text-gray-100">{{ hid.type === 'switch' ? 'Switch' : 'Button' }}</td>
                <td class="px-4 py-3">
                  <div v-if="hid.color" class="flex items-center gap-2">
                    <div class="w-5 h-5 rounded border border-gray-200 dark:border-gray-600 flex-shrink-0" :style="{ backgroundColor: colorHex(hid.color) }"></div>
                    <span class="text-gray-700 dark:text-gray-300 text-xs">{{ colorName(hid.color) }}</span>
                  </div>
                  <span v-else class="text-gray-400 dark:text-gray-500 text-xs">—</span>
                </td>
                <td class="px-4 py-3 text-gray-600 dark:text-gray-400 text-xs">{{ formatSwitchType(hid.switch_type) }}</td>
                <td class="px-4 py-3">
                  <div class="flex flex-wrap gap-1">
                    <span v-for="tagId in (hid.tags || [])" :key="tagId"
                      class="px-1.5 py-0.5 bg-gray-100 dark:bg-gray-700 text-gray-700 dark:text-gray-300 text-xs rounded">
                      {{ tagName(tagId) }}
                    </span>
                    <span v-if="!(hid.tags || []).length" class="text-gray-400 dark:text-gray-500 text-xs">—</span>
                  </div>
                </td>
                <td class="px-4 py-3 text-center text-xs text-gray-500 dark:text-gray-400">
                  {{ (hid.actions || []).length || '—' }}
                </td>
                <td class="px-4 py-3 text-center print:hidden">
                  <input type="checkbox" :checked="hid.enabled !== false" class="rounded" @change="toggleEnabled(idx)" />
                </td>
                <td class="px-4 py-3 text-right print:hidden whitespace-nowrap">
                  <button v-if="idx > 0" class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 mr-1 text-xs" title="Move up" @click="moveHid(idx, idx - 1)">↑</button>
                  <button v-if="idx < client.hids.length - 1" class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 mr-2 text-xs" title="Move down" @click="moveHid(idx, idx + 1)">↓</button>
                  <button class="text-blue-600 hover:text-blue-700 dark:text-blue-400 text-xs mr-2" @click="openEdit(idx)">Edit</button>
                  <button class="text-red-600 hover:text-red-700 dark:text-red-400 text-xs" @click="confirmRemoveHid(idx)">Remove</button>
                </td>
              </tr>
            </tbody>
          </table>
        </div>
      </div>
    </template>

    <!-- Add/Edit HID modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showHidModal" class="fixed inset-0 z-50 flex items-start justify-center p-4 pt-10 bg-black/50 overflow-y-auto" @click.self="showHidModal = false">
          <div class="w-full max-w-lg bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6 mb-8">
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">
              {{ editIdx === null ? 'Add' : 'Edit' }} Button / Switch
            </h3>
            <form @submit.prevent="submitHid" class="space-y-4">

              <!-- Type -->
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

              <!-- Color (buttons only) -->
              <div v-if="hidForm.type === 'button'">
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Color <span class="font-normal text-gray-400">(optional)</span></label>
                <div class="relative">
                  <button type="button"
                    class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 px-3 py-2.5 text-base text-left flex items-center gap-2 focus:outline-none focus:ring-2 focus:ring-blue-500"
                    @click="showColorDropdown = !showColorDropdown">
                    <div v-if="selectedColor" class="w-5 h-5 rounded border border-gray-200 dark:border-gray-600 flex-shrink-0" :style="{ backgroundColor: selectedColor.hex }"></div>
                    <span class="text-gray-900 dark:text-gray-100 flex-1">{{ selectedColor ? selectedColor.name : 'No color' }}</span>
                    <svg class="w-4 h-4 text-gray-400 flex-shrink-0" fill="none" stroke="currentColor" viewBox="0 0 24 24">
                      <path stroke-linecap="round" stroke-linejoin="round" stroke-width="2" d="M19 9l-7 7-7-7" />
                    </svg>
                  </button>
                  <div v-if="showColorDropdown" class="absolute z-20 w-full mt-1 bg-white dark:bg-gray-800 rounded-lg border border-gray-200 dark:border-gray-700 shadow-lg overflow-hidden">
                    <button type="button" class="w-full px-3 py-2.5 text-base text-gray-500 dark:text-gray-400 hover:bg-gray-50 dark:hover:bg-gray-700 text-left"
                      @click="hidForm.color = ''; showColorDropdown = false">No color</button>
                    <button type="button" v-for="c in colors" :key="c.id"
                      class="w-full px-3 py-2.5 text-base hover:bg-gray-50 dark:hover:bg-gray-700 text-left flex items-center gap-3"
                      @click="hidForm.color = c.id; showColorDropdown = false">
                      <div class="w-5 h-5 rounded border border-gray-200 dark:border-gray-600 flex-shrink-0" :style="{ backgroundColor: c.hex }"></div>
                      <span class="text-gray-900 dark:text-gray-100">{{ c.name }}</span>
                    </button>
                  </div>
                </div>
              </div>

              <!-- Switch Contact -->
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Switch Contact</label>
                <select v-model="hidForm.switch_type" class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="NORMALLY_OPEN">Normally Open (NO)</option>
                  <option value="NORMALLY_CLOSED">Normally Closed (NC)</option>
                </select>
              </div>

              <!-- Enabled -->
              <div class="flex items-center gap-2">
                <input v-model="hidForm.enabled" type="checkbox" id="hidEnabled" class="rounded text-blue-600 focus:ring-blue-500" />
                <label for="hidEnabled" class="text-sm text-gray-700 dark:text-gray-300">Enabled</label>
              </div>

              <!-- Tags -->
              <div v-if="tags.length">
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">Tags</label>
                <div class="flex flex-wrap gap-x-4 gap-y-2">
                  <label v-for="tag in tags" :key="tag.id" class="flex items-center gap-1.5 cursor-pointer">
                    <input type="checkbox" :value="tag.id" v-model="hidForm.tags" class="rounded text-blue-600 focus:ring-blue-500" />
                    <span class="text-sm text-gray-700 dark:text-gray-300">{{ tag.name }}</span>
                  </label>
                </div>
              </div>

              <!-- Programmed Actions -->
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-2">
                  Programmed Actions
                  <span class="font-normal text-gray-400">({{ hidForm.actions.length }})</span>
                </label>

                <div v-if="hidForm.actions.length" class="space-y-1.5 mb-3">
                  <div v-for="(act, ai) in hidForm.actions" :key="ai"
                    class="flex items-center gap-2 px-3 py-2 bg-gray-50 dark:bg-gray-800 rounded-lg text-sm">
                    <span class="flex-1 text-gray-700 dark:text-gray-300 text-xs">
                      {{ circuitName(act.circuit) }}
                      <span class="text-gray-400 mx-1">·</span>
                      {{ formatChangeState(act.change_state) }}
                      <span class="text-gray-400 mx-1">·</span>
                      {{ formatAction(act.action) }}
                    </span>
                    <button type="button" class="text-red-500 hover:text-red-700 text-xs flex-shrink-0" @click="removeAction(ai)">Remove</button>
                  </div>
                </div>

                <div v-if="circuits.length" class="border border-dashed border-gray-300 dark:border-gray-600 rounded-lg p-3">
                  <p class="text-xs text-gray-500 dark:text-gray-400 mb-2">Add action</p>
                  <div class="grid grid-cols-1 gap-2 sm:grid-cols-3">
                    <select v-model="actionDraft.circuit" class="rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-2 py-1.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                      <option value="">Circuit…</option>
                      <option v-for="c in circuits" :key="c.id" :value="c.id">{{ c.name }} — {{ c.description }}</option>
                    </select>
                    <select v-model="actionDraft.change_state" class="rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-2 py-1.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                      <option value="SHORT">Short Press</option>
                      <option value="LONG">Long Press</option>
                    </select>
                    <select v-model="actionDraft.action" class="rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-2 py-1.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                      <option v-for="opt in availableActions" :key="opt.value" :value="opt.value">{{ opt.label }}</option>
                    </select>
                  </div>
                  <button type="button"
                    class="mt-2 px-3 py-1.5 text-xs font-medium text-white bg-green-600 hover:bg-green-700 rounded-lg transition-colors disabled:opacity-40 disabled:cursor-not-allowed"
                    :disabled="!actionDraft.circuit"
                    @click="addAction">
                    Add Action
                  </button>
                </div>
                <p v-else class="text-xs text-gray-400 dark:text-gray-500">No circuits defined — add circuits first to configure actions.</p>
              </div>

              <div class="flex gap-3 justify-end pt-2 border-t border-gray-100 dark:border-gray-800">
                <button type="button" class="px-4 py-2.5 text-sm text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="showHidModal = false">Cancel</button>
                <button type="submit" class="px-4 py-2.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors">{{ editIdx === null ? 'Add' : 'Save' }}</button>
              </div>
            </form>
          </div>
        </div>
      </Transition>
    </Teleport>

    <ConfirmModal :show="removeHidIdx !== null" title="Remove"
      :message="`Are you sure you wish to delete ${client?.hids?.[removeHidIdx]?.type === 'switch' ? 'switch' : 'button'} #${(removeHidIdx ?? 0) + 1}?`"
      confirm-label="Remove" @confirm="doRemoveHid" @cancel="removeHidIdx = null" />
  </AppLayout>
</template>

<script setup>
import { ref, computed, watch, onMounted } from 'vue'
import { useRoute, RouterLink } from 'vue-router'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import ClientSvg from '../components/ClientSvg.vue'
import { useClients } from '../composables/useClients'
import { useColors } from '../composables/useColors'
import { useTags } from '../composables/useTags'
import { useCircuits } from '../composables/useCircuits'
import { useToast } from '../composables/useToast'
import { moveArrayItem } from '../composables/useValidators'

const route = useRoute()
const { get, update } = useClients()
const { items: colors, load: loadColors } = useColors()
const { items: tags, load: loadTags } = useTags()
const { items: circuits, relayModels, load: loadCircuits } = useCircuits()
const { addToast } = useToast()

const client = ref(null)
const showHidModal = ref(false)
const showColorDropdown = ref(false)
const editIdx = ref(null)
const removeHidIdx = ref(null)

function emptyForm() {
  return { type: 'button', color: '', switch_type: 'NORMALLY_OPEN', enabled: true, tags: [], actions: [] }
}

const hidForm = ref(emptyForm())
const actionDraft = ref({ circuit: '', change_state: 'SHORT', action: 'TOGGLE' })

const selectedColor = computed(() =>
  hidForm.value.color ? colors.value.find(c => c.id === hidForm.value.color) ?? null : null
)

const availableActions = computed(() => {
  const circuitId = Number(actionDraft.value.circuit)
  if (!circuitId) return allActions
  const circuit = circuits.value.find(c => c.id === circuitId)
  if (!circuit) return allActions
  const rm = relayModels.value.find(r => r.id === circuit.relay_model)
  if (!rm) return allActions
  if (rm.type === 'BINARY') return [{ value: 'TOGGLE', label: 'Toggle' }]
  if (rm.type === 'VARIABLE') return [
    { value: 'INCREASE', label: 'Increase' },
    { value: 'DECREASE', label: 'Decrease' },
    { value: 'INCREASE_MAXIMUM', label: 'Maximum' },
    { value: 'DECREASE_MAXIMUM', label: 'Minimum' }
  ]
  return allActions
})

const allActions = [
  { value: 'TOGGLE', label: 'Toggle' },
  { value: 'INCREASE', label: 'Increase' },
  { value: 'DECREASE', label: 'Decrease' },
  { value: 'INCREASE_MAXIMUM', label: 'Maximum' },
  { value: 'DECREASE_MAXIMUM', label: 'Minimum' }
]

watch(() => actionDraft.value.circuit, () => {
  const valid = availableActions.value.some(a => a.value === actionDraft.value.action)
  if (!valid && availableActions.value.length > 0) {
    actionDraft.value.action = availableActions.value[0].value
  }
})

onMounted(async () => {
  const id = Number(route.params.id)
  client.value = await get(id)
  if (client.value && !client.value.hids) client.value.hids = []
  await Promise.all([loadColors(), loadTags(), loadCircuits()])
})

function colorHex(id) { return colors.value.find(c => c.id === id)?.hex ?? '#888' }
function colorName(id) { return colors.value.find(c => c.id === id)?.name ?? '?' }
function tagName(id) { return tags.value.find(t => t.id === id)?.name ?? '?' }
function circuitName(id) {
  const c = circuits.value.find(c => c.id === id)
  return c ? `${c.name} — ${c.description}` : `Circuit #${id}`
}

function formatSwitchType(type) {
  return type === 'NORMALLY_CLOSED' ? 'Normally Closed' : 'Normally Open'
}

function formatChangeState(val) {
  return val === 'LONG' ? 'Long Press' : 'Short Press'
}

function formatAction(val) {
  const map = { TOGGLE: 'Toggle', INCREASE: 'Increase', DECREASE: 'Decrease', INCREASE_MAXIMUM: 'Maximum', DECREASE_MAXIMUM: 'Minimum' }
  return map[val] ?? val
}

function openAdd() {
  hidForm.value = emptyForm()
  actionDraft.value = { circuit: '', change_state: 'SHORT', action: 'TOGGLE' }
  showColorDropdown.value = false
  editIdx.value = null
  showHidModal.value = true
}

function openEdit(idx) {
  const hid = client.value.hids[idx]
  hidForm.value = {
    type: hid.type ?? 'button',
    color: hid.color ?? '',
    switch_type: hid.switch_type ?? 'NORMALLY_OPEN',
    enabled: hid.enabled !== false,
    tags: Array.isArray(hid.tags) ? [...hid.tags] : [],
    actions: JSON.parse(JSON.stringify(hid.actions ?? []))
  }
  actionDraft.value = { circuit: '', change_state: 'SHORT', action: 'TOGGLE' }
  showColorDropdown.value = false
  editIdx.value = idx
  showHidModal.value = true
}

function addAction() {
  if (!actionDraft.value.circuit) return
  hidForm.value.actions = [
    ...hidForm.value.actions,
    {
      circuit: Number(actionDraft.value.circuit),
      change_state: String(actionDraft.value.change_state),
      action: String(actionDraft.value.action)
    }
  ]
  actionDraft.value.circuit = ''
}

function removeAction(i) {
  hidForm.value.actions = hidForm.value.actions.filter((_, idx) => idx !== i)
}

async function submitHid() {
  const isAdd = editIdx.value === null
  try {
    const hid = {
      type: String(hidForm.value.type),
      switch_type: String(hidForm.value.switch_type),
      enabled: Boolean(hidForm.value.enabled),
      tags: hidForm.value.tags.map(id => Number(id)),
      actions: hidForm.value.actions.map(a => ({
        circuit: Number(a.circuit),
        change_state: String(a.change_state),
        action: String(a.action)
      }))
    }
    if (hidForm.value.type === 'button' && hidForm.value.color) {
      hid.color = Number(hidForm.value.color)
    }
    const existing = JSON.parse(JSON.stringify(client.value.hids ?? []))
    if (isAdd) {
      existing.push(hid)
    } else {
      existing[editIdx.value] = hid
    }
    await update(client.value.id, { hids: existing })
    client.value.hids = existing
    showHidModal.value = false
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}

async function toggleEnabled(idx) {
  const newHids = JSON.parse(JSON.stringify(client.value.hids))
  newHids[idx].enabled = newHids[idx].enabled !== false ? false : true
  await update(client.value.id, { hids: newHids })
  client.value.hids = newHids
}

async function moveHid(from, to) {
  const newHids = JSON.parse(JSON.stringify(moveArrayItem(client.value.hids, from, to)))
  await update(client.value.id, { hids: newHids })
  client.value.hids = newHids
}

function confirmRemoveHid(idx) {
  removeHidIdx.value = idx
}

async function doRemoveHid() {
  try {
    const newHids = JSON.parse(JSON.stringify(client.value.hids))
    newHids.splice(removeHidIdx.value, 1)
    await update(client.value.id, { hids: newHids })
    client.value.hids = newHids
  } catch (e) {
    addToast('error', `Failed to remove: ${e.message}`)
  } finally {
    removeHidIdx.value = null
  }
}
</script>
