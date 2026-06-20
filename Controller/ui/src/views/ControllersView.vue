<template>
  <AppLayout>
    <div class="flex flex-wrap items-center gap-3 justify-between mb-6 print:mb-4">
      <h1 class="text-2xl font-bold text-gray-900 dark:text-gray-100 print:text-xl print:!text-black">Controllers</h1>
      <div class="flex gap-2 print:hidden flex-shrink-0">
        <button class="px-4 py-2 rounded-lg bg-gray-100 hover:bg-gray-200 dark:bg-gray-800 dark:hover:bg-gray-700 text-gray-700 dark:text-gray-200 text-sm font-medium transition-colors" @click="printLandscape">Print</button>
        <button v-if="!isCloudMode" class="px-4 py-2 rounded-lg bg-amber-600 text-white text-sm font-medium transition-colors" :class="hasConnectedControllers ? 'hover:bg-amber-700' : 'opacity-40 cursor-not-allowed'" :disabled="!hasConnectedControllers" @click="deployAll">Deploy All</button>
        <button class="px-4 py-2 rounded-lg bg-blue-600 hover:bg-blue-700 text-white text-sm font-medium transition-colors" @click="openAdd">Add Controller</button>
      </div>
    </div>

    <!-- Print-only table -->
    <div class="hidden print:block mb-6">
      <table class="w-full text-xs text-black">
        <thead class="uppercase tracking-wider border-b border-black">
          <tr>
            <th class="py-2 text-left font-semibold">Name</th>
            <th class="py-2 text-left font-semibold">Area</th>
            <th class="py-2 text-left font-semibold">Product ID</th>
            <th class="py-2 text-left font-semibold">UUID</th>
            <th class="py-2 text-left font-semibold">MAC Address</th>
          </tr>
        </thead>
        <tbody class="divide-y divide-gray-300 [print-color-adjust:exact]">
          <tr v-for="ctrl in items" :key="ctrl.id" class="print:even:!bg-gray-100">
            <td class="py-2 font-semibold">{{ ctrl.name }}</td>
            <td class="py-2">{{ areaName(ctrl.area) }}</td>
            <td class="py-2">{{ ctrl.product }}</td>
            <td class="py-2 font-mono">{{ ctrl.uuid }}</td>
            <td class="py-2 font-mono">{{ ctrl.mac || '—' }}</td>
          </tr>
        </tbody>
      </table>
    </div>

    <div class="grid gap-4 grid-cols-1 sm:grid-cols-2 xl:grid-cols-3 print:hidden">
      <div v-if="items.length === 0" class="text-gray-400 dark:text-gray-500 py-8 col-span-full">No controllers defined.</div>

      <div v-for="ctrl in items" :key="ctrl.id"
        class="bg-white dark:bg-gray-900 rounded-xl border border-gray-200 dark:border-gray-700 p-4 break-inside-avoid">
        <!-- Header -->
        <div class="flex items-start justify-between gap-2 mb-3">
          <div>
            <div class="flex items-center gap-2 flex-wrap">
              <p class="font-semibold text-gray-900 dark:text-gray-100">{{ ctrl.name }}</p>
              <span v-if="isDeployRequired(ctrl)"
                class="px-1.5 py-0.5 text-xs font-medium rounded bg-amber-100 text-amber-800 dark:bg-amber-900/30 dark:text-amber-200 print:hidden">
                Deployment Required
              </span>
            </div>
            <p class="text-xs text-gray-500 dark:text-gray-400 font-mono">{{ ctrl.uuid }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400 font-mono">{{ ctrl.mac || '—' }}</p>
            <p class="text-xs text-gray-500 dark:text-gray-400">{{ ctrl.product }}</p>
            <template v-if="sessions[ctrl.id]?.isAuthenticated && versionCache[ctrl.id]">
              <p class="text-xs text-gray-500 dark:text-gray-400">API: {{ versionCache[ctrl.id].app }}</p>
              <p class="text-xs text-gray-500 dark:text-gray-400">UI: {{ versionCache[ctrl.id].ui }}</p>
            </template>
            <p class="text-xs text-gray-500 dark:text-gray-400">{{ areaName(ctrl.area) }}</p>
            <p v-if="!ctrl.mac || ctrl.mac === 'ff:ff:ff:ff:ff:ff'" class="text-xs text-yellow-700 dark:text-yellow-500 mt-1 font-medium">MAC Address is invalid</p>
          </div>
          <div class="flex gap-2 print:hidden flex-shrink-0">
            <button class="px-2.5 py-1 text-xs font-medium rounded border border-gray-300 dark:border-gray-600 text-gray-700 dark:text-gray-300 hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="openEdit(ctrl)">Edit</button>
            <button class="px-2.5 py-1 text-xs font-medium rounded border border-red-200 dark:border-red-800 text-red-600 dark:text-red-400 hover:bg-red-50 dark:hover:bg-red-900/20 transition-colors" @click="confirmDelete(ctrl)">Delete</button>
          </div>
        </div>

        <!-- Auth / deploy section -->
        <div v-if="!isCloudMode || sessions[ctrl.id]?.isAuthenticated" class="print:hidden border-t border-gray-100 dark:border-gray-800 pt-3 mt-3 space-y-2">
          <div v-if="!isCloudMode && !sessions[ctrl.id]?.isAuthenticated" class="space-y-2">
            <input v-model="ipInputs[ctrl.id]" type="text" placeholder="192.168.1.x"
              class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-gray-50 dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-2 py-1.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500"
              @dblclick="ipInputs[ctrl.id] = window.location.hostname" />
            <div class="flex gap-2">
              <input v-model="tokenInputs[ctrl.id]" type="text" placeholder="Visual token" maxlength="6"
                class="flex-1 min-w-0 rounded-lg border border-gray-300 dark:border-gray-600 bg-gray-50 dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-2 py-1.5 text-sm font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
              <button class="px-3 py-1.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors flex-shrink-0" @click="authenticate(ctrl.id)">Connect</button>
            </div>
          </div>
          <div v-else-if="sessions[ctrl.id]?.isAuthenticated" class="space-y-2">
            <div class="flex items-center justify-between">
              <span class="text-xs text-green-600 dark:text-green-400 font-medium">Connected · {{ sessions[ctrl.id].ip }}</span>
              <button class="text-xs text-gray-500 hover:text-gray-700 dark:hover:text-gray-300 underline" @click="logout(ctrl.id)">Disconnect</button>
            </div>
            <div class="flex flex-col gap-1.5">
              <button class="w-full px-2 py-1.5 text-sm font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="openEventLog(ctrl.id)">View Event Log</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="openErrorLog(ctrl.id)">View Error Log</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmPullBackup(ctrl)">Pull Backup</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="pushCertificates(ctrl)">Push Certs</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium border rounded-lg transition-colors"
                :class="[
                  isCloudMode ? 'opacity-40 cursor-not-allowed' : '',
                  sessions[ctrl.id]?.provisioningModeEnabled
                    ? 'bg-green-600 hover:bg-green-700 text-white border-transparent'
                    : 'border-green-500 text-green-600 hover:bg-green-50 dark:hover:bg-green-900/20'
                ]"
                :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="toggleProvisioning(ctrl.id)">{{ sessions[ctrl.id]?.provisioningModeEnabled ? 'Disable Provisioning Mode' : 'Enable Provisioning Mode' }}</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmOta(ctrl)">Force OTA Update</button>
              <button
                class="w-full px-2 py-1.5 text-sm font-medium rounded-lg transition-colors"
                :class="[
                  isCloudMode ? 'opacity-40 cursor-not-allowed' : '',
                  isDeployRequired(ctrl)
                    ? 'bg-amber-600 text-white hover:bg-amber-700'
                    : 'border border-amber-600 text-amber-700 dark:text-amber-400 hover:bg-amber-50 dark:hover:bg-amber-900/20'
                ]"
                :disabled="isCloudMode"
                :title="isCloudMode ? 'Not available in hosted mode' : undefined"
                @click="deploy(ctrl)">Deploy</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="pushCloudBackup(ctrl)">Push Cloud Backup</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-gray-50 dark:hover:bg-gray-800'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmCloudRestore(ctrl)">Restore Cloud Backup</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium text-red-700 dark:text-red-300 border border-red-300 dark:border-red-700 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-red-50 dark:hover:bg-red-900/20'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmCloudDelete(ctrl)">Delete Cloud Backup</button>
              <button class="w-full px-2 py-1.5 text-sm font-medium text-red-700 dark:text-red-300 border border-red-300 dark:border-red-700 rounded-lg transition-colors" :class="isCloudMode ? 'opacity-40 cursor-not-allowed' : 'hover:bg-red-50 dark:hover:bg-red-900/20'" :disabled="isCloudMode" :title="isCloudMode ? 'Not available in hosted mode' : undefined" @click="confirmReboot(ctrl)">Reboot</button>
            </div>
          </div>
        </div>
      </div>
    </div>

    <!-- Add/Edit modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showModal" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
          <div class="w-full max-w-md bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6">
            <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-4">{{ editing ? 'Edit Controller' : 'Add Controller' }}</h3>
            <form @submit.prevent="save" class="space-y-4">
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Name <span class="text-xs font-normal text-gray-400">(max 20 chars)</span></label>
                <input v-model="form.name" type="text" maxlength="20" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Area</label>
                <select v-model.number="form.area" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select area…</option>
                  <option v-for="a in areas" :key="a.id" :value="a.id">{{ a.name }}</option>
                </select>
              </div>
              <div>
                <label class="block text-sm font-medium text-gray-700 dark:text-gray-300 mb-1">Product</label>
                <select v-model="form.product" required class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base focus:outline-none focus:ring-2 focus:ring-blue-500">
                  <option value="">Select product…</option>
                  <option v-for="p in products" :key="p.pid" :value="p.pid">{{ p.pid }}</option>
                </select>
              </div>
              <div>
                <div class="flex items-center justify-between mb-1">
                  <label class="block text-sm font-medium text-gray-700 dark:text-gray-300">MAC Address</label>
                  <button type="button" class="text-xs text-blue-600 dark:text-blue-400 hover:underline" @click="generateMac">Generate</button>
                </div>
                <input v-model="form.mac" type="text" placeholder="aa:bb:cc:dd:ee:ff"
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base font-mono focus:outline-none focus:ring-2 focus:ring-blue-500"
                  :class="macError ? 'border-red-400 focus:ring-red-400' : ''" />
                <p v-if="macError" class="mt-1 text-xs text-red-600 dark:text-red-400">{{ macError }}</p>
                <div v-if="form.mac === 'ff:ff:ff:ff:ff:ff'" class="mt-1 flex items-start gap-1.5 text-xs text-yellow-600 dark:text-yellow-400">
                  <svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 20 20" fill="currentColor" class="w-3.5 h-3.5 flex-shrink-0 mt-px"><path fill-rule="evenodd" d="M8.485 2.495c.673-1.167 2.357-1.167 3.03 0l6.28 10.875c.673 1.167-.17 2.625-1.516 2.625H3.72c-1.347 0-2.189-1.458-1.515-2.625L8.485 2.495zM10 5a.75.75 0 01.75.75v3.5a.75.75 0 01-1.5 0v-3.5A.75.75 0 0110 5zm0 9a1 1 0 100-2 1 1 0 000 2z" clip-rule="evenodd" /></svg>
                  <span>Placeholder MAC — update before provisioning.</span>
                </div>
              </div>
              <div>
                <div class="flex items-center justify-between mb-1">
                  <label class="block text-sm font-medium text-gray-700 dark:text-gray-300">UUID</label>
                  <button type="button" class="text-xs text-blue-600 dark:text-blue-400 hover:underline" @click="form.uuid = randomUUID()">Generate</button>
                </div>
                <input v-model="form.uuid" type="text" required pattern="[0-9a-f]{8}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{4}-[0-9a-f]{12}"
                  class="w-full rounded-lg border border-gray-300 dark:border-gray-600 bg-white dark:bg-gray-800 text-gray-900 dark:text-gray-100 px-3 py-2.5 text-base font-mono focus:outline-none focus:ring-2 focus:ring-blue-500" />
              </div>
              <div class="flex gap-3 justify-end pt-2">
                <button type="button" class="px-4 py-2.5 text-sm text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="showModal = false">Cancel</button>
                <button type="submit" class="px-4 py-2.5 text-sm font-medium text-white bg-blue-600 hover:bg-blue-700 rounded-lg transition-colors">Save</button>
              </div>
            </form>
          </div>
        </div>
      </Transition>
    </Teleport>


    <!-- Event log modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showEventLog" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
          <div class="w-full max-w-2xl bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6 max-h-[80vh] flex flex-col">
            <div class="flex items-center justify-between mb-4">
              <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100">Event Log</h3>
              <div class="flex items-center gap-3">
                <button class="px-3 py-1 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="refreshEventLog">Refresh</button>
                <button class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 text-xl leading-none" @click="showEventLog = false">&times;</button>
              </div>
            </div>
            <div class="overflow-y-auto flex-1">
              <table class="w-full text-xs">
                <thead class="border-b border-gray-200 dark:border-gray-700">
                  <tr>
                    <th class="py-1.5 pr-4 text-left font-medium text-gray-500 dark:text-gray-400 whitespace-nowrap">Time</th>
                    <th class="py-1.5 pr-4 text-left font-medium text-gray-500 dark:text-gray-400">Level</th>
                    <th class="py-1.5 text-left font-medium text-gray-500 dark:text-gray-400">Message</th>
                  </tr>
                </thead>
                <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
                  <tr v-if="!eventLog.length">
                    <td colspan="3" class="py-4 text-center text-gray-400 dark:text-gray-500">No events.</td>
                  </tr>
                  <tr v-for="(e, i) in eventLog" :key="i" class="even:bg-gray-50 dark:even:bg-gray-800/50">
                    <td class="py-1.5 pr-4 text-gray-500 dark:text-gray-400 whitespace-nowrap font-mono">{{ formatEventTime(e.time) }}</td>
                    <td class="py-1.5 pr-4 whitespace-nowrap">
                      <span :class="eventLevelClass(e.level)" class="px-1.5 py-0.5 rounded text-xs font-medium uppercase">{{ e.level || '—' }}</span>
                    </td>
                    <td class="py-1.5 text-gray-900 dark:text-gray-100">{{ e.text }}</td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
        </div>
      </Transition>
    </Teleport>

    <!-- Error log modal -->
    <Teleport to="body">
      <Transition enter-active-class="ease-out duration-200" enter-from-class="opacity-0" enter-to-class="opacity-100"
                  leave-active-class="ease-in duration-150" leave-from-class="opacity-100" leave-to-class="opacity-0">
        <div v-if="showErrorLog" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
          <div class="w-full max-w-2xl bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6 max-h-[80vh] flex flex-col">
            <div class="flex items-center justify-between mb-4">
              <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100">Error Log</h3>
              <div class="flex items-center gap-3">
                <button class="px-3 py-1 text-xs font-medium text-gray-700 dark:text-gray-300 border border-gray-300 dark:border-gray-600 rounded-lg hover:bg-gray-50 dark:hover:bg-gray-800 transition-colors" @click="refreshErrorLog">Refresh</button>
                <button class="text-gray-400 hover:text-gray-600 dark:hover:text-gray-300 text-xl leading-none" @click="showErrorLog = false">&times;</button>
              </div>
            </div>
            <div class="overflow-y-auto flex-1">
              <table class="w-full text-xs">
                <thead class="border-b border-gray-200 dark:border-gray-700">
                  <tr>
                    <th class="py-1.5 text-left font-medium text-gray-500 dark:text-gray-400">Message</th>
                  </tr>
                </thead>
                <tbody class="divide-y divide-gray-100 dark:divide-gray-800">
                  <tr v-if="!errorLog.length">
                    <td class="py-4 text-center text-gray-400 dark:text-gray-500">No errors.</td>
                  </tr>
                  <tr v-for="(e, i) in errorLog" :key="i" class="even:bg-gray-50 dark:even:bg-gray-800/50">
                    <td class="py-1.5 text-gray-900 dark:text-gray-100">{{ e.text }}</td>
                  </tr>
                </tbody>
              </table>
            </div>
          </div>
        </div>
      </Transition>
    </Teleport>

    <!-- Deploy progress modal -->
    <Teleport to="body">
      <div v-if="deployProgress.show" class="fixed inset-0 z-50 flex items-center justify-center p-4 bg-black/50">
        <div class="w-full max-w-md bg-white dark:bg-gray-900 rounded-xl shadow-xl p-6">
          <h3 class="text-base font-semibold text-gray-900 dark:text-gray-100 mb-5">Deploying…</h3>
          <!-- Bar 1: overall controller progress (deployAll only) -->
          <div v-if="deployProgress.ctrlTotal > 1" class="mb-4">
            <div class="flex justify-between text-xs text-gray-500 dark:text-gray-400 mb-1">
              <span>{{ deployProgress.ctrlLabel }}</span>
              <span>{{ deployProgress.ctrlCurrent }} / {{ deployProgress.ctrlTotal }}</span>
            </div>
            <div class="w-full bg-gray-200 dark:bg-gray-700 rounded-full h-2">
              <div class="bg-amber-500 h-2 rounded-full transition-all duration-300" :style="{ width: `${deployProgress.ctrlTotal ? (deployProgress.ctrlCurrent / deployProgress.ctrlTotal * 100) : 0}%` }"></div>
            </div>
          </div>
          <!-- Bar 2: step progress -->
          <div class="mb-4">
            <div class="flex justify-between text-xs text-gray-500 dark:text-gray-400 mb-1">
              <span>{{ deployProgress.stepLabel }}</span>
              <span>{{ deployProgress.stepCurrent }} / {{ deployProgress.stepTotal }}</span>
            </div>
            <div class="w-full bg-gray-200 dark:bg-gray-700 rounded-full h-2">
              <div class="bg-blue-500 h-2 rounded-full transition-all duration-300" :style="{ width: `${deployProgress.stepTotal ? (deployProgress.stepCurrent / deployProgress.stepTotal * 100) : 0}%` }"></div>
            </div>
          </div>
          <!-- Bar 3: item progress -->
          <div v-if="deployProgress.itemTotal > 0">
            <div class="flex justify-between text-xs text-gray-500 dark:text-gray-400 mb-1">
              <span>{{ deployProgress.itemLabel }}</span>
              <span>{{ deployProgress.itemCurrent }} / {{ deployProgress.itemTotal }}</span>
            </div>
            <div class="w-full bg-gray-200 dark:bg-gray-700 rounded-full h-2">
              <div class="bg-green-500 h-2 rounded-full transition-all duration-300" :style="{ width: `${deployProgress.itemTotal ? (deployProgress.itemCurrent / deployProgress.itemTotal * 100) : 0}%` }"></div>
            </div>
          </div>
        </div>
      </div>
    </Teleport>

    <ConfirmModal :show="!!deleteTarget" title="Delete Controller" :message="`Delete controller '${deleteTarget?.name}'? All assignments will be lost.`"
      confirm-label="Delete" @confirm="doDelete" @cancel="deleteTarget = null" />

    <ConfirmModal :show="!!pullBackupTarget" title="Pull Backup" :message="`Pull backup from '${pullBackupTarget?.name}'? This will replace all local configuration data and cannot be undone.`"
      variant="warning" confirm-label="Pull Backup" @confirm="doPullBackup" @cancel="pullBackupTarget = null" />

    <ConfirmModal :show="!!cloudRestoreTarget" title="Restore Cloud Backup" :message="`Restore cloud backup to '${cloudRestoreTarget?.name}'? The device will retrieve and decrypt the backup from the cloud.`"
      variant="warning" confirm-label="Restore" @confirm="doCloudRestore" @cancel="cloudRestoreTarget = null" />

    <ConfirmModal :show="showLoadBackupPrompt" title="Load Backup Now?" message="Load the retrieved cloud backup into the app? This will replace all local configuration data and cannot be undone."
      variant="warning" confirm-label="Load Backup" @confirm="doLoadCloudBackup" @cancel="showLoadBackupPrompt = false" />

    <ConfirmModal :show="!!cloudDeleteTarget" title="Delete Cloud Backup" :message="`Delete the cloud backup for '${cloudDeleteTarget?.name}'? This is permanent and cannot be undone.`"
      variant="danger" confirm-label="Delete" @confirm="doCloudDelete" @cancel="cloudDeleteTarget = null" />

    <ConfirmModal :show="!!rebootTarget" title="Reboot Controller" :message="`Reboot '${rebootTarget?.name}'? The device will be unreachable for several seconds while it restarts.`"
      variant="danger" confirm-label="Reboot" @confirm="doReboot" @cancel="rebootTarget = null" />

    <ConfirmModal :show="!!otaTarget" title="Force OTA Update" :message="`Paste the firmware version payload for '${otaTarget?.name}'. Any combination of app and ui binaries is accepted. The controller reboots after the app partition is written.`"
      variant="warning" confirm-label="Force Update" textarea-placeholder='{"binaries":[{"partition":"app","url":"https://..."},{"partition":"ui","url":"https://..."}]}' @confirm="(payload) => doOta(payload)" @cancel="otaTarget = null" />
  </AppLayout>
