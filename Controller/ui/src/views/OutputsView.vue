<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-2 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Outputs</h1>
      <button class="px-4 py-2 rounded-lg bg-gray-100 hover:bg-gray-200 dark:bg-gray-800 dark:hover:bg-gray-700 text-gray-700 dark:text-gray-200 text-sm font-medium transition-colors print:hidden" onclick="window.print()">Print</button>
    </div>

    <!-- Unassigned circuits panel -->
    <section class="mb-8 print:hidden">
      <h2 class="text-sm font-semibold text-gray-600 dark:text-gray-400 uppercase tracking-wider mb-2">Available Circuits</h2>
      <p class="text-xs text-gray-500 dark:text-gray-400 mb-3">
        Drag a circuit onto an empty port to assign it, or tap a circuit then tap a port on mobile.
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
        <div v-if="unassignedCircuits.length === 0" class="text-sm text-gray-400 dark:text-gray-500 self-center">
          All circuits assigned.
        </div>
        <div
          v-for="circuit in unassignedCircuits"
          :key="circuit.id"
          draggable="true"
          class="w-28 h-28 rounded-xl border-2 flex flex-col items-center justify-center text-center p-2 cursor-grab active:cursor-grabbing transition-colors select-none"
          :class="selectedCircuitId === circuit.id
            ? 'bg-blue-600 border-blue-600 text-white shadow-md'
            : 'bg-white dark:bg-gray-900 border-gray-300 dark:border-gray-600 text-gray-800 dark:text-gray-200 hover:border-blue-400'"
          @dragstart="startDragCircuit($event, circuit.id)"
          @dragend="endDrag"
          @click="toggleSelect(circuit.id)"
        >
          <span class="font-mono text-base font-bold leading-tight">{{ circuit.name }}</span>
          <span class="text-xs leading-tight mt-1 opacity-70">{{ circuit.description }}</span>
        </div>
      </div>
      <p v-if="selectedCircuitId" class="mt-2 text-xs text-blue-600 dark:text-blue-400 font-medium">
        Circuit selected — tap an empty port to assign it. Tap the circuit again to deselect.
      </p>
    </section>

    <!-- Controllers -->
    <section v-for="ctrl in enrichedControllers" :key="ctrl.id" class="mb-8">
      <button type="button" class="flex items-center gap-3 mb-3 w-full text-left print:hidden" @click="toggleCollapse(ctrl.id)">
        <svg class="w-4 h-4 text-gray-400 shrink-0 transition-transform" :class="collapsed.has(ctrl.id) ? '-rotate-90' : ''" fill="none" viewBox="0 0 24 24" stroke="currentColor" stroke-width="2">
          <path stroke-linecap="round" stroke-linejoin="round" d="M19 9l-7 7-7-7" />
        </svg>
        <h2 class="text-lg font-semibold text-gray-900 dark:text-gray-100">{{ ctrl.name }}</h2>
        <span class="text-xs text-gray-500 dark:text-gray-400">{{ ctrl.product }}</span>
      </button>
      <h2 class="hidden print:block text-lg font-semibold text-black mb-3">{{ ctrl.name }} <span class="text-xs font-normal text-gray-600">{{ ctrl.product }}</span></h2>
      <div v-show="!collapsed.has(ctrl.id)" class="flex flex-wrap gap-3 print:!flex">
        <div
          v-for="port in ctrl.ports"
          :key="port.num"
          class="w-36 h-28 rounded-xl border-2 flex flex-col items-center justify-center text-center p-2 transition-all select-none"
          :class="portClass(ctrl.id, port)"
          :style="portStyle(ctrl.id, port)"
          :draggable="!!port.circuit"
          @dragstart="port.circuit && startDragPort($event, ctrl.id, port.num, port.circuit.id)"
          @dragend="endDrag"
          @dragover.prevent="onDragOver(ctrl.id, port)"
          @dragleave.self="dragOver = null"
          @drop.prevent="onDropPort(ctrl.id, port)"
          @click="portClick(ctrl.id, port)"
        >
          <span class="text-xs font-semibold mb-1 print:!text-black"
                :class="port.circuit ? 'text-gray-700 dark:text-black' : 'text-gray-500 dark:text-gray-400'">Port {{ port.num }}</span>
          <template v-if="port.circuit">
            <span class="font-mono text-xs font-bold text-gray-900 dark:text-black cursor-grab print:!text-black">{{ port.circuit.name }}</span>
            <span class="text-xs text-gray-600 dark:text-black leading-tight print:!text-black">{{ port.circuit.description }}</span>
            <button class="mt-1 text-red-500 hover:text-red-700 text-xs print:hidden leading-none" title="Unassign" @click.stop="unassign(ctrl.id, port.num)">✕</button>
          </template>
          <template v-else>
            <span class="text-xs print:text-gray-500" :class="canDrop(ctrl.id, port) ? 'text-blue-400 dark:text-blue-500' : 'text-gray-300 dark:text-gray-600'">
              {{ canDrop(ctrl.id, port) ? 'Drop here' : 'Empty' }}
            </span>
          </template>
        </div>
      </div>
    </section>

    <div v-if="enrichedControllers.length === 0" class="text-gray-400 dark:text-gray-500 py-8">Add a controller to assign outputs.</div>

    <ConfirmModal :show="!!unassignTarget" title="Unassign Circuit"
      :message="`Unassign '${unassignTarget?.circuitName}' from port ${unassignTarget?.port}?`"
      confirm-label="Unassign" @confirm="doUnassign" @cancel="unassignTarget = null" />
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useControllers } from '../composables/useControllers'
import { useCircuits } from '../composables/useCircuits'
import { useToast } from '../composables/useToast'

