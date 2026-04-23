<template>
  <RouterView />
  <ToastContainer />
</template>

<script setup>
import { onMounted } from 'vue'
import { RouterView } from 'vue-router'
import ToastContainer from './components/ToastContainer.vue'
import { useTheme } from './composables/useTheme'

const { applyTheme } = useTheme()
onMounted(applyTheme)
</script>