</template>

<script setup>
import { ref, reactive, computed, onMounted, onUnmounted, watch } from 'vue'
import { sha256 } from '@noble/hashes/sha256'
import { importDB, exportDB } from 'dexie-export-import'
import AppLayout from '../components/AppLayout.vue'
import ConfirmModal from '../components/ConfirmModal.vue'
import { useControllers } from '../composables/useControllers'
import { useAreas } from '../composables/useAreas'
import { useControllerSession } from '../composables/useControllerSession'
import { buildControllerPayload, buildClientPayload, getExtendedClientIds } from '../composables/usePayloads'
import { useToast } from '../composables/useToast'
import { useAppState } from '../composables/useAppState'
import { randomUUID } from '../composables/useValidators'

const MAC_RE = /^[0-9A-Fa-f]{2}[:-]([0-9A-Fa-f]{2}[:-]){4}[0-9A-Fa-f]{2}$/
import { isCloudMode } from '../composables/useCloudMode'
import { db } from '../composables/useDatabase'

// Module-level ETag cache keyed by controller UUID — persists across navigation
const etagCache = reactive({})
// Version cache keyed by controller ID — cleared on logout
const versionCache = reactive({})

const { items, products, load, create, update, remove } = useControllers()
const { items: areas, load: loadAreas } = useAreas()
const { addToast } = useToast()
const { state } = useAppState()

