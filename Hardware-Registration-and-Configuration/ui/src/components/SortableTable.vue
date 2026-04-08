<template>
  <div class="flex flex-col">
    <div class="overflow-x-auto">
      <table class="w-full text-sm">
        <thead>
          <tr class="bg-gray-50 dark:bg-gray-800 border-b border-gray-200 dark:border-gray-700">
            <th
              v-for="col in columns"
              :key="col.key"
              class="text-left px-4 py-3 text-xs font-semibold text-gray-500 dark:text-gray-400 whitespace-nowrap select-none"
              :class="col.sortable !== false ? 'cursor-pointer hover:text-gray-700 dark:hover:text-gray-200' : ''"
              @click="col.sortable !== false && toggleSort(col.key)"
            >
              {{ col.label }}
              <template v-if="col.sortable !== false">
                <!-- ChevronUp -->
                <svg
                  v-if="sortKey === col.key && sortDir === 'asc'"
                  xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24"
                  stroke-width="2" stroke="currentColor"
                  class="inline w-3 h-3 ml-0.5 align-middle"
                >
                  <path stroke-linecap="round" stroke-linejoin="round" d="m4.5 15.75 7.5-7.5 7.5 7.5" />
                </svg>
                <!-- ChevronDown -->
                <svg
                  v-else-if="sortKey === col.key && sortDir === 'desc'"
                  xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24"
                  stroke-width="2" stroke="currentColor"
                  class="inline w-3 h-3 ml-0.5 align-middle"
                >
                  <path stroke-linecap="round" stroke-linejoin="round" d="m19.5 8.25-7.5 7.5-7.5-7.5" />
                </svg>
              </template>
            </th>
          </tr>
        </thead>
        <tbody>
          <tr v-if="pagedRows.length === 0">
            <td
              :colspan="columns.length"
              class="px-4 py-12 text-center text-sm text-gray-500 dark:text-gray-400"
            >
              No data.
            </td>
          </tr>
          <tr
            v-else
            v-for="(row, i) in pagedRows"
            :key="i"
            class="border-b border-gray-100 dark:border-gray-800 odd:bg-white even:bg-gray-50 dark:odd:bg-gray-900 dark:even:bg-gray-800/50 hover:bg-blue-50 dark:hover:bg-blue-900/20 transition-colors"
          >
            <td
              v-for="col in columns"
              :key="col.key"
              class="px-4 py-3 text-gray-700 dark:text-gray-300"
            >
              <slot :name="col.key" :row="row" :value="row[col.key]">
                {{ row[col.key] }}
              </slot>
            </td>
          </tr>
        </tbody>
      </table>
    </div>

    <!-- Pagination -->
    <div
      v-if="pageSize"
      class="flex items-center justify-between px-4 py-3 border-t border-gray-200 dark:border-gray-700 flex-wrap gap-3"
    >
      <p class="text-xs text-gray-500 dark:text-gray-400">
        Showing {{ showingFrom }}–{{ showingTo }} of {{ sortedRows.length }} results
      </p>

      <div class="flex items-center gap-1">
        <button
          class="px-2.5 py-1.5 text-xs rounded border border-gray-300 dark:border-gray-600 text-gray-600 dark:text-gray-400 hover:bg-gray-50 dark:hover:bg-gray-800 disabled:opacity-40 disabled:cursor-not-allowed transition-colors"
          :disabled="currentPage === 1"
          @click="currentPage--"
        >
          Previous
        </button>
        <button
          v-for="pg in pageNumbers"
          :key="pg"
          class="min-w-[2rem] px-2.5 py-1.5 text-xs rounded border transition-colors"
          :class="pg === currentPage
            ? 'bg-blue-600 border-blue-600 text-white'
            : 'border-gray-300 dark:border-gray-600 text-gray-600 dark:text-gray-400 hover:bg-gray-50 dark:hover:bg-gray-800'"
          @click="currentPage = pg"
        >
          {{ pg }}
        </button>
        <button
          class="px-2.5 py-1.5 text-xs rounded border border-gray-300 dark:border-gray-600 text-gray-600 dark:text-gray-400 hover:bg-gray-50 dark:hover:bg-gray-800 disabled:opacity-40 disabled:cursor-not-allowed transition-colors"
          :disabled="currentPage === totalPages"
          @click="currentPage++"
        >
          Next
        </button>
      </div>

      <div class="flex items-center gap-2">
        <label class="text-xs text-gray-500 dark:text-gray-400">Rows per page</label>
        <select
          v-model.number="currentPageSize"
          class="text-xs rounded border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-700 dark:text-gray-300 px-2 py-1 focus:outline-none focus:ring-1 focus:ring-blue-500"
          @change="currentPage = 1"
        >
          <option v-for="s in pageSizes" :key="s" :value="s">{{ s }}</option>
        </select>
      </div>
    </div>
  </div>
</template>

<script setup>
import { ref, computed, watch } from 'vue'

const props = defineProps({
  columns: { type: Array, required: true },
  rows: { type: Array, required: true },
  defaultSort: { type: Object, default: null },
  pageSize: { type: Number, default: null },
  pageSizes: { type: Array, default: () => [10, 25, 50, 100] }
})

const sortKey = ref(props.defaultSort?.key ?? null)
const sortDir = ref(props.defaultSort?.dir ?? 'asc')
const currentPage = ref(1)
const currentPageSize = ref(props.pageSize ?? 10)

watch(() => props.rows, () => { currentPage.value = 1 })

function toggleSort(key) {
  if (sortKey.value === key) {
    sortDir.value = sortDir.value === 'asc' ? 'desc' : 'asc'
  } else {
    sortKey.value = key
    sortDir.value = 'asc'
  }
  currentPage.value = 1
}

const sortedRows = computed(() => {
  if (!sortKey.value) return props.rows
  const dir = sortDir.value === 'asc' ? 1 : -1
  return [...props.rows].sort((a, b) => {
    const av = a[sortKey.value]
    const bv = b[sortKey.value]
    if (typeof av === 'number' && typeof bv === 'number') return (av - bv) * dir
    return String(av ?? '').localeCompare(String(bv ?? '')) * dir
  })
})

const totalPages = computed(() =>
  props.pageSize ? Math.max(1, Math.ceil(sortedRows.value.length / currentPageSize.value)) : 1
)

const pageNumbers = computed(() => {
  const total = totalPages.value
  const current = currentPage.value
  if (total <= 5) return Array.from({ length: total }, (_, i) => i + 1)
  let start = Math.max(1, current - 2)
  let end = start + 4
  if (end > total) { end = total; start = Math.max(1, end - 4) }
  return Array.from({ length: end - start + 1 }, (_, i) => start + i)
})

const pagedRows = computed(() => {
  if (!props.pageSize) return sortedRows.value
  const start = (currentPage.value - 1) * currentPageSize.value
  return sortedRows.value.slice(start, start + currentPageSize.value)
})

const showingFrom = computed(() => {
  const total = sortedRows.value.length
  return total === 0 ? 0 : (currentPage.value - 1) * currentPageSize.value + 1
})
const showingTo = computed(() =>
  Math.min(currentPage.value * currentPageSize.value, sortedRows.value.length)
)
</script>
