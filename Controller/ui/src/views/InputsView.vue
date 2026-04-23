<template>
  <AppLayout>
    <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-2 print:text-xl">Inputs</h1>
    <p class="text-sm text-gray-500 dark:text-gray-400 mb-6 print:hidden">Select a client below, then click an empty port to assign it.</p>

    <!-- Assignable clients panel -->
    <section class="mb-8 print:hidden">
      <h2 class="text-sm font-semibold text-gray-600 dark:text-gray-400 uppercase tracking-wider mb-3">Unassigned Clients</h2>
      <div v-if="unassignedClients.length === 0" class="text-sm text-gray-400 dark:text-gray-500">All clients have been assigned.</div>
      <div class="flex flex-wrap gap-2">
        <button
          v-for="client in unassignedClients"
          :key="client.id"
          class="px-3 py-2 rounded-lg border text-sm font-medium transition-colors"
          :class="selectedClientId === client.id
            ? 'bg-blue-600 border-blue-600 text-white'
            : 'bg-white dark:bg-gray-900 border-gray-300 dark:border-gray-600 text-gray-800 dark:text-gray-200 hover:border-blue-400'"
          @click="toggleSelect(client.id)"
        >
          <span class="font-mono text-xs mr-1">{{ client.name }}</span> {{ client.description }}
        </button>
      </div>
      <p v-if="selectedClientId" class="mt-2 text-xs text-blue-600 dark:text-blue-400">Click an empty port to assign the selected client. Click again to deselect.</p>
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
          class="w-36 h-28 rounded-xl border-2 flex flex-col items-center justify-center text-center p-2 cursor-pointer transition-all select-none"
          :class="portClass(port)"
          @click="portClick(ctrl.id, port)"
        >
          <span class="text-xs font-semibold text-gray-500 dark:text-gray-400 mb-1">Port {{ port.num }}</span>
          <template v-if="port.client">
            <span class="font-mono text-xs font-bold text-gray-900 dark:text-gray-100">{{ port.client.name }}</span>
            <span class="text-xs text-gray-600 dark:text-gray-400 leading-tight">{{ port.client.description }}</span>
            <button class="mt-1 text-red-500 hover:text-red-700 text-xs print:hidden" @click.stop="unassign(ctrl.id, port.num)">×</button>
          </template>
          <template v-else>
            <span class="text-gray-300 dark:text-gray-600 text-xs">Empty</span>
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
const { items: allClients, load: loadClients, getExtendedClientIds } = useClients()
const { addToast } = useToast()

const selectedClientId = ref(null)
const unassignTarget = ref(null)

onMounted(() => Promise.all([loadControllers(), loadClients()]))

const assignedClientIds = computed(() => {
  const ids = []
  controllers.value.forEach(c => Object.values(c.inputs || {}).forEach(id => ids.push(id)))
  return ids
})

const unassignedClients = computed(() =>
  allClients.value.filter(c => !assignedClientIds.value.includes(c.id))
)

const enrichedControllers = computed(() => controllers.value.map(ctrl => {
  const product = products.value.find(p => p.pid === ctrl.product)
  const count = product?.inputs?.count ?? 0
  const ports = Array.from({ length: count }, (_, i) => {
    const num = i + 1
    const clientId = ctrl.inputs?.[num]
    const client = clientId ? allClients.value.find(c => c.id === clientId) : null
    return { num, client }
  })
  return { ...ctrl, ports }
}))

function toggleSelect(id) {
  selectedClientId.value = selectedClientId.value === id ? null : id
}

function portClass(port) {
  if (port.client) {
    return 'border-green-400 bg-green-50 dark:bg-green-900/20'
  }
  if (selectedClientId.value) {
    return 'border-blue-300 bg-blue-50 dark:bg-blue-900/10 hover:border-blue-500 hover:bg-blue-100 dark:hover:bg-blue-900/30'
  }
  return 'border-gray-200 dark:border-gray-700 bg-gray-50 dark:bg-gray-800/50'
}

async function portClick(controllerId, port) {
  if (port.client) return
  if (!selectedClientId.value) return

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
