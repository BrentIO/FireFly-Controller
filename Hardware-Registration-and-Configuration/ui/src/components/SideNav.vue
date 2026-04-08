<template>
  <nav class="w-56 bg-gray-800 text-white flex flex-col flex-shrink-0 h-screen">
    <div class="px-4 py-4 text-base font-semibold border-b border-gray-700">
      Hardware Configuration
    </div>
    <ul class="flex-1 py-2 overflow-y-auto">
      <li v-for="item in navItems" :key="item.path">
        <span
          v-if="item.disabled"
          class="flex items-center px-4 py-2 text-gray-500 cursor-not-allowed text-sm"
        >
          {{ item.label }}
        </span>
        <RouterLink
          v-else
          :to="item.path"
          class="flex items-center px-4 py-2 text-sm hover:bg-gray-700 transition-colors"
          :class="{ 'bg-gray-700': route.path === item.path }"
        >
          {{ item.label }}
          <span v-if="state.navErrors[item.key]" class="ml-auto text-red-400 font-bold">!</span>
        </RouterLink>
      </li>
    </ul>
    <div class="px-4 py-3 text-xs text-gray-500 border-t border-gray-700 space-y-0.5">
      <div>API: {{ state.apiVersion || '—' }}</div>
      <div>UI: {{ uiVersion }}</div>
    </div>
  </nav>
</template>

<script setup>
import { computed } from 'vue'
import { RouterLink, useRoute } from 'vue-router'
import { useAppState } from '../composables/useAppState'

const { state } = useAppState()
const route = useRoute()

const uiVersion = import.meta.env.VITE_UI_VERSION ?? 'dev'

const navItems = computed(() => [
  { path: '/mcu',         label: 'MCU',             key: 'mcu' },
  { path: '/network',     label: 'Network',          key: 'network' },
  { path: '/identity',    label: 'Identity',         key: 'identity' },
  { path: '/partitions',  label: 'Partition Table',  key: 'partitions',  disabled: !state.identityLoaded },
  { path: '/peripherals', label: 'Peripherals',      key: 'peripherals', disabled: !state.identityLoaded },
  { path: '/events',      label: 'Event Log',        key: 'events' },
  { path: '/errors',      label: 'Error Log',        key: 'errors' }
])
</script>