const showModal = ref(false)
const showEventLog = ref(false)
const showErrorLog = ref(false)
const editing = ref(null)
const deleteTarget = ref(null)
const pullBackupTarget = ref(null)
const otaTarget = ref(null)
const cloudRestoreTarget = ref(null)
const cloudRestorePayload = ref(null)
const showLoadBackupPrompt = ref(false)
const cloudDeleteTarget = ref(null)
const rebootTarget = ref(null)
const eventLog = ref([])
const errorLog = ref([])
const activeEventLogId = ref(null)
const activeErrorLogId = ref(null)
const eventLogRefreshTimer = ref(null)
const errorLogRefreshTimer = ref(null)
const deployProgress = reactive({
  show: false,
  ctrlLabel: '',
  ctrlCurrent: 0,
  ctrlTotal: 1,
  stepLabel: '',
  stepCurrent: 0,
  stepTotal: 6,
  itemLabel: '',
  itemCurrent: 0,
  itemTotal: 0
})
const macError = ref('')
const emptyForm = () => ({ name: '', area: '', product: '', mac: '', uuid: '' })
const form = ref(emptyForm())
const localDexieHash = ref(null)

const ipInputs = reactive({})
const tokenInputs = reactive({})
const sessions = reactive({})

// Separate non-reactive store for session controllers
const sessionCtrls = {}

