<template>
  <nav class="w-56 bg-gray-900 dark:bg-gray-950 text-white flex flex-col flex-shrink-0 h-screen border-r border-gray-700 dark:border-gray-800">
    <div class="px-4 py-4 flex items-center justify-between border-b border-gray-700 dark:border-gray-800">
      <span class="text-base font-semibold text-gray-100">Hardware Configuration</span>
      <div class="flex items-center gap-1">
        <!-- Close button — mobile only -->
        <button
          class="p-1 rounded text-gray-400 hover:text-gray-100 hover:bg-gray-800 transition-colors md:hidden"
          aria-label="Close menu"
          @click="$emit('close')"
        >
          <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-4 h-4">
            <path stroke-linecap="round" stroke-linejoin="round" d="M6 18 18 6M6 6l12 12" />
          </svg>
        </button>
        <!-- Theme toggle -->
        <button
          class="p-1 rounded text-gray-400 hover:text-gray-100 hover:bg-gray-800 transition-colors"
          :aria-label="isDark ? 'Switch to light mode' : 'Switch to dark mode'"
          @click="toggleTheme"
        >
          <!-- Sun (shown in dark mode — click to go light) -->
          <svg v-if="isDark" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-4 h-4">
            <path stroke-linecap="round" stroke-linejoin="round" d="M12 3v2.25m6.364.386-1.591 1.591M21 12h-2.25m-.386 6.364-1.591-1.591M12 18.75V21m-4.773-4.227-1.591 1.591M5.25 12H3m4.227-4.773L5.636 5.636M15.75 12a3.75 3.75 0 1 1-7.5 0 3.75 3.75 0 0 1 7.5 0Z" />
          </svg>
          <!-- Moon (shown in light mode — click to go dark) -->
          <svg v-else xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-4 h-4">
            <path stroke-linecap="round" stroke-linejoin="round" d="M21.752 15.002A9.72 9.72 0 0 1 18 15.75c-5.385 0-9.75-4.365-9.75-9.75 0-1.33.266-2.597.748-3.752A9.753 9.753 0 0 0 3 11.25C3 16.635 7.365 21 12.75 21a9.753 9.753 0 0 0 9.002-5.998Z" />
          </svg>
        </button>
      </div>
    </div>

    <ul class="flex-1 py-2 overflow-y-auto">
      <li v-for="item in navItems" :key="item.path">
        <span
          v-if="item.disabled"
          class="flex items-center px-4 py-2 text-sm text-gray-600 cursor-not-allowed"
        >
          {{ item.label }}
        </span>
        <RouterLink
          v-else
          :to="item.path"
          class="flex items-center px-4 py-2 text-sm text-gray-300 hover:bg-gray-800 hover:text-gray-100 transition-colors rounded-sm mx-1"
          :class="{ 'bg-gray-800 text-gray-100': route.path === item.path }"
          @click="$emit('close')"
        >
          {{ item.label }}
          <span v-if="state.navErrors[item.key]" class="ml-auto" aria-label="Warning">
            <!-- MDI alert icon (triangle with exclamation) -->
            <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 24 24" fill="currentColor" class="w-4 h-4 text-red-400">
              <path d="M13 14H11V9H13M13 18H11V16H13M1 21H23L12 2L1 21Z" />
            </svg>
          </span>
        </RouterLink>
      </li>
    </ul>

    <div class="px-4 py-3 text-xs text-gray-600 border-t border-gray-700 dark:border-gray-800 space-y-0.5">
      <div>API: {{ state.apiVersion || '—' }}</div>
      <div>UI: {{ uiVersion }}</div>
    </div>
  </nav>
</template>

<script setup>
import { computed } from 'vue'
import { RouterLink, useRoute } from 'vue-router'
import { useAppState } from '../composables/useAppState'
import { useTheme } from '../composables/useTheme'

defineEmits(['close'])

const { state } = useAppState()
const { isDark, toggleTheme } = useTheme()
const route = useRoute()

const uiVersion = import.meta.env.VITE_UI_VERSION ?? 'dev'

const navItems = computed(() => [
  { path: '/mcu',         label: 'MCU',            key: 'mcu' },
  { path: '/network',     label: 'Network',         key: 'network' },
  { path: '/identity',    label: 'Identity',        key: 'identity' },
  { path: '/partitions',  label: 'Partition Table', key: 'partitions',  disabled: !state.identityLoaded },
  { path: '/peripherals', label: 'Peripherals',     key: 'peripherals', disabled: !state.identityLoaded },
  { path: '/events',      label: 'Event Log',       key: 'events' },
  { path: '/errors',      label: 'Error Log',       key: 'errors' }
])
</script>
