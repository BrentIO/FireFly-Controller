<template>
  <div class="fixed top-4 right-4 z-50 flex flex-col gap-2 w-80 print:hidden">
    <div
      v-for="toast in toasts"
      :key="toast.id"
      class="flex items-start justify-between rounded-lg px-4 py-3 shadow-lg text-sm text-white"
      :class="{
        'bg-green-600': toast.type === 'success',
        'bg-amber-500': toast.type === 'warning',
        'bg-red-600': toast.type === 'error'
      }"
    >
      <span class="whitespace-pre-wrap">{{ toast.message }}</span>
      <button
        v-if="toast.type === 'error'"
        class="ml-3 font-bold leading-none opacity-80 hover:opacity-100 flex-shrink-0"
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