function initSession(id) {
  if (!sessionCtrls[id]) {
    const ctrl = useControllerSession(id)
    sessionCtrls[id] = ctrl
    sessions[id] = ctrl.session
    // Pre-populate IP input from stored session
    if (ctrl.session.ip) ipInputs[id] = ctrl.session.ip
  }
}

function getSessionCtrl(id) {
  initSession(id)
  return sessionCtrls[id]
}

async function computeLocalDexieHash() {
  try {
    const blob = await exportDB(db)
    const arrayBuf = await blob.arrayBuffer()
    const hashBytes = sha256(new Uint8Array(arrayBuf))
    localDexieHash.value = Array.from(hashBytes).map(b => b.toString(16).padStart(2, '0')).join('')
  } catch {
    localDexieHash.value = null
  }
}

function isDeployRequired(ctrl) {
  return (
    sessions[ctrl.id]?.isAuthenticated &&
    localDexieHash.value !== null &&
    etagCache[ctrl.uuid] !== undefined &&
    etagCache[ctrl.uuid] !== localDexieHash.value
  )
}

onMounted(async () => {
  await Promise.all([load(), loadAreas()])
  items.value.forEach(c => initSession(c.id))
  await computeLocalDexieHash()
  await Promise.all(
    items.value
      .filter(c => sessions[c.id]?.isAuthenticated && c.uuid && !(c.uuid in etagCache))
      .map(c => fetchBackupEtag(c))
  )
})

onUnmounted(() => {
  if (eventLogRefreshTimer.value) clearInterval(eventLogRefreshTimer.value)
  if (errorLogRefreshTimer.value) clearInterval(errorLogRefreshTimer.value)
})

watch(showEventLog, val => {
  if (!val && eventLogRefreshTimer.value) {
    clearInterval(eventLogRefreshTimer.value)
    eventLogRefreshTimer.value = null
  }
})

watch(showErrorLog, val => {
  if (!val && errorLogRefreshTimer.value) {
    clearInterval(errorLogRefreshTimer.value)
    errorLogRefreshTimer.value = null
  }
})

