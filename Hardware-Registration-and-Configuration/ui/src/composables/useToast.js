import { ref } from 'vue'

const toasts = ref([])
let nextId = 0

export function useToast() {
  function addToast(type, message) {
    const id = nextId++
    toasts.value.push({ id, type, message })
    if (type === 'success') {
      setTimeout(() => removeToast(id), 5000)
    }
  }

  function removeToast(id) {
    toasts.value = toasts.value.filter(t => t.id !== id)
  }

  return { toasts, addToast, removeToast }
}
