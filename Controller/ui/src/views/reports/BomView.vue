<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl">Bill of Materials</h1>
      <button class="px-4 py-2 rounded-lg bg-gray-100 hover:bg-gray-200 dark:bg-gray-800 dark:hover:bg-gray-700 text-gray-700 dark:text-gray-200 text-sm font-medium transition-colors print:hidden" onclick="window.print()">Print</button>
    </div>

    <div class="space-y-6">
      <!-- Controllers -->
      <section class="break-inside-avoid">
        <h2 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-3">Controllers</h2>
        <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-hidden">
          <table class="w-full text-sm">
            <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
              <tr><th class="px-4 py-2 text-left">Product ID</th><th class="px-4 py-2 text-right">Qty</th></tr>
            </thead>
            <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
              <tr v-if="!controllerBom.length"><td colspan="2" class="px-4 py-4 text-center text-gray-400">None.</td></tr>
              <tr v-for="row in controllerBom" :key="row.pid" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
                <td class="px-4 py-2 text-gray-900 dark:text-gray-100">{{ row.pid }}</td>
                <td class="px-4 py-2 text-right text-gray-700 dark:text-gray-300 font-semibold">{{ row.qty }}</td>
              </tr>
            </tbody>
          </table>
        </div>
      </section>

      <!-- Clients -->
      <section class="break-inside-avoid">
        <h2 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-3">Clients</h2>
        <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-hidden">
          <table class="w-full text-sm">
            <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
              <tr><th class="px-4 py-2 text-left">Button Count</th><th class="px-4 py-2 text-right">Qty</th></tr>
            </thead>
            <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
              <tr v-if="!clientBom.length"><td colspan="2" class="px-4 py-4 text-center text-gray-400">None.</td></tr>
              <tr v-for="row in clientBom" :key="row.count" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
                <td class="px-4 py-2 text-gray-900 dark:text-gray-100">{{ row.count }} button(s)</td>
                <td class="px-4 py-2 text-right text-gray-700 dark:text-gray-300 font-semibold">{{ row.qty }}</td>
              </tr>
            </tbody>
          </table>
        </div>
      </section>

      <!-- Breakers & Circuits -->
      <section class="break-inside-avoid">
        <h2 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-3">Relays</h2>
        <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-hidden">
          <table class="w-full text-sm">
            <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider">
              <tr><th class="px-4 py-2 text-left">Manufacturer</th><th class="px-4 py-2 text-left">Model</th><th class="px-4 py-2 text-left">Description</th><th class="px-4 py-2 text-right">Qty</th></tr>
            </thead>
            <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
              <tr v-if="!relayBom.length"><td colspan="4" class="px-4 py-4 text-center text-gray-400">None.</td></tr>
              <tr v-for="row in relayBom" :key="row.id" class="hover:bg-gray-50 dark:hover:bg-gray-800/50">
                <td class="px-4 py-2 text-gray-900 dark:text-gray-100">{{ row.manufacturer }}</td>
                <td class="px-4 py-2 text-gray-700 dark:text-gray-300">{{ row.model }}</td>
                <td class="px-4 py-2 text-gray-600 dark:text-gray-400">{{ row.description }}</td>
                <td class="px-4 py-2 text-right font-semibold text-gray-700 dark:text-gray-300">{{ row.qty }}</td>
              </tr>
            </tbody>
          </table>
        </div>
      </section>
    </div>
  </AppLayout>
</template>

<script setup>
import { ref, computed, onMounted } from 'vue'
import AppLayout from '../../components/AppLayout.vue'
import { useControllers } from '../../composables/useControllers'
import { useClients } from '../../composables/useClients'
import { useCircuits } from '../../composables/useCircuits'

const { items: controllers, products, load: loadControllers } = useControllers()
const { items: clients, load: loadClients, getExtendedClientIds } = useClients()
const { items: circuits, relayModels, load: loadCircuits } = useCircuits()

const extendedIds = ref([])

onMounted(async () => {
  await Promise.all([loadControllers(), loadClients(), loadCircuits()])
  extendedIds.value = await getExtendedClientIds()
})

const controllerBom = computed(() => {
  const counts = {}
  controllers.value.forEach(c => { counts[c.product] = (counts[c.product] ?? 0) + 1 })
  return Object.entries(counts).map(([pid, qty]) => ({ pid, qty })).sort((a, b) => a.pid.localeCompare(b.pid))
})

const clientBom = computed(() => {
  const real = clients.value.filter(c => !extendedIds.value.includes(c.id))
  const counts = {}
  real.forEach(c => {
    const n = c.hids?.length ?? 0
    counts[n] = (counts[n] ?? 0) + 1
  })
  return Object.entries(counts).map(([count, qty]) => ({ count: Number(count), qty })).sort((a, b) => a.count - b.count)
})

const relayBom = computed(() => {
  const counts = {}
  circuits.value.forEach(c => {
    const key = c.relay_model
    if (!counts[key]) {
      const rm = relayModels.value.find(r => r.id === key)
      counts[key] = { id: key, manufacturer: rm?.manufacturer ?? '?', model: rm?.model ?? '?', description: rm?.description ?? '?', qty: 0 }
    }
    counts[key].qty++
  })
  return Object.values(counts).sort((a, b) => a.model.localeCompare(b.model))
})
</script>
