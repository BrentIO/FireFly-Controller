<template>
  <div class="flex h-screen bg-gray-100">
    <SideNav />
    <main class="flex-1 overflow-auto p-6">
      <slot />
    </main>
  </div>
</template>

<script setup>
import { onMounted } from 'vue'
import SideNav from './SideNav.vue'
import { useAppState } from '../composables/useAppState'

const { loadAppState } = useAppState()

onMounted(loadAppState)
</script>