const { items: controllers, products, load: loadControllers, assignOutput } = useControllers()
const { items: allCircuits, load: loadCircuits } = useCircuits()
const { addToast } = useToast()

const selectedCircuitId = ref(null)
const unassignTarget = ref(null)
const collapsed = ref(new Set())

function toggleCollapse(id) {
  const s = new Set(collapsed.value)
  s.has(id) ? s.delete(id) : s.add(id)
  collapsed.value = s
}

const dragging = ref(null) // { circuitId, fromControllerId, fromPort }
const dragOver = ref(null)

onMounted(() => Promise.all([loadControllers(), loadCircuits()]))

const assignedCircuitIds = computed(() => {
  const ids = new Set()
  controllers.value.forEach(c => Object.values(c.outputs || {}).forEach(id => ids.add(id)))
  return ids
})

const unassignedCircuits = computed(() =>
  allCircuits.value.filter(c => !assignedCircuitIds.value.has(c.id))
)

const enrichedControllers = computed(() => controllers.value.map(ctrl => {
  const product = products.value.find(p => p.pid === ctrl.product)
  const count = product?.outputs?.count ?? 0
  return {
    ...ctrl,
    ports: Array.from({ length: count }, (_, i) => {
      const num = i + 1
      const circuitId = ctrl.outputs?.[num]
      return { num, circuit: circuitId ? allCircuits.value.find(c => c.id === circuitId) ?? null : null }
    })
  }
}))

function portKey(controllerId, portNum) {
  return `${controllerId}:${portNum}`
}

function canDrop(controllerId, port) {
  if (port.circuit) return false
  return !!dragging.value || !!selectedCircuitId.value
}

function portClass(controllerId, port) {
  const key = portKey(controllerId, port.num)
  if (port.circuit) {
    return dragOver.value === key
      ? 'border-amber-400 bg-amber-50 dark:bg-amber-900/20 cursor-pointer'
      : 'cursor-grab'
  }
  if (dragOver.value === key) {
    return 'border-blue-500 bg-blue-100 dark:bg-blue-900/30 cursor-copy'
  }
  if (canDrop(controllerId, port)) {
    return 'border-blue-300 bg-blue-50 dark:bg-blue-900/10 cursor-pointer border-dashed'
  }
  return 'border-gray-200 dark:border-gray-700 bg-gray-50 dark:bg-gray-800/50 print:border-gray-400 print:bg-white'
}

function portStyle(controllerId, port) {
  if (!port.circuit) return {}
  const key = portKey(controllerId, port.num)
  if (dragOver.value === key) return {}
  return { borderColor: '#8df086', backgroundColor: '#c7e5c5', printColorAdjust: 'exact', WebkitPrintColorAdjust: 'exact' }
}

function toggleSelect(id) {
  selectedCircuitId.value = selectedCircuitId.value === id ? null : id
}

function startDragCircuit(event, circuitId) {
  dragging.value = { circuitId, fromControllerId: null, fromPort: null }
  event.dataTransfer.effectAllowed = 'move'
}

function startDragPort(event, controllerId, portNum, circuitId) {
  dragging.value = { circuitId, fromControllerId: controllerId, fromPort: portNum }
  event.dataTransfer.effectAllowed = 'move'
}

function endDrag() {
  dragging.value = null
  dragOver.value = null
}

function onDragOver(controllerId, port) {
  if (!port.circuit) {
    dragOver.value = portKey(controllerId, port.num)
  }
}

async function onDropPort(controllerId, port) {
  if (!dragging.value) { dragOver.value = null; return }
  if (port.circuit) { endDrag(); return }
  const drag = { ...dragging.value }
  endDrag()
  try {
    if (drag.fromControllerId !== null) {
      await assignOutput(drag.fromControllerId, drag.fromPort, null)
    }
    await assignOutput(controllerId, port.num, drag.circuitId)
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
}

async function onDropUnassign() {
  if (!dragging.value?.fromControllerId) { endDrag(); return }
  const drag = { ...dragging.value }
  endDrag()
  try {
    await assignOutput(drag.fromControllerId, drag.fromPort, null)
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
}

async function portClick(controllerId, port) {
  if (port.circuit || !selectedCircuitId.value) return
  try {
    await assignOutput(controllerId, port.num, selectedCircuitId.value)
    selectedCircuitId.value = null
    addToast('success', 'Circuit assigned.')
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  }
}

function unassign(controllerId, portNum) {
  const ctrl = controllers.value.find(c => c.id === controllerId)
  const circuitId = ctrl?.outputs?.[portNum]
  const circuit = allCircuits.value.find(c => c.id === circuitId)
  unassignTarget.value = { controllerId, port: portNum, circuitName: circuit?.name ?? '?' }
}

async function doUnassign() {
  try {
    await assignOutput(unassignTarget.value.controllerId, unassignTarget.value.port, null)
  } catch (e) {
    addToast('error', `Failed: ${e.message}`)
  } finally {
    unassignTarget.value = null
  }
}
</script>
