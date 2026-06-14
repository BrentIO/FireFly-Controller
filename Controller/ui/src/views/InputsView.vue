<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-2 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl print:!text-black">Inputs</h1>
      <button class="px-4 py-2 rounded-lg bg-gray-100 hover:bg-gray-200 dark:bg-gray-800 dark:hover:bg-gray-700 text-gray-700 dark:text-gray-200 text-sm font-medium transition-colors print:hidden" @click="printPage">Print</button>
    </div>

    <!-- Unassigned clients panel -->
    <section class="mb-8 print:hidden">
      <h2 class="text-sm font-semibold text-gray-600 dark:text-gray-400 uppercase tracking-wider mb-2">Available Clients</h2>
      <p class="text-xs text-gray-500 dark:text-gray-400 mb-3">
        Drag a client onto an empty port to assign it, or tap a client then tap a port on mobile.
      </p>
      <div
        class="min-h-[3rem] rounded-xl border-2 border-dashed p-3 flex flex-wrap gap-2 transition-colors"
        :class="dragOver === 'panel'
          ? 'border-blue-400 bg-blue-50 dark:bg-blue-900/20'
          : 'border-gray-200 dark:border-gray-700 bg-gray-50 dark:bg-gray-800/30'"
        @dragover.prevent="dragOver = 'panel'"
        @dragleave.self="dragOver = null"
        @drop.prevent="onDropUnassign"
      >
        <div v-if="unassignedClients.length === 0" class="text-sm text-gray-400 dark:text-gray-500 self-center">
          All clients assigned.
        </div>
        <div
          v-for="client in unassignedClients"
          :key="client.id"
          draggable="true"
          class="w-28 h-28 rounded-xl border-2 flex flex-col items-center justify-center text-center p-2 cursor-grab active:cursor-grabbing transition-colors select-none"
          :class="selectedClientId === client.id
            ? 'bg-blue-600 border-blue-600 text-white shadow-md'
            : 'bg-white dark:bg-gray-900 border-gray-300 dark:border-gray-600 text-gray-800 dark:text-gray-200 hover:border-blue-400'"
          @dragstart="startDragClient($event, client.id)"
          @dragend="endDrag"
          @click="toggleSelect(client.id)"
        >
          <span class="font-mono text-base font-bold leading-tight">{{ client.name }}</span>
          <span class="text-xs leading-tight mt-1 opacity-70">{{ client.description }}</span>
          <span v-if="primaryBySecondaryId.has(client.id)" class="mt-1 px-1.5 py-0.5 text-[10px] font-semibold rounded bg-orange-600 text-white dark:bg-orange-600 dark:text-white leading-none">Extended</span>
        </div>
      </div>
      <p v-if="selectedClientId" class="mt-2 text-xs text-blue-600 dark:text-blue-400 font-medium">
        Client selected — tap an empty port to assign it. Tap the client again to deselect.
      </p>
    </section>

    <!-- Controllers -->
    <section v-for="(ctrl, index) in enrichedControllers" :key="ctrl.id" class="mb-8" :class="index > 0 ? 'print:break-before-page' : ''">
      <button type="button" class="flex items-center gap-3 mb-3 w-full text-left print:hidden" @click="toggleCollapse(ctrl.id)">
        <svg class="w-4 h-4 text-gray-400 shrink-0 transition-transform" :class="collapsed.has(ctrl.id) ? '-rotate-90' : ''" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
          <path stroke-linecap="round" stroke-linejoin="round" d="M19 9l-7 7-7-7" />
        </svg>
        <h2 class="text-lg font-semibold text-gray-900 dark:text-gray-100">{{ ctrl.name }}</h2>
        <span class="text-xs text-gray-500 dark:text-gray-400">{{ ctrl.product }}</span>
      </button>
      <div class="hidden print:block mb-3">
        <h2 class="text-lg font-semibold text-black">{{ ctrl.name }}</h2>
        <p class="text-xs text-black font-mono">{{ ctrl.uuid }}</p>
        <p class="text-xs text-black">{{ ctrl.product }}</p>
        <p class="text-xs text-black">{{ areas.find(a => a.id === ctrl.area)?.name ?? '' }}</p>
      </div>
      <div v-show="!collapsed.has(ctrl.id)" class="flex flex-wrap gap-3 print:!flex">
        <div
          v-for="port in ctrl.ports"
          :key="port.num"
          class="w-36 h-28 rounded-xl border-2 flex flex-col overflow-hidden transition-all select-none break-inside-avoid"
          :class="portClass(ctrl.id, port)"
          :style="portStyle(ctrl.id, port)"
          :draggable="!!port.client"
          @dragstart="port.client && startDragPort($event, ctrl.id, port.num, port.client.id)"
          @dragend="endDrag"
          @dragover.prevent="onDragOver(ctrl.id, port)"
          @dragleave.self="dragOver = null"
          @drop.prevent="onDropPort(ctrl.id, port)"
          @click="portClick(ctrl.id, port)"
        >
          <div class="bg-black text-white text-xs font-bold text-center py-1 flex-shrink-0"
               style="print-color-adjust:exact;-webkit-print-color-adjust:exact">Port {{ port.num }}</div>
          <div class="flex-1 flex flex-col items-center justify-center text-center px-2 pb-1">
            <template v-if="port.client">
              <span class="font-mono text-xs font-bold text-gray-900 dark:text-black cursor-grab print:!text-black">{{ port.client.name }}</span>
              <span class="text-xs text-gray-600 dark:text-black leading-tight print:!text-black">{{ port.client.description }}</span>
              <span v-if="primaryBySecondaryId.has(port.client.id)" class="mt-1 px-1.5 py-0.5 text-[10px] font-semibold rounded bg-orange-600 text-white dark:bg-orange-600 dark:text-white leading-none print:!bg-orange-600 print:!text-white">Extended</span>
              <button class="mt-1 text-red-500 hover:text-red-700 text-xs print:hidden leading-none" title="Unassign" @click.stop="unassign(ctrl.id, port.num)">✕</button>
            </template>
            <template v-else>
              <span class="print:hidden text-xs"
                :class="portEmptyTextClass(ctrl.id, port)">
                {{ portEmptyLabel(ctrl.id, port) }}
              </span>
            </template>
          </div>
        </div>
      </div>
    </section>

    <div v-if="enrichedControllers.length === 0" class="text-gray-400 dark:text-gray-500 py-8">Add a controller to assign inputs.</div>

    <ConfirmModal :show="!!unassignTarget" title="Unassign Client"
      :message="`Unassign '${unassignTarget?.clientName}' from controller '${unassignTarget?.controllerName}' port ${unassignTarget?.port}?`"
      confirm-label="Unassign" @confirm="doUnassign" @cancel="unassignTarget = null" />
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useControllers } from '../composables/useControllers'
import { useClients } from '../composables/useClients'
import { useAreas } from '../composables/useAreas'
import { useToast } from '../composables/useToast'

