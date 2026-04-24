<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-2 print:text-xl">Inputs</h1>

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
          class="px-3 py-2 rounded-lg border text-sm font-medium cursor-grab active:cursor-grabbing transition-colors select-none"
          :class="selectedClientId === client.id
            ? 'bg-blue-600 border-blue-600 text-white shadow-md'
            : 'bg-white dark:bg-gray-900 border-gray-300 dark:border-gray-600 text-gray-800 dark:text-gray-200 hover:border-blue-400'"
          @dragstart="startDragClient($event, client.id)"
          @dragend="endDrag"
          @click="toggleSelect(client.id)"
        >
          <span class="font-mono text-xs mr-1">{{ client.name }}</span>{{ client.description }}
        </div>
      </div>
      <p v-if="selectedClientId" class="mt-2 text-xs text-blue-600 dark:text-blue-400 font-medium">
        Client selected — tap an empty port to assign it. Tap the client again to deselect.
      </p>
    </section>

    <!-- Controllers -->
    <section v-for="ctrl in enrichedControllers" :key="ctrl.id" class="mb-8">
      <div class="flex items-center gap-3 mb-3">
        <h2 class="text-lg font-semibold text-gray-900 dark:text-gray-100">{{ ctrl.name }}</h2>
        <span class="text-xs text-gray-500 dark:text-gray-400">{{ ctrl.product }}</span>
      </div>
      <div class="flex flex-wrap gap-3">
        <div
          v-for="port in ctrl.ports"
          :key="port.num"
          class="w-36 h-28 rounded-xl border-2 flex flex-col items-center justify-center text-center p-2 transition-all select-none"
          :class="portClass(ctrl.id, port)"
          :draggable="!!port.client"
          @dragstart="port.client && startDragPort($event, ctrl.id, port.num, port.client.id)"
          @dragend="endDrag"
          @dragover.prevent="onDragOver(ctrl.id, port)"
          @dragleave.self="dragOver = null"
          @drop.prevent="onDropPort(ctrl.id, port)"
          @click="portClick(ctrl.id, port)"
        >
          <span class="text-xs font-semibold text-gray-500 dark:text-gray-400 mb-1">Port {{ port.num }}</span>
          <template v-if="port.client">
            <span class="font-mono text-xs font-bold text-gray-900 dark:text-gray-100 cursor-grab">{{ port.client.name }}</span>
            <span class="text-xs text-gray-600 dark:text-gray-400 leading-tight">{{ port.client.description }}</span>
            <button class="mt-1 text-red-500 hover:text-red-700 text-xs print:hidden leading-none" title="Unassign" @click.stop="unassign(ctrl.id, port.num)">✕</button>
          </template>
          <template v-else>
            <span class="text-xs" :class="canDrop(ctrl.id, port) ? 'text-blue-400 dark:text-blue-500' : 'text-gray-300 dark:text-gray-600'">
              {{ canDrop(ctrl.id, port) ? 'Drop here' : 'Empty' }}
            </span>
          </template>
        </div>
      </div>
    </section>

    <div v-if="enrichedControllers.length === 0" class="text-gray-400 dark:text-gray-500 py-8">Add a controller to assign inputs.</div>

    <ConfirmModal :show="!!unassignTarget" title="Unassign Client"
      :message="`Unassign '${unassignTarget?.clientName}' from port ${unassignTarget?.port}?`"
      confirm-label="Unassign" @confirm="doUnassign" @cancel="unassignTarget = null" />
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useControllers } from '../composables/useControllers'
import { useClients } from '../composables/useClients'
import { useToast } from '../composables/useToast'

const { items: controllers, products, load: loadControllers, assignInput } = useControllers()
const { items: allClients, load: loadClients } = useClients()
const { addToast } = useToast()

const selectedClientId = ref(null)
const unassignTarget = ref(null)

// drag state: { clientId, fromControllerId, fromPort }
const dragging = ref(null)
const dragOver = ref(null) // 'panel' | 'ctrl-port' key

onMounted(() => Promise.all([loadControllers(), loadClients()]))

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

function portKey(controllerId, portNum) {
  return `${controllerId}:${portNum}`
}

function canDrop(controllerId, port) {
  if (port.client) return false
  return !!dragging.value || !!selectedClientId.value
}

function portClass(controllerId, port) {
  const key = portKey(controllerId, port.num)
  if (port.client) {
    return dragOver.value === key
      ? 'border-amber-400 bg-amber-50 dark:bg-amber-900/20 cursor-pointer'
      : 'border-green-400 bg-green-50 dark:bg-green-900/20 cursor-grab'
  }
  if (dragOver.value === key) {
    return 'border-blue-500 bg-blue-100 dark:bg-blue-900/30 cursor-copy'
  }
  if (canDrop(controllerId, port)) {
    return 'border-blue-300 bg-blue-50 dark:bg-blue-900/10 cursor-pointer border-dashed'
  }
  return 'border-gray-200 dark:border-gray-700 bg-gray-50 dark:bg-gray-800/50'
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
  if (!port.client) {
    dragOver.value = portKey(controllerId, port.num)
  }
}

async function onDropPort(controllerId, port) {
  if (!dragging.value) { dragOver.value = null; return }
  if (port.client) { endDrag(); return } // don't overwrite occupied port

  try {
    await assignInput(controllerId, port.num, dragging.value.clientId)
    // If came from another port, unassign that port
    if (dragging.value.fromControllerId !== null) {
      await assignInput(dragging.value.fromControllerId, dragging.value.fromPort, null)
    }
    addToast('success', 'Client assigned.')
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
  endDrag()
}

async function onDropUnassign() {
  if (!dragging.value?.fromControllerId) { endDrag(); return }
  try {
    await assignInput(dragging.value.fromControllerId, dragging.value.fromPort, null)
    addToast('success', 'Client unassigned.')
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
  endDrag()
}

// Click-to-assign (mobile fallback)
async function portClick(controllerId, port) {
  if (port.client || !selectedClientId.value) return
  try {
    await assignInput(controllerId, port.num, selectedClientId.value)
    selectedClientId.value = null
    addToast('success', 'Client assigned.')
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
}

function unassign(controllerId, portNum) {
  const ctrl = controllers.value.find(c => c.id === controllerId)
  const clientId = ctrl?.inputs?.[portNum]
  const client = allClients.value.find(c => c.id === clientId)
  unassignTarget.value = { controllerId, port: portNum, clientName: client?.name ?? '?' }
}

async function doUnassign() {
  try {
    await assignInput(unassignTarget.value.controllerId, unassignTarget.value.port, null)
    addToast('success', 'Unassigned.')
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  } finally {
    unassignTarget.value = null
  }
}
</script>
