<template>
  <div class="fixed top-4 right-4 z-50 flex flex-col gap-2 w-80">
    <div
      v-for="toast in toasts"
      :key="toast.id"
      class="flex items-start justify-between rounded-md px-4 py-3 shadow-lg text-sm text-white"
      :class="toast.type === 'success' ? 'bg-green-600' : 'bg-red-600'"
    >
      <span>{{ toast.message }}</span>
      <button
        v-if="toast.type === 'error'"
        class="ml-3 font-bold leading-none"
        @click="removeToast(toast.id)"
      >
        &times;
      </button>
    </div>
  </div>
</template>

<script setup>
import { useToast } from '../composables/useToast'

const { toasts, removeToast } = useToast()
</script>
