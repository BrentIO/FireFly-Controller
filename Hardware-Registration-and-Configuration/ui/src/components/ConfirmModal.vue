<template>
  <Teleport to="body">
    <Transition
      enter-active-class="ease-out duration-200"
      enter-from-class="opacity-0"
      enter-to-class="opacity-100"
      leave-active-class="ease-in duration-150"
      leave-from-class="opacity-100"
      leave-to-class="opacity-0"
    >
      <div
        v-if="show"
        class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50 transition-opacity"
        @click="$emit('cancel')"
      >
        <Transition
          enter-active-class="ease-out duration-200"
          enter-from-class="opacity-0 scale-95"
          enter-to-class="opacity-100 scale-100"
          leave-active-class="ease-in duration-150"
          leave-from-class="opacity-100 scale-100"
          leave-to-class="opacity-0 scale-95"
        >
          <div
            v-if="show"
            class="relative w-full max-w-md rounded-xl bg-white dark:bg-gray-900 shadow-xl ring-1 ring-black/10 dark:ring-white/10 p-6 transition-all"
            @click.stop
          >
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-2">{{ title }}</h3>
            <p class="text-sm text-gray-600 dark:text-gray-400 mb-4">{{ message }}</p>

            <div
              v-if="details && Object.keys(details).length"
              class="mb-4 rounded-lg bg-gray-50 dark:bg-gray-800 border border-gray-200 dark:border-gray-700 divide-y divide-gray-200 dark:divide-gray-700"
            >
              <div v-for="(val, key) in details" :key="key" class="px-3 py-2">
                <p class="text-xs text-gray-500 dark:text-gray-400 mb-0.5">{{ key }}</p>
                <p class="text-sm font-medium text-gray-900 dark:text-gray-100">{{ val }}</p>
              </div>
            </div>

            <div class="flex gap-3 justify-end">
              <button
                type="button"
                class="px-4 py-2.5 text-sm font-medium text-gray-700 dark:text-gray-300 bg-white dark:bg-gray-800 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-700 transition-colors min-h-[44px]"
                @click="$emit('cancel')"
              >
                Cancel
              </button>
              <button
                type="button"
                class="px-4 py-2.5 text-sm font-medium text-white rounded-lg transition-colors min-h-[44px]"
                :class="{
                  'bg-green-600 hover:bg-green-700': variant === 'success',
                  'bg-amber-500 hover:bg-amber-600': variant === 'warning',
                  'bg-red-600 hover:bg-red-700': variant === 'danger' || !['success', 'warning'].includes(variant)
                }"
                @click="$emit('confirm')"
              >
                {{ confirmLabel }}
              </button>
            </div>
          </div>
        </Transition>
      </div>
    </Transition>
  </Teleport>
</template>

<script setup>
defineProps({
  show: Boolean,
  title: { type: String, default: 'Confirm' },
  message: { type: String, default: 'Are you sure?' },
  details: { type: Object, default: null },
  variant: { type: String, default: 'danger' },
  confirmLabel: { type: String, default: 'Confirm' }
})
defineEmits(['confirm', 'cancel'])
</script>