const { items: controllers, products, load: loadControllers, assignInput } = useControllers()
const { items: allClients, load: loadClients } = useClients()
const { items: areas, load: loadAreas } = useAreas()
const { addToast } = useToast()

const selectedClientId = ref(null)
const unassignTarget = ref(null)
const collapsed = ref(new Set())

function toggleCollapse(id) {
  const s = new Set(collapsed.value)
  s.has(id) ? s.delete(id) : s.add(id)
  collapsed.value = s
}

// drag state: { clientId, fromControllerId, fromPort }
const dragging = ref(null)
const dragOver = ref(null) // 'panel' | 'ctrl-port' key

onMounted(() => Promise.all([loadControllers(), loadClients(), loadAreas()]))

const assignedClientIds = computed(() => {
  const ids = new Set()
  controllers.value.forEach(c => Object.values(c.inputs || {}).forEach(id => ids.add(id)))
  return ids
})

const unassignedClients = computed(() =>
  allClients.value.filter(c => !assignedClientIds.value.has(c.id))
)

const enrichedControllers = computed(() => controllers.value.map(ctrl => {
  const product = products.value.find(p => p.pid === ctrl.product)
  const count = product?.inputs?.count ?? 0
  return {
    ...ctrl,
    ports: Array.from({ length: count }, (_, i) => {
      const num = i + 1
      const clientId = ctrl.inputs?.[num]
      return { num, client: clientId ? allClients.value.find(c => c.id === clientId) ?? null : null }
    })
  }
}))

// Map from secondary client id → primary client object
const primaryBySecondaryId = computed(() => {
  const map = new Map()
  for (const c of allClients.value) {
    if (c.extends != null) map.set(c.extends, c)
  }
  return map
})

function portKey(controllerId, portNum) {
  return `${controllerId}:${portNum}`
}

// Returns the controller id that has the given clientId assigned to any port, or null
function getAssignedControllerId(clientId) {
  for (const ctrl of controllers.value) {
    if (Object.values(ctrl.inputs || {}).includes(clientId)) return ctrl.id
  }
  return null
}

// Returns the controller id that the active client MUST be placed on due to extended pairing, or null
function requiredControllerId(clientId) {
  const client = allClients.value.find(c => c.id === clientId)
  if (!client) return null

  // If this is a primary with an extends, constrain to the same controller as the secondary
  if (client.extends != null) {
    return getAssignedControllerId(client.extends)
  }

  // If this is a secondary (someone else extends it), constrain to the same controller as the primary
  const primaryClient = primaryBySecondaryId.value.get(clientId)
  if (primaryClient) {
    return getAssignedControllerId(primaryClient.id)
  }

  // If this client has actions, lock to the controller those circuits are assigned to
  const actionCircuitIds = (client.hids ?? []).flatMap(h => (h.actions ?? []).map(a => a.circuit))
  if (actionCircuitIds.length > 0) {
    const required = new Set()
    for (const circuitId of actionCircuitIds) {
      const ctrl = controllers.value.find(c => Object.values(c.outputs || {}).includes(circuitId))
      if (ctrl) required.add(ctrl.id)
    }
    if (required.size === 1) return [...required][0]
  }

  return null
}

