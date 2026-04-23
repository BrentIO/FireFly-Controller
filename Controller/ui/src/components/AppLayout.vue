<template>
  <div class="flex h-screen bg-gray-50 dark:bg-gray-950 overflow-hidden">
    <!-- Mobile backdrop -->
    <Transition
      enter-active-class="ease-out duration-200"
      enter-from-class="opacity-0"
      enter-to-class="opacity-100"
      leave-active-class="ease-in duration-150"
      leave-from-class="opacity-100"
      leave-to-class="opacity-0"
    >
      <div
        v-if="sidebarOpen"
        class="fixed inset-0 z-20 bg-black/50 md:hidden print:hidden"
        @click="sidebarOpen = false"
      />
    </Transition>

    <!-- Sidebar -->
    <div
      class="fixed inset-y-0 left-0 z-30 transition-transform duration-200 md:relative md:translate-x-0 md:flex print:hidden"
      :class="sidebarOpen ? 'translate-x-0' : '-translate-x-full'"
    >
      <SideNav @close="sidebarOpen = false" />
    </div>

    <!-- Main content -->
    <div class="flex flex-col flex-1 min-w-0 overflow-hidden">
      <!-- Mobile top bar -->
      <div class="flex items-center gap-3 px-4 h-12 bg-gray-900 border-b border-gray-700 md:hidden flex-shrink-0 print:hidden">
        <button
          class="p-1 text-gray-400 hover:text-gray-100 transition-colors"
          aria-label="Open menu"
          @click="sidebarOpen = true"
        >
          <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-5 h-5">
            <path stroke-linecap="round" stroke-linejoin="round" d="M3.75 6.75h16.5M3.75 12h16.5m-16.5 5.25h16.5" />
          </svg>
        </button>
        <span class="text-sm font-semibold text-gray-100">FireFly Controller</span>
      </div>

      <main class="flex-1 overflow-auto p-6">
        <slot />
      </main>
    </div>
  </div>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import SideNav from './SideNav.vue'
import { useAppState } from '../composables/useAppState'

const { loadAppState } = useAppState()
const sidebarOpen = ref(false)

onMounted(loadAppState)
</script>
