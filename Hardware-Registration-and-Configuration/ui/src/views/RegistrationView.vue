<template>
  <AppLayout>
    <div class="max-w-2xl">
      <h2 class="text-xl font-semibold text-gray-900 dark:text-gray-100 mb-6">Cloud Registration</h2>

      <div v-if="loading" class="text-gray-500 dark:text-gray-400 text-sm">Loading…</div>

      <div v-else class="space-y-6">
        <!-- Status card -->
        <div class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 px-6 py-4 flex items-center gap-3">
          <span
            class="inline-flex items-center px-2.5 py-0.5 rounded-full text-xs font-medium"
            :class="registrationState.registered
              ? 'bg-green-100 text-green-800 dark:bg-green-900/30 dark:text-green-300'
              : 'bg-amber-100 text-amber-800 dark:bg-amber-900/30 dark:text-amber-300'"
          >
            {{ registrationState.registered ? 'Registered' : 'Not Registered' }}
          </span>
          <span v-if="registrationState.checkedAt" class="text-xs text-gray-500 dark:text-gray-400">
            Last checked: {{ new Date(registrationState.checkedAt * 1000).toLocaleString() }}
          </span>
        </div>

        <!-- Registration form -->
        <form
          class="bg-white dark:bg-gray-900 rounded-xl shadow-sm ring-1 ring-black/5 dark:ring-white/10 p-6 space-y-5"
          @submit.prevent="register"
        >
          <div>
            <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">FireFly-Cloud URL</label>
            <input
              v-model="form.url"
              type="url"
              class="block w-full border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 rounded-lg px-3 py-2 text-sm font-mono disabled:opacity-60 focus:outline-none focus:ring-2 focus:ring-blue-500"
              required
            />
          </div>

          <div>
            <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Registration Key</label>
            <input
              v-model="form.registrationKey"
              type="text"
              minlength="6"
              maxlength="6"
              class="block w-full border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 rounded-lg px-3 py-2 text-sm font-mono uppercase tracking-widest disabled:opacity-60 focus:outline-none focus:ring-2 focus:ring-blue-500"
              placeholder="ABC123"
              required
            />
            <p class="mt-1 text-xs text-gray-500 dark:text-gray-400">
              6-character one-time code obtained from a registered FireFly-Cloud user. Expires after 30 minutes.
            </p>
          </div>

          <div v-if="errorMessage" class="bg-red-50 dark:bg-red-900/20 border border-red-200 dark:border-red-700 text-red-800 dark:text-red-300 text-sm rounded-lg px-4 py-3">
            {{ errorMessage }}
          </div>

          <div class="pt-2">
            <button
              type="submit"
              :disabled="submitting"
              class="px-4 py-2 bg-blue-600 text-white text-sm font-medium rounded-lg hover:bg-blue-700 disabled:opacity-50 transition-colors"
            >
              {{ submitting ? 'Registering…' : 'Register Device' }}
            </button>
          </div>
        </form>
      </div>
    </div>
  </AppLayout>
</template>

<script setup>
import { ref, onMounted } from 'vue'
import AppLayout from '../components/AppLayout.vue'
import { apiFetch } from '../composables/useApi'
import { useToast } from '../composables/useToast'
import { useAppState } from '../composables/useAppState'

const { addToast } = useToast()
const { state, loadRegistrationState, setNavError } = useAppState()

const loading = ref(true)
const submitting = ref(false)
const errorMessage = ref('')

const registrationState = state.registrationState

const form = ref({
  url: 'https://api.fireflylx.com',
  registrationKey: ''
})

async function load() {
  loading.value = true
  try {
    await loadRegistrationState()
  } finally {
    loading.value = false
  }
}

async function register() {
  submitting.value = true
  errorMessage.value = ''
  try {
    const res = await apiFetch('/registration', {
      method: 'POST',
      headers: {
        'Content-Type': 'application/json',
        'X-Registration-Key': form.value.registrationKey.toUpperCase()
      },
      body: JSON.stringify({ url: form.value.url })
    })
    if (res.status === 204) {
      addToast('success', 'Device registered')
      setNavError('registration', false)
      form.value.registrationKey = ''
      await loadRegistrationState()
    } else if (res.status === 403) {
      errorMessage.value = 'Invalid or expired registration key. Obtain a new key from FireFly-Cloud and try again.'
    } else if (res.status === 502) {
      errorMessage.value = 'Unable to reach FireFly-Cloud. Check the URL and network connectivity.'
    } else if (res.status === 409) {
      errorMessage.value = 'Device identity is not provisioned. Complete the Identity step first.'
    } else if (res.status === 503) {
      errorMessage.value = 'Device clock is not synchronized. Wait for NTP sync and try again.'
    } else {
      const body = await res.json().catch(() => ({}))
      errorMessage.value = body.message ?? `Registration failed (${res.status})`
    }
  } catch (_) {
    errorMessage.value = 'Registration request error. Check network connectivity.'
  } finally {
    submitting.value = false
  }
}

onMounted(load)
</script>