function canDrop(controllerId, port) {
  if (port.client) return false
  const activeClientId = dragging.value?.clientId ?? selectedClientId.value
  if (!activeClientId) return false

  const required = requiredControllerId(activeClientId)
  if (required !== null && required !== controllerId) return false

  return true
}

function portClass(controllerId, port) {
  const key = portKey(controllerId, port.num)
  if (port.client) {
    return dragOver.value === key
      ? 'border-amber-400 bg-amber-50 dark:bg-amber-900/20 cursor-pointer'
      : 'cursor-grab'
  }
  if (dragOver.value === key) {
    return 'border-blue-500 bg-blue-100 dark:bg-blue-900/30 cursor-copy'
  }
  const activeClientId = dragging.value?.clientId ?? selectedClientId.value
  if (activeClientId) {
    // There's an active drag/selection — check if this port is blocked by controller constraint
    const required = requiredControllerId(activeClientId)
    if (required !== null && required !== controllerId) {
      // Blocked by controller pairing constraint — show distinct visual
      return 'border-red-200 dark:border-red-900 bg-red-50 dark:bg-red-900/10 border-dashed cursor-not-allowed'
    }
    return 'border-blue-300 bg-blue-50 dark:bg-blue-900/10 cursor-pointer border-dashed'
  }
  return 'border-gray-200 dark:border-gray-700 bg-gray-50 dark:bg-gray-800/50 print:border-gray-400 print:bg-white'
}

function portEmptyLabel(controllerId, port) {
  const activeClientId = dragging.value?.clientId ?? selectedClientId.value
  if (!activeClientId) return 'Empty'
  const required = requiredControllerId(activeClientId)
  if (required !== null && required !== controllerId) return 'Blocked'
  return 'Drop here'
}

function portEmptyTextClass(controllerId, port) {
  const activeClientId = dragging.value?.clientId ?? selectedClientId.value
  if (!activeClientId) return 'text-gray-300 dark:text-gray-600'
  const required = requiredControllerId(activeClientId)
  if (required !== null && required !== controllerId) return 'text-red-400 dark:text-red-600'
  return 'text-blue-400 dark:text-blue-500'
}

function portStyle(controllerId, port) {
  if (!port.client) return {}
  const key = portKey(controllerId, port.num)
  if (dragOver.value === key) return {}
  return { borderColor: '#f2981b', backgroundColor: '#f7cc8f', printColorAdjust: 'exact', WebkitPrintColorAdjust: 'exact' }
}

function toggleSelect(id) {
  selectedClientId.value = selectedClientId.value === id ? null : id
}

// Drag handlers
function startDragClient(event, clientId) {
  dragging.value = { clientId, fromControllerId: null, fromPort: null }
  event.dataTransfer.effectAllowed = 'move'
}

function startDragPort(event, controllerId, portNum, clientId) {
  dragging.value = { clientId, fromControllerId: controllerId, fromPort: portNum }
  event.dataTransfer.effectAllowed = 'move'
}

function endDrag() {
  dragging.value = null
  dragOver.value = null
}

function onDragOver(controllerId, port) {
  if (!port.client && canDrop(controllerId, port)) {
    dragOver.value = portKey(controllerId, port.num)
  }
}

async function onDropPort(controllerId, port) {
  if (!dragging.value) { dragOver.value = null; return }
  if (port.client) { endDrag(); return }
  if (!canDrop(controllerId, port)) { endDrag(); return }
  const drag = { ...dragging.value }
  endDrag()
  try {
    if (drag.fromControllerId !== null) {
      await assignInput(drag.fromControllerId, drag.fromPort, null)
    }
    await assignInput(controllerId, port.num, drag.clientId)
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
}

async function onDropUnassign() {
  if (!dragging.value?.fromControllerId) { endDrag(); return }
  const drag = { ...dragging.value }
  endDrag()
  try {
    await assignInput(drag.fromControllerId, drag.fromPort, null)
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
}

// Click-to-assign (mobile fallback)
async function portClick(controllerId, port) {
  if (port.client || !selectedClientId.value) return
  if (!canDrop(controllerId, port)) return
  try {
    await assignInput(controllerId, port.num, selectedClientId.value)
    selectedClientId.value = null
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
}

function unassign(controllerId, portNum) {
  const ctrl = controllers.value.find(c => c.id === controllerId)
  const clientId = ctrl?.inputs?.[portNum]
  const client = allClients.value.find(c => c.id === clientId)
  unassignTarget.value = { controllerId, port: portNum, clientName: client?.name ?? '?', controllerName: ctrl?.name ?? '?' }
}

async function doUnassign() {
  try {
    await assignInput(unassignTarget.value.controllerId, unassignTarget.value.port, null)
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  } finally {
    unassignTarget.value = null
  }
}

function printPage() {
  const prev = document.title
  document.title = 'Inputs'
  window.addEventListener('afterprint', () => { document.title = prev }, { once: true })
  window.print()
}
</script>