function areaName(id) { return areas.value.find(a => a.id === id)?.name ?? '—' }

function printLandscape() {
  const prev = document.title
  document.title = 'Controllers'
  const style = document.createElement('style')
  style.textContent = '@page { size: landscape; }'
  document.head.appendChild(style)
  window.addEventListener('afterprint', () => {
    document.title = prev
    document.head.removeChild(style)
  }, { once: true })
  window.print()
}

function openAdd() {
  editing.value = null
  form.value = emptyForm()
  macError.value = ''
  showModal.value = true
}

function openEdit(c) {
  editing.value = c
  form.value = { name: c.name, area: c.area, product: c.product, mac: c.mac ?? '', uuid: c.uuid }
  macError.value = ''
  showModal.value = true
}

function generateMac() {
  form.value.mac = 'ff:ff:ff:ff:ff:ff'
  macError.value = ''
}

async function save() {
  macError.value = ''
  const mac = form.value.mac.toLowerCase()
  if (!MAC_RE.test(mac)) {
    macError.value = 'Enter a valid MAC address (e.g. aa:bb:cc:dd:ee:ff)'
    return
  }
  try {
    if (editing.value) {
      await update(editing.value.id, { ...form.value, mac })
    } else {
      await create({ ...form.value, mac })
    }
    showModal.value = false
    await load()
    items.value.forEach(c => initSession(c.id))
  } catch (e) {
    addToast('error', `Failed to save: ${e.message}`)
  }
}

function confirmDelete(c) {
  const inputCount = Object.keys(c.inputs || {}).length
  const outputCount = Object.keys(c.outputs || {}).length
  if (inputCount > 0 || outputCount > 0) {
    addToast('warning', `Cannot delete '${c.name}': remove all input and output assignments first.`)
    return
  }
  deleteTarget.value = c
}

async function doDelete() {
  try {
    await remove(deleteTarget.value.id)
  } catch (e) {
    addToast('error', `Failed to delete: ${e.message}`)
  } finally {
    deleteTarget.value = null
  }
}

async function authenticate(id) {
  const ctrl = getSessionCtrl(id)
  ctrl.setIp(ipInputs[id] ?? '')
  ctrl.setVisualToken(tokenInputs[id] ?? '')
  try {
    await ctrl.authenticate()
    addToast('success', 'Connected.')
    const ctrlRecord = items.value.find(c => c.id === id)
    if (ctrlRecord) {
      await fetchBackupEtag(ctrlRecord)
      fetchVersions(ctrlRecord)
    }
    await fetchProvisioningState(id)
    await verifyDeviceUuid(id)
  } catch (e) {
    addToast('error', `Connection failed: ${e.message}`)
  }
}

async function verifyDeviceUuid(id) {
  const sessionCtrl = getSessionCtrl(id)
  const ctrlRecord = items.value.find(c => c.id === id)
  if (!ctrlRecord) return
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/version', {}, sessionCtrl.session.visualToken)
    if (res.ok) {
      const version = await res.json()
      if (version.uuid && version.uuid !== ctrlRecord.uuid) {
        addToast('warning', `Connected, but device UUID (${version.uuid}) does not match configured UUID (${ctrlRecord.uuid}). Verify the correct controller is connected.`)
      }
    }
  } catch { /* Non-fatal — UUID verification is best-effort */ }
}

async function fetchVersions(ctrl) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/version', {}, sessionCtrl.session.visualToken)
    const body = res.ok ? await res.json() : {}
    const app = body.application
    const ui = body.ui
    const appVersion = app ? `${app.version} (${app.commit})` : '—'
    const uiVersion = ui ? `${ui.version} (${ui.commit})` : '—'
    versionCache[ctrl.id] = { app: appVersion, ui: uiVersion }
  } catch {
    versionCache[ctrl.id] = { app: '—', ui: '—' }
  }
}

function logout(id) {
  const ctrlRecord = items.value.find(c => c.id === id)
  if (ctrlRecord?.uuid) delete etagCache[ctrlRecord.uuid]
  delete versionCache[id]
  getSessionCtrl(id).logout()
  tokenInputs[id] = ''
}

function handleUnauthorized(id) {
  const ctrl = items.value.find(c => c.id === id)
  addToast('error', `${ctrl?.name ?? id}: Authentication failed`)
  logout(id)
}

function handleFetchError(id) {
  const ctrl = items.value.find(c => c.id === id)
  addToast('error', `Controller ${ctrl?.name ?? id} is unavailable, check the HTTP server.`)
  logout(id)
}

async function fetchBackupEtag(ctrl) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  try {
    const abortCtrl = new AbortController()
    const timerId = setTimeout(() => abortCtrl.abort(), 5000)
    const res = await fetch(`http://${sessionCtrl.session.ip}/backup`, {
      method: 'HEAD',
      headers: { 'visual-token': sessionCtrl.session.visualToken },
      signal: abortCtrl.signal
    })
    clearTimeout(timerId)
    if (res.status === 200) {
      const etag = res.headers.get('ETag')
      etagCache[ctrl.uuid] = etag ? etag.replace(/"/g, '') : ''
    } else if (res.status === 404) {
      etagCache[ctrl.uuid] = ''
    }
    // 405 or other errors: leave cache entry untouched (show no indicator)
  } catch {
    // Network error: leave cache entry untouched
  }
}

async function deploy(ctrl) {
  deployProgress.show = true
  deployProgress.ctrlLabel = ctrl.name
  deployProgress.ctrlTotal = 1
  deployProgress.ctrlCurrent = 0
  await _doDeployController(ctrl, 0, 1)
  deployProgress.show = false
}

