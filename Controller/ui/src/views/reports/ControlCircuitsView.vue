<template>
  <AppLayout>
    <div class="flex items-center justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl print:text-black">Control Circuits</h1>
      <button class="px-4 py-2 rounded-lg bg-gray-100 hover:bg-gray-200 dark:bg-gray-800 dark:hover:bg-gray-700 text-gray-700 dark:text-gray-200 text-sm font-medium transition-colors print:hidden" onclick="window.print()">Print</button>
    </div>

    <div class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 overflow-x-auto">
      <table class="w-full text-sm">
        <thead class="bg-gray-50 dark:bg-gray-800 text-gray-600 dark:text-gray-400 text-xs uppercase tracking-wider print:text-black print:bg-white">
          <tr>
            <th class="px-4 py-3 text-left">Power Source</th>
            <th class="px-4 py-3 text-left">Circuit</th>
            <th class="px-4 py-3 text-left">Area</th>
            <th class="px-4 py-3 text-left">Description</th>
            <th class="px-4 py-3 text-left">Relay Type</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
          <tr v-if="!rows.length">
            <td colspan="5" class="px-4 py-8 text-center text-gray-400 dark:text-gray-500">No circuits defined.</td>
          </tr>
          <template v-for="(group, breakerName) in grouped" :key="breakerName">
            <tr v-for="(row, idx) in group" :key="row.id" class="hover:bg-gray-50 dark:hover:bg-gray-800/50 break-inside-avoid">
              <td class="px-4 py-3 text-gray-900 dark:text-gray-100 font-medium print:text-black">{{ idx === 0 ? breakerName : '' }}</td>
              <td class="px-4 py-3 text-gray-700 dark:text-gray-300 text-xs print:text-black">{{ row.name }}</td>
              <td class="px-4 py-3 text-gray-600 dark:text-gray-400 print:text-black">{{ row.areaName }}</td>
              <td class="px-4 py-3 text-gray-900 dark:text-gray-100 print:text-black">{{ row.description }}</td>
              <td class="px-4 py-3 text-gray-600 dark:text-gray-400 print:text-black">{{ row.relayType }}</td>
            </tr>
          </template>
        </tbody>
      </table>
    </div>
  </AppLayout>
</template>

<script setup>
import { computed, onMounted } from 'vue'
import AppLayout from '../../components/AppLayout.vue'
import { useCircuits } from '../../composables/useCircuits'
import { useBreakers } from '../../composables/useBreakers'
import { useAreas } from '../../composables/useAreas'

const { items: circuits, relayModels, load: loadCircuits } = useCircuits()
const { items: breakers, load: loadBreakers } = useBreakers()
const { items: areas, load: loadAreas } = useAreas()

onMounted(() => Promise.all([loadCircuits(), loadBreakers(), loadAreas()]))

const rows = computed(() => circuits.value.map(c => {
  const area = areas.value.find(a => a.id === c.area)
  const breaker = breakers.value.find(b => b.id === c.breaker)
  const relay = relayModels.value.find(r => r.id === c.relay_model)
  return {
    ...c,
    areaName: area?.name ?? '—',
    breakerName: breaker?.name ?? '—',
    relayType: relay ? `${relay.manufacturer} ${relay.model}` : '—'
  }
}).sort((a, b) => a.breakerName.localeCompare(b.breakerName) || a.name.localeCompare(b.name)))

const grouped = computed(() => {
  const g = {}
  rows.value.forEach(r => {
    if (!g[r.breakerName]) g[r.breakerName] = []
    g[r.breakerName].push(r)
  })
  return g
})
</script>
