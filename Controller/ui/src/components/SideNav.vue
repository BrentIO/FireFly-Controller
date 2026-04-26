<template>
  <nav class="w-64 bg-gray-900 dark:bg-gray-950 text-white flex flex-col flex-shrink-0 h-screen border-r border-gray-700 dark:border-gray-800">
    <!-- Header -->
    <div class="px-4 py-4 flex items-center justify-between border-b border-gray-700 dark:border-gray-800">
      <span class="text-base font-semibold text-gray-100">FireFly Controller</span>
      <div class="flex items-center gap-1">
        <button
          class="p-1 rounded text-gray-400 hover:text-gray-100 hover:bg-gray-800 transition-colors md:hidden"
          aria-label="Close menu"
          @click="$emit('close')"
        >
          <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-4 h-4">
            <path stroke-linecap="round" stroke-linejoin="round" d="M6 18 18 6M6 6l12 12" />
          </svg>
        </button>
        <button
          class="p-1 rounded text-gray-400 hover:text-gray-100 hover:bg-gray-800 transition-colors"
          :aria-label="isDark ? 'Switch to light mode' : 'Switch to dark mode'"
          @click="toggleTheme"
        >
          <svg v-if="isDark" xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-4 h-4">
            <path stroke-linecap="round" stroke-linejoin="round" d="M12 3v2.25m6.364.386-1.591 1.591M21 12h-2.25m-.386 6.364-1.591-1.591M12 18.75V21m-4.773-4.227-1.591 1.591M5.25 12H3m4.227-4.773L5.636 5.636M15.75 12a3.75 3.75 0 1 1-7.5 0 3.75 3.75 0 0 1 7.5 0Z" />
          </svg>
          <svg v-else xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-4 h-4">
            <path stroke-linecap="round" stroke-linejoin="round" d="M21.752 15.002A9.72 9.72 0 0 1 18 15.75c-5.385 0-9.75-4.365-9.75-9.75 0-1.33.266-2.597.748-3.752A9.753 9.753 0 0 0 3 11.25C3 16.635 7.365 21 12.75 21a9.753 9.753 0 0 0 9.002-5.998Z" />
          </svg>
        </button>
      </div>
    </div>

    <!-- Nav items -->
    <ul class="flex-1 py-2 overflow-y-auto">
      <!-- Main items -->
      <li v-for="item in mainItems" :key="item.path">
        <RouterLink
          :to="item.path"
          class="flex items-center px-4 py-2 text-sm text-gray-300 hover:bg-gray-800 hover:text-gray-100 transition-colors rounded-sm mx-1"
          :class="{ 'bg-gray-800 text-gray-100': route.path === item.path }"
          @click="$emit('close')"
        >{{ item.label }}</RouterLink>
      </li>

      <!-- Configuration group -->
      <li class="mt-2">
        <button
          class="flex items-center w-full px-4 py-2 text-xs font-semibold text-gray-500 uppercase tracking-wider hover:text-gray-300 transition-colors"
          @click="toggleConfig"
        >
          <svg class="w-3 h-3 mr-1.5 transition-transform duration-150" :class="configOpen ? 'rotate-90' : ''" viewBox="0 0 24 24" fill="currentColor">
            <path d="M8.59 16.59L13.17 12 8.59 7.41 10 6l6 6-6 6z"/>
          </svg>
          Configuration
        </button>
        <ul v-show="configOpen">
          <li v-for="item in configItems" :key="item.path">
            <RouterLink
              :to="item.path"
              class="flex items-center pl-8 pr-4 py-2 text-sm text-gray-300 hover:bg-gray-800 hover:text-gray-100 transition-colors rounded-sm mx-1"
              :class="{ 'bg-gray-800 text-gray-100': route.path === item.path }"
              @click="$emit('close')"
            >{{ item.label }}</RouterLink>
          </li>
        </ul>
      </li>

      <!-- Reports group -->
      <li class="mt-1">
        <button
          class="flex items-center w-full px-4 py-2 text-xs font-semibold text-gray-500 uppercase tracking-wider hover:text-gray-300 transition-colors"
          @click="toggleReports"
        >
          <svg class="w-3 h-3 mr-1.5 transition-transform duration-150" :class="reportsOpen ? 'rotate-90' : ''" viewBox="0 0 24 24" fill="currentColor">
            <path d="M8.59 16.59L13.17 12 8.59 7.41 10 6l6 6-6 6z"/>
          </svg>
          Reports
        </button>
        <ul v-show="reportsOpen">
          <li v-for="item in reportItems" :key="item.path">
            <RouterLink
              :to="item.path"
              class="flex items-center pl-8 pr-4 py-2 text-sm text-gray-300 hover:bg-gray-800 hover:text-gray-100 transition-colors rounded-sm mx-1"
              :class="{ 'bg-gray-800 text-gray-100': route.path === item.path }"
              @click="$emit('close')"
            >{{ item.label }}</RouterLink>
          </li>
        </ul>
      </li>

      <!-- Validate -->
      <li class="mt-1">
        <RouterLink
          to="/validate"
          class="flex items-center px-4 py-2 text-sm text-gray-300 hover:bg-gray-800 hover:text-gray-100 transition-colors rounded-sm mx-1"
          :class="{ 'bg-gray-800 text-gray-100': route.path === '/validate' }"
          @click="$emit('close')"
        >Validate</RouterLink>
      </li>

      <!-- Utilities -->
      <li class="mt-2 border-t border-gray-700 dark:border-gray-800 pt-2">
        <RouterLink
          v-for="item in utilItems"
          :key="item.path"
          :to="item.path"
          class="flex items-center px-4 py-2 text-sm text-gray-300 hover:bg-gray-800 hover:text-gray-100 transition-colors rounded-sm mx-1"
          :class="{ 'bg-gray-800 text-gray-100': route.path === item.path }"
          @click="$emit('close')"
        >{{ item.label }}</RouterLink>
      </li>
    </ul>

    <!-- Footer -->
    <div class="px-4 py-3 text-xs text-gray-600 border-t border-gray-700 dark:border-gray-800 space-y-0.5">
      <div>{{ state.uiVersion }}</div>
    </div>
  </nav>