async function _doDeployController(ctrl, ctrlIndex, ctrlTotal) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')

  const allCerts = await db.certificates.toArray()
  const certsToSync = allCerts.filter(c => c.isController || c.isClient)
  const hasCerts = certsToSync.length > 0
  const STEPS = hasCerts ? 8 : 7
  let step = 0

  function setStep(label) {
    deployProgress.stepLabel = label
    deployProgress.stepCurrent = step
    deployProgress.stepTotal = STEPS
    deployProgress.itemLabel = ''
    deployProgress.itemCurrent = 0
    deployProgress.itemTotal = 0
    step++
  }

  function setItem(label, current, total) {
    deployProgress.itemLabel = label
    deployProgress.itemCurrent = current
    deployProgress.itemTotal = total
  }

  const ip = sessionCtrl.session.ip
  const token = sessionCtrl.session.visualToken

  // For /api/* endpoints (controllers, clients, etc.)
  async function fetchOrHandle(path, options = {}) {
    try {
      const res = await controllerFetch(ip, path, options, token)
      if (res.status === 401) { handleUnauthorized(ctrl.id); return null }
      return res
    } catch {
      handleFetchError(ctrl.id)
      return null
    }
  }

  // For non-/api endpoints (/backup, /certs) that need full URL control
  async function fetchDirectOrHandle(path, options = {}, timeoutMs = 5000) {
    const abortCtrl = new AbortController()
    const timerId = setTimeout(() => abortCtrl.abort(), timeoutMs)
    try {
      const res = await fetch(`http://${ip}${path}`, {
        ...options,
        headers: { 'visual-token': token, ...options.headers },
        signal: abortCtrl.signal
      })
      clearTimeout(timerId)
      if (res.status === 401) { handleUnauthorized(ctrl.id); return null }
      return res
    } catch {
      clearTimeout(timerId)
      handleFetchError(ctrl.id)
      return null
    }
  }

  let success = false
  try {
    // Step 1 (conditional): Push certificates
    if (hasCerts) {
      setStep('Pushing certificates…')
      const listRes = await fetchDirectOrHandle('/certs', {}, 10000)
      if (!listRes) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
      if (!listRes.ok) {
        addToast('error', `${ctrl.name}: could not read cert list (HTTP ${listRes.status})`)
        deployProgress.ctrlCurrent = ctrlIndex + 1
        return false
      }
      const existing = await listRes.json()
      const existingNames = new Set(Array.isArray(existing) ? existing.map(c => c.file ?? c.filename ?? c.name ?? c) : [])
      setItem('', 0, certsToSync.length)
      for (let i = 0; i < certsToSync.length; i++) {
        const cert = certsToSync[i]
        if (existingNames.has(cert.fileName)) {
          setItem(cert.fileName, i + 1, certsToSync.length)
          continue
        }
        const certType = cert.isController && cert.isClient ? 'both' : cert.isController ? 'controller' : 'client'
        const formData = new FormData()
        formData.append('file', new Blob([cert.certificate], { type: 'application/x-x509-ca-cert' }), cert.fileName)
        const uploadRes = await fetchDirectOrHandle('/certs', {
          method: 'POST',
          headers: { 'X-Cert-Type': certType },
          body: formData
        }, 10000)
        if (!uploadRes) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
        if (uploadRes.status !== 201 && uploadRes.status !== 403) {
          addToast('error', `${ctrl.name}: failed to upload cert '${cert.fileName}' (HTTP ${uploadRes.status})`)
          deployProgress.ctrlCurrent = ctrlIndex + 1
          return false
        }
        setItem(cert.fileName, i + 1, certsToSync.length)
      }
    }

    // Step 2: Read controllers
    setStep('Reading controllers…')
    const ctrlsRes = await fetchOrHandle('/controllers')
    if (!ctrlsRes) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
    const existingCtrls = ctrlsRes.ok ? await ctrlsRes.json() : []

    // Step 3: Delete controllers
    setStep('Deleting controllers…')
    setItem('', 0, existingCtrls.length)
    for (let i = 0; i < existingCtrls.length; i++) {
      const c = existingCtrls[i]
      setItem(c ?? '', i, existingCtrls.length)
      const r = await fetchOrHandle(`/controllers/${c}`, { method: 'DELETE' })
      if (!r) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
      setItem(c ?? '', i + 1, existingCtrls.length)
    }

    // Step 4: Deploy all controllers
    setStep('Deploying controllers…')
    const allControllers = await db.controllers.toArray()
    setItem('', 0, allControllers.length)
    for (let i = 0; i < allControllers.length; i++) {
      const c = allControllers[i]
      setItem(c.name, i, allControllers.length)
      const payload = await buildControllerPayload(c.id)
      const putRes = await fetchOrHandle(`/controllers/${c.uuid}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(payload)
      })
      if (!putRes) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
      if (!putRes.ok && putRes.status !== 204) {
        addToast('error', `${ctrl.name}: failed to deploy controller '${c.name}' (HTTP ${putRes.status})`)
        deployProgress.ctrlCurrent = ctrlIndex + 1
        return false
      }
      setItem(c.name, i + 1, allControllers.length)
    }

    // Step 5: Read clients
    setStep('Reading clients…')
    const clientsRes = await fetchOrHandle('/clients')
    if (!clientsRes) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
    const existingClients = clientsRes.ok ? await clientsRes.json() : []

    // Step 6: Delete clients
    setStep('Deleting clients…')
    setItem('', 0, existingClients.length)
    for (let i = 0; i < existingClients.length; i++) {
      const c = existingClients[i]
      setItem(c ?? '', i, existingClients.length)
      const r = await fetchOrHandle(`/clients/${c}`, { method: 'DELETE' })
      if (!r) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
      setItem(c ?? '', i + 1, existingClients.length)
    }

    // Step 7: Deploy clients
    setStep('Deploying clients…')
    const secondaryIds = new Set(await getExtendedClientIds())
    const allClients = await db.clients.toArray()
    const primaryClients = allClients.filter(c => !secondaryIds.has(c.id))
    setItem('', 0, primaryClients.length)
    for (let i = 0; i < primaryClients.length; i++) {
      const client = primaryClients[i]
      setItem(client.name, i, primaryClients.length)
      const clientPayload = await buildClientPayload(client.id)
      const clientRes = await fetchOrHandle(`/clients/${client.uuid}`, {
        method: 'PUT',
        headers: { 'Content-Type': 'application/json' },
        body: JSON.stringify(clientPayload)
      })
      if (!clientRes) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
      if (!clientRes.ok && clientRes.status !== 204) {
        addToast('error', `${ctrl.name}: failed to push client '${client.name}' (HTTP ${clientRes.status})`)
        deployProgress.ctrlCurrent = ctrlIndex + 1
        return false
      }
      setItem(client.name, i + 1, primaryClients.length)
    }

    // Step 8: Push backup
    setStep('Pushing backup…')
    const backupBlob = await exportDB(db)
    const backupRes = await fetchDirectOrHandle('/backup', {
      method: 'PUT',
      headers: { 'Content-Type': 'application/json' },
      body: backupBlob
    }, 30000)
    if (!backupRes) { deployProgress.ctrlCurrent = ctrlIndex + 1; return false }
    if (!backupRes.ok && backupRes.status !== 204) {
      addToast('error', `${ctrl.name}: failed to push backup (HTTP ${backupRes.status})`)
      deployProgress.ctrlCurrent = ctrlIndex + 1
      return false
    }

    deployProgress.stepCurrent = STEPS
    deployProgress.ctrlCurrent = ctrlIndex + 1
    addToast('success', `Deployed to ${ctrl.name}.`)
    success = true

    // Verify deployment by comparing ETag to local hash
    await fetchBackupEtag(ctrl)

  } catch (e) {
    addToast('error', `${ctrl.name}: deploy error: ${e.message}`)
    deployProgress.ctrlCurrent = ctrlIndex + 1
  }

  return success
}

async function deployAll() {
  const connected = items.value.filter(c => sessions[c.id]?.isAuthenticated)
  if (connected.length === 0) {
    addToast('warning', 'No controllers are connected.')
    return
  }
  deployProgress.show = true
  deployProgress.ctrlTotal = connected.length
  deployProgress.ctrlCurrent = 0
  const results = []
  for (let i = 0; i < connected.length; i++) {
    deployProgress.ctrlLabel = connected[i].name
    results.push(await _doDeployController(connected[i], i, connected.length))
  }
  deployProgress.show = false
  if (results.some(r => r === false)) {
    addToast('error', 'One or more controllers failed to deploy.')
  }
}

const hasConnectedControllers = computed(() => items.value.some(c => sessions[c.id]?.isAuthenticated))

async function fetchEventLog(id) {
  const sessionCtrl = getSessionCtrl(id)
  if (!sessionCtrl.session.isAuthenticated) return
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/events', {}, sessionCtrl.session.visualToken)
    if (res.status === 401) { handleUnauthorized(id); showEventLog.value = false; return }
    eventLog.value = res.ok ? await res.json() : []
  } catch {
    handleFetchError(id)
    showEventLog.value = false
  }
}

async function openEventLog(id) {
  if (eventLogRefreshTimer.value) {
    clearInterval(eventLogRefreshTimer.value)
    eventLogRefreshTimer.value = null
  }
  activeEventLogId.value = id
  eventLog.value = []
  showEventLog.value = true
  await fetchEventLog(id)
  eventLogRefreshTimer.value = setInterval(() => fetchEventLog(activeEventLogId.value), 5000)
}

async function refreshEventLog() {
  if (activeEventLogId.value) await fetchEventLog(activeEventLogId.value)
}

function formatEventTime(t) {
  if (!t) return '—'
  return new Date(t * 1000).toLocaleString()
}

function eventLevelClass(level) {
  if (level === 'error') return 'bg-red-100 text-red-800 dark:bg-red-900/30 dark:text-red-200'
  if (level === 'notify') return 'bg-orange-100 text-orange-800 dark:bg-orange-900/30 dark:text-orange-200'
  return 'bg-blue-100 text-blue-800 dark:bg-blue-900/30 dark:text-blue-200'
}

// --- Error Log ---

async function openErrorLog(id) {
  activeErrorLogId.value = id
  await fetchErrorLog(id)
  showErrorLog.value = true
  errorLogRefreshTimer.value = setInterval(() => fetchErrorLog(activeErrorLogId.value), 5000)
}

async function fetchErrorLog(id) {
  const sessionCtrl = getSessionCtrl(id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/errors', {}, sessionCtrl.session.visualToken)
    if (res.status === 401) { handleUnauthorized(id); showErrorLog.value = false; return }
    errorLog.value = res.ok ? await res.json() : []
  } catch {
    handleFetchError(id)
    showErrorLog.value = false
  }
}

async function refreshErrorLog() {
  if (activeErrorLogId.value) {
    await fetchErrorLog(activeErrorLogId.value)
  }
}

// --- Pull Backup ---

function confirmPullBackup(ctrl) {
  pullBackupTarget.value = ctrl
}

async function doPullBackup() {
  const ctrl = pullBackupTarget.value
  pullBackupTarget.value = null
  if (!ctrl) return

  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { session } = sessionCtrl
  try {
    const TIMEOUT = 5000
    const abortCtrl = new AbortController()
    const timerId = setTimeout(() => abortCtrl.abort(), TIMEOUT)
    const res = await fetch(`http://${session.ip}/backup`, {
      headers: { 'visual-token': session.visualToken },
      signal: abortCtrl.signal
    })
    clearTimeout(timerId)

    if (!res.ok) {
      addToast('error', `Pull backup failed: HTTP ${res.status}`)
      return
    }

    const data = await res.json()

    if (data.formatName !== 'dexie') {
      addToast('error', 'Pull backup failed: response is not a valid Dexie export.')
      return
    }
    if (data.data?.databaseName !== 'FireFly-Controller') {
      addToast('error', 'Pull backup failed: backup was not exported from FireFly Controller.')
      return
    }

    const blob = new Blob([JSON.stringify(data)], { type: 'application/json' })
    const file = new File([blob], 'backup.json', { type: 'application/json' })

    await db.delete()
    await importDB(file)
    addToast('success', `Backup pulled from ${ctrl.name}. Reloading…`)
    setTimeout(() => window.location.reload(), 1500)
  } catch (e) {
    addToast('error', `Pull backup error: ${e.message}`)
  }
}

