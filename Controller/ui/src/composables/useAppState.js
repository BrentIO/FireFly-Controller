import { reactive } from 'vue'

const state = reactive({
  uiVersion: import.meta.env.VITE_UI_VERSION ?? 'dev'
})

export function useAppState() {
  return { state }
}
