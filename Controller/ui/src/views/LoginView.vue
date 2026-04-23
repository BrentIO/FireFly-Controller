<template>
  <div class="min-h-screen flex items-center justify-center bg-gray-50 dark:bg-gray-950 p-4">
    <div class="w-full max-w-sm">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 mb-2 text-center">FireFly Controller</h1>
      <p class="text-sm text-gray-500 dark:text-gray-400 text-center mb-8">Enter your visual authorization token to continue.</p>

      <form class="space-y-4" @submit.prevent="login">
        <div>
          <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Visual Token</label>
          <input
            v-model="token"
            type="text"
            required
            class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-sm focus:outline-none focus:ring-2 focus:ring-blue-500"
            placeholder="Enter token"
            :disabled="loading"
          />
        </div>

        <p v-if="error" class="text-sm text-red-600 dark:text-red-400">{{ error }}</p>

        <button
          type="submit"
          :disabled="loading || !token"
          class="w-full py-2.5 px-4 rounded-lg bg-blue-600 hover:bg-blue-700 disabled:opacity-50 text-white text-sm font-medium transition-colors min-h-[44px]"
        >
          {{ loading ? 'Signing in…' : 'Sign In' }}
        </button>
      </form>
    </div>
  </div>
</template>

<script setup>
import { ref } from 'vue'
import { useRouter } from 'vue-router'

const router = useRouter()
const token = ref('')
const loading = ref(false)
const error = ref('')

async function login() {
  error.value = ''
  loading.value = true
  try {
    const res = await fetch('/auth', {
      method: 'POST',
      signal: AbortSignal.timeout(5000),
      headers: { 'visual-token': token.value.trim() }
    })
    if (res.status === 204) {
      sessionStorage.setItem('visual_token', token.value.trim())
      router.push('/controllers')
    } else if (res.status === 401) {
      error.value = 'Invalid token. Please try again.'
    } else {
      error.value = `Authentication failed (${res.status}).`
    }
  } catch (e) {
    error.value = 'Could not reach the device. Check your network connection.'
  } finally {
    loading.value = false
  }
}
</script>