</template>

<script setup>
import { ref } from 'vue'
import { RouterLink, useRoute } from 'vue-router'
import { useAppState } from '../composables/useAppState'
import { useTheme } from '../composables/useTheme'

defineEmits(['close'])

const { state } = useAppState()
const { isDark, toggleTheme } = useTheme()
const route = useRoute()

const configOpen = ref(localStorage.getItem('nav_config_open') === 'true')
const reportsOpen = ref(localStorage.getItem('nav_reports_open') === 'true')

function toggleConfig() {
  configOpen.value = !configOpen.value
  localStorage.setItem('nav_config_open', configOpen.value)
}
function toggleReports() {
  reportsOpen.value = !reportsOpen.value
  localStorage.setItem('nav_reports_open', reportsOpen.value)
}

const mainItems = [
  { path: '/controllers', label: 'Controllers' },
  { path: '/clients', label: 'Clients' },
  { path: '/inputs', label: 'Inputs' },
  { path: '/outputs', label: 'Outputs' },
  { path: '/circuits', label: 'Circuits' },
  { path: '/breakers', label: 'Breakers' }
]

const configItems = [
  { path: '/config/areas', label: 'Areas' },
  { path: '/config/certificates', label: 'Certificates' },
  { path: '/config/colors', label: 'Colors' },
  { path: '/config/icons', label: 'Icons' },
  { path: '/config/mqtt', label: 'MQTT' },
  { path: '/config/ota', label: 'OTA Updates' },
  { path: '/config/relay-types', label: 'Relay Types' },
  { path: '/config/tags', label: 'Tags' },
  { path: '/config/wifi', label: 'WiFi' }
]

const reportItems = [
  { path: '/reports/bom', label: 'Bill of Materials' },
  { path: '/reports/control-circuits', label: 'Control Circuits' }
]

const utilItems = [
  { path: '/config/import', label: 'Import' },
  { path: '/config/reset', label: 'Reset' }
]
</script>
