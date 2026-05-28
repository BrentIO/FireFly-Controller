import { reactive } from 'vue'

const state = reactive({
  uiVersion: ''
})

export function useAppState() {
  return { state }
}
