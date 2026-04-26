<template>
  <svg
    xmlns="http://www.w3.org/2000/svg"
    :viewBox="`0 0 80 ${svgHeight}`"
    class="w-full drop-shadow"
    style="max-width:80px"
  >
    <!-- Outer plate / wall surround -->
    <rect x="1" y="1" width="78" :height="svgHeight - 2" rx="6" ry="6"
          fill="#52525b" stroke="#27272a" stroke-width="1"/>
    <!-- Screw holes -->
    <circle cx="40" cy="9" r="3.5" fill="#27272a"/>
    <circle cx="40" :cy="svgHeight - 9" r="3.5" fill="#27272a"/>
    <!-- Inner face plate -->
    <rect x="8" y="19" width="64" :height="svgHeight - 38" rx="3" ry="3"
          fill="#e4e4e7"/>

    <g v-for="(hid, i) in hids" :key="i" :opacity="hid.enabled === false ? 0.35 : 1">
      <!-- Button / rocker face -->
      <rect
        :x="btnX(i)"
        :y="btnTop(i)"
        :width="btnW"
        :height="BTN_H"
        rx="2" ry="2"
        fill="#f9fafb"
        stroke="#d1d5db"
        stroke-width="0.5"
      />

      <!-- Switch: rocker dividing line -->
      <line
        v-if="hid.type === 'switch'"
        :x1="btnX(i)"
        :y1="btnTop(i) + BTN_H / 2"
        :x2="btnX(i) + btnW"
        :y2="btnTop(i) + BTN_H / 2"
        stroke="#9ca3af"
        stroke-width="1"
      />

      <!-- Button: LED indicator -->
      <circle
        v-else
        :cx="btnX(i) + btnW / 2"
        :cy="ledY(i)"
        r="4"
        :fill="ledColor(hid)"
        stroke="#00000018"
        stroke-width="0.5"
      />
    </g>
  </svg>
</template>

<script setup>
import { computed } from 'vue'

const props = defineProps({
  hids: { type: Array, default: () => [] },
  colors: { type: Array, default: () => [] }
})

const BTN_H = 36
const BTN_GAP = 3
const BTN_GAP_H = 3
const PLATE_PAD = 21
const FACE_X = 12
const FACE_W = 56

const cols = computed(() => {
  const n = props.hids.length
  if (n >= 5) return 3
  if (n === 4) return 2
  return 1
})

const rows = computed(() => Math.ceil(props.hids.length / cols.value))

const btnW = computed(() => (FACE_W - (cols.value - 1) * BTN_GAP_H) / cols.value)

const svgHeight = computed(() => {
  const r = Math.max(1, rows.value)
  return PLATE_PAD * 2 + r * BTN_H + (r - 1) * BTN_GAP
})

function btnCol(i) { return i % cols.value }
function btnRow(i) { return Math.floor(i / cols.value) }

function btnX(i) {
  return FACE_X + btnCol(i) * (btnW.value + BTN_GAP_H)
}

function btnTop(i) {
  return PLATE_PAD + btnRow(i) * (BTN_H + BTN_GAP)
}

function ledY(i) {
  const top = btnTop(i)
  const inverted = props.hids.length === 5 && i === 4
  return inverted ? top + 8 : top + BTN_H - 8
}

function ledColor(hid) {
  if (!hid.color) return '#9ca3af'
  return props.colors.find(c => c.id === hid.color)?.hex ?? '#9ca3af'
}
</script>
