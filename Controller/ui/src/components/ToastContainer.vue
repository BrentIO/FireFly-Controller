<template>
  <div class="fixed top-4 right-4 z-50 flex flex-col gap-2 w-80 print:hidden">
    <TransitionGroup name="toast" tag="div" class="flex flex-col gap-2">
      <div
        v-for="toast in toasts"
        :key="toast.id"
        class="rounded-lg px-4 py-3 shadow-lg flex items-start gap-3"
        :class="{
          'bg-green-50 border border-green-200 dark:bg-green-900/50 dark:border-green-700 text-green-800 dark:text-green-200': toast.type === 'success',
          'bg-amber-50 border border-amber-200 dark:bg-amber-900/50 dark:border-amber-700 text-amber-800 dark:text-amber-200': toast.type === 'warning',
          'bg-red-50 border border-red-200 dark:bg-red-900/50 dark:border-red-700 text-red-800 dark:text-red-200': toast.type === 'error'
        }"
      >
        <p class="flex-1 text-sm font-medium whitespace-pre-wrap">{{ toast.message }}</p>
        <button
          @click="removeToast(toast.id)"
          class="flex-shrink-0 ml-1 rounded hover:opacity-70 transition-opacity"
          aria-label="Dismiss"
        >
          <svg xmlns="http://www.w3.org/2000/svg" fill="none" viewBox="0 0 24 24" stroke-width="1.5" stroke="currentColor" class="w-4 h-4">
            <path stroke-linecap="round" stroke-linejoin="round" d="M6 18 18 6M6 6l12 12" />
          </svg>
        </button>
      </div>
    </TransitionGroup>
  </div>
</template>

<script setup>
import { useToast } from '../composables/useToast'

const { toasts, removeToast } = useToast()
</script>

<style scoped>
.toast-enter-active,
.toast-leave-active {
  transition: all 0.3s ease;
}
.toast-enter-from {
  opacity: 0;
  transform: translateX(100%);
}
.toast-leave-to {
  opacity: 0;
  transform: translateX(100%);
}
</style>