// --- Push Certificates ---

async function pushCertificates(ctrl) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { session } = sessionCtrl
  const TIMEOUT = 10000

  try {
    const allCerts = await db.certificates.toArray()
    const certsToSync = allCerts.filter(c => c.isController || c.isClient)
    if (certsToSync.length === 0) {
      addToast('warning', 'No certificates with a type assigned to push.')
      return
    }

    // Fetch the current list of certs on the controller
    let abortCtrl = new AbortController()
    let timerId = setTimeout(() => abortCtrl.abort(), TIMEOUT)
    const listRes = await fetch(`http://${session.ip}/certs`, {
      headers: { 'visual-token': session.visualToken },
      signal: abortCtrl.signal
    })
    clearTimeout(timerId)

    if (!listRes.ok) {
      addToast('error', `Push certs failed: could not read controller cert list (HTTP ${listRes.status})`)
      return
    }

    const existing = await listRes.json()
    const existingNames = new Set(Array.isArray(existing) ? existing.map(c => c.file ?? c.filename ?? c.name ?? c) : [])

    let pushed = 0
    let skipped = 0

    for (const cert of certsToSync) {
      if (existingNames.has(cert.fileName)) {
        skipped++
        continue
      }

      const certType = cert.isController && cert.isClient ? 'both'
        : cert.isController ? 'controller'
        : 'client'

      const formData = new FormData()
      const blob = new Blob([cert.certificate], { type: 'application/x-x509-ca-cert' })
      formData.append('file', blob, cert.fileName)

      abortCtrl = new AbortController()
      timerId = setTimeout(() => abortCtrl.abort(), TIMEOUT)
      const uploadRes = await fetch(`http://${session.ip}/certs`, {
        method: 'POST',
        headers: { 'visual-token': session.visualToken, 'X-Cert-Type': certType },
        body: formData,
        signal: abortCtrl.signal
      })
      clearTimeout(timerId)

      if (uploadRes.status === 201 || uploadRes.status === 403) {
        // 403 means already exists — treat as success
        pushed++
      } else {
        addToast('error', `Push certs: failed to upload '${cert.fileName}' (HTTP ${uploadRes.status})`)
        return
      }
    }

    addToast('success', `Certificates pushed to ${ctrl.name}: ${pushed} uploaded, ${skipped} already present.`)
  } catch (e) {
    addToast('error', `Push certs error: ${e.message}`)
  }
}

