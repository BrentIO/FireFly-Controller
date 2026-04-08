<template>
  <div>
    <table class="min-w-full divide-y divide-gray-200 text-sm">
      <thead class="bg-gray-50">
        <tr>
          <th
            v-for="col in columns"
            :key="col.key"
            class="px-4 py-3 text-left font-medium text-gray-500 uppercase tracking-wider"
            :class="col.sortable !== false ? 'cursor-pointer select-none hover:bg-gray-100' : ''"
            @click="col.sortable !== false && toggleSort(col.key)"
          >
            {{ col.label }}
            <span v-if="col.sortable !== false" class="ml-1 text-gray-400">
              <span v-if="sortKey === col.key">{{ sortDir === 'asc' ? '↑' : '↓' }}</span>
              <span v-else class="opacity-40">↕</span>
            </span>
          </th>
        </tr>
      </thead>
      <tbody class="bg-white divide-y divide-gray-200">
        <tr v-if="pagedRows.length === 0">
          <td :colspan="columns.length" class="px-4 py-6 text-center text-gray-400">
            No data.
          </td>
        </tr>
        <tr v-for="(row, i) in pagedRows" :key="i" class="hover:bg-gray-50">
          <td
            v-for="col in columns"
            :key="col.key"
            class="px-4 py-3 text-gray-700"
          >
            <slot :name="col.key" :row="row" :value="row[col.key]">
              {{ row[col.key] }}
            </slot>
          </td>
        </tr>
      </tbody>
    </table>

    <div v-if="pageSize" class="flex items-center justify-between px-4 py-3 border-t border-gray-200 bg-gray-50 text-sm">
      <div class="flex items-center gap-2 text-gray-600">
        <span>Rows per page:</span>
        <select
          v-model.number="currentPageSize"
          class="border border-gray-300 rounded px-2 py-1 text-sm"
          @change="currentPage = 1"
        >
          <option v-for="s in pageSizes" :key="s" :value="s">{{ s }}</option>
        </select>
        <span>{{ rangeLabel }}</span>
      </div>
      <div class="flex items-center gap-1">
        <button
          class="px-2 py-1 rounded border border-gray-300 disabled:opacity-40"
          :disabled="currentPage === 1"
          @click="currentPage--"
        >
          &lt;
        </button>
        <button
          class="px-2 py-1 rounded border border-gray-300 disabled:opacity-40"
          :disabled="currentPage === totalPages"
          @click="currentPage++"
        >
          &gt;
        </button>
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
  return [...props.rows].sort((a, b) => {
    const av = a[sortKey.value]
    const bv = b[sortKey.value]
    const cmp = typeof av === 'number' && typeof bv === 'number'
      ? av - bv
      : String(av ?? '').localeCompare(String(bv ?? ''))
    return sortDir.value === 'asc' ? cmp : -cmp
  })
})

const totalPages = computed(() =>
  props.pageSize ? Math.max(1, Math.ceil(sortedRows.value.length / currentPageSize.value)) : 1
)

const pagedRows = computed(() => {
  if (!props.pageSize) return sortedRows.value
  const start = (currentPage.value - 1) * currentPageSize.value
  return sortedRows.value.slice(start, start + currentPageSize.value)
})

const rangeLabel = computed(() => {
  const total = sortedRows.value.length
  if (!props.pageSize) return `${total} rows`
  const start = Math.min((currentPage.value - 1) * currentPageSize.value + 1, total)
  const end = Math.min(currentPage.value * currentPageSize.value, total)
  return `${start}–${end} of ${total}`
})
</script>
