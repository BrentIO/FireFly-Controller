<template>
  <div class="min-h-screen bg-gray-100 flex items-center justify-center">
    <div class="bg-white rounded-lg shadow-md w-full max-w-sm p-8">
      <h1 class="text-xl font-semibold text-gray-900 mb-6">Hardware Configuration</h1>
      <form @submit.prevent="submit">
        <label class="block text-sm font-medium text-gray-700 mb-1">Authorization Token</label>
        <input
          v-model="token"
          type="text"
          minlength="8"
          maxlength="8"
          autocomplete="off"
          class="block w-full border border-gray-300 rounded-md px-3 py-2 text-sm font-mono mb-4 focus:outline-none focus:ring-2 focus:ring-blue-500"
          placeholder="Enter 8-character token"
          required
        />
        <button
          type="submit"
          :disabled="loading"
          class="w-full bg-blue-600 text-white py-2 rounded-md text-sm font-medium hover:bg-blue-700 disabled:opacity-50"
        >
          {{ loading ? 'Authorizing…' : 'Authorize' }}
        </button>
      </form>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'
import { useToast } from '../composables/useToast'

const router = useRouter()
const { addToast } = useToast()
const token = ref('')
const loading = ref(false)

async function submit() {
  loading.value = true
  try {
    const res = await fetch('/auth', {
      method: 'POST',
      headers: { 'visual-token': token.value }
    })
    if (res.status === 204) {
      sessionStorage.setItem('visual_token', token.value)
      router.push('/mcu')
    } else {
      addToast('error', 'Invalid token')
    }
  } catch (_) {
    addToast('error', 'Connection error')
  } finally {
    loading.value = false
  }
}
</script>