// --- Provisioning Mode ---

async function fetchProvisioningState(id) {
  const sessionCtrl = getSessionCtrl(id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/provisioning', {}, sessionCtrl.session.visualToken)
    if (res.ok) {
      const data = await res.json()
      sessionCtrl.session.provisioningModeEnabled = !!data.enabled
      sessionCtrl.save()
    }
  } catch {
    // Non-fatal — leave current state unchanged
  }
}

async function toggleProvisioning(id) {
  const sessionCtrl = getSessionCtrl(id)
  const { controllerFetch } = await import('../composables/useApi')
  const currentlyEnabled = sessionCtrl.session.provisioningModeEnabled
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/provisioning', {
      method: currentlyEnabled ? 'DELETE' : 'PUT'
    }, sessionCtrl.session.visualToken)
    if (res.status === 202) {
      sessionCtrl.session.provisioningModeEnabled = !currentlyEnabled
      sessionCtrl.save()
      addToast('success', `Provisioning mode ${!currentlyEnabled ? 'enabled' : 'disabled'}.`)
    } else {
      addToast('error', `Provisioning toggle failed: HTTP ${res.status}`)
    }
  } catch (e) {
    addToast('error', `Provisioning error: ${e.message}`)
  }
}

// --- OTA ---

function confirmOta(ctrl) {
  otaTarget.value = ctrl
}

async function doOta(payloadText) {
  const ctrl = otaTarget.value
  otaTarget.value = null
  if (!ctrl) return

  let payload
  try {
    payload = JSON.parse(payloadText)
  } catch {
    addToast('error', 'OTA failed: invalid JSON payload')
    return
  }

  if (Array.isArray(payload)) {
    if (payload.length === 0) {
      addToast('error', 'OTA failed: payload array is empty')
      return
    }
    payload = payload[0]
  }

  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')

  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/ota', {
      method: 'POST',
      headers: { 'Content-Type': 'application/json' },
      body: JSON.stringify(payload)
    }, sessionCtrl.session.visualToken)

    if (res.status === 202) {
      addToast('success', `OTA update initiated on ${ctrl.name}.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `OTA failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `OTA error: ${e.message}`)
  }
}

// --- Cloud Backup ---

async function pushCloudBackup(ctrl) {
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/cloud-backup', {
      method: 'POST'
    }, sessionCtrl.session.visualToken)
    if (res.ok) {
      let statusCode = ''
      try { const body = await res.json(); statusCode = ` (cloud: ${body.status})` } catch { /* ignore */ }
      addToast('success', `Cloud backup pushed for ${ctrl.name}.${statusCode}`)
    } else if (res.status === 404) {
      addToast('warning', `No local backup found on ${ctrl.name}.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `Push cloud backup failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `Push cloud backup error: ${e.message}`)
  }
}

function confirmCloudRestore(ctrl) {
  cloudRestoreTarget.value = ctrl
}

async function doCloudRestore() {
  const ctrl = cloudRestoreTarget.value
  cloudRestoreTarget.value = null
  if (!ctrl) return
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/cloud-backup', {
      method: 'GET'
    }, sessionCtrl.session.visualToken)
    if (res.ok) {
      const payload = await res.text()
      cloudRestorePayload.value = payload
      showLoadBackupPrompt.value = true
    } else if (res.status === 404) {
      addToast('warning', `No cloud backup found for ${ctrl.name}.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `Restore cloud backup failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `Restore cloud backup error: ${e.message}`)
  }
}

async function doLoadCloudBackup() {
  showLoadBackupPrompt.value = false
  const payload = cloudRestorePayload.value
  cloudRestorePayload.value = null
  if (!payload) return
  try {
    let parsed
    try { parsed = JSON.parse(payload) } catch {
      addToast('error', 'Import failed: retrieved backup is not valid JSON.')
      return
    }
    if (parsed.formatName !== 'dexie') {
      addToast('error', 'Import failed: retrieved backup is not a valid Dexie export.')
      return
    }
    await db.delete()
    const blob = new Blob([payload], { type: 'application/json' })
    await importDB(blob)
    addToast('success', 'Cloud backup loaded! Reloading…')
    setTimeout(() => window.location.reload(), 1500)
  } catch (e) {
    addToast('error', `Import failed: ${e.message}`)
  }
}

function confirmCloudDelete(ctrl) {
  cloudDeleteTarget.value = ctrl
}

async function doCloudDelete() {
  const ctrl = cloudDeleteTarget.value
  cloudDeleteTarget.value = null
  if (!ctrl) return
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/cloud-backup', {
      method: 'DELETE'
    }, sessionCtrl.session.visualToken)
    if (res.status === 204) {
      addToast('success', `Cloud backup deleted for ${ctrl.name}.`)
    } else if (res.status === 404) {
      addToast('warning', `No cloud backup found for ${ctrl.name}.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `Delete cloud backup failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `Delete cloud backup error: ${e.message}`)
  }
}

function confirmReboot(ctrl) {
  rebootTarget.value = ctrl
}

async function doReboot() {
  const ctrl = rebootTarget.value
  rebootTarget.value = null
  if (!ctrl) return
  const sessionCtrl = getSessionCtrl(ctrl.id)
  const { controllerFetch } = await import('../composables/useApi')
  try {
    const res = await controllerFetch(sessionCtrl.session.ip, '/reboot', {
      method: 'POST'
    }, sessionCtrl.session.visualToken)
    if (res.status === 204) {
      addToast('success', `${ctrl.name} is rebooting.`)
    } else {
      let msg = `HTTP ${res.status}`
      try { const body = await res.json(); msg = body.message ?? msg } catch { /* ignore */ }
      addToast('error', `Reboot failed: ${msg}`)
    }
  } catch (e) {
    addToast('error', `Reboot error: ${e.message}`)
  }
}

</script>
