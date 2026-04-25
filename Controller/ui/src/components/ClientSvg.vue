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
        :x="12"
        :y="btnTop(i)"
        width="56"
        :height="BTN_H"
        rx="2" ry="2"
        fill="#f9fafb"
        stroke="#d1d5db"
        stroke-width="0.5"
      />

      <!-- Switch: rocker dividing line -->
      <line
        v-if="hid.type === 'switch'"
        :x1="12"
        :y1="btnTop(i) + BTN_H / 2"
        :x2="68"
        :y2="btnTop(i) + BTN_H / 2"
        stroke="#9ca3af"
        stroke-width="1"
      />

      <!-- Button: LED indicator -->
      <circle
        v-else
        cx="40"
        :cy="ledY(i)"
        r="5"
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

const BTN_H = 42
const BTN_GAP = 4
const PLATE_PAD = 21

const svgHeight = computed(() => {
  const n = Math.max(1, props.hids.length)
  return PLATE_PAD * 2 + n * BTN_H + (n - 1) * BTN_GAP
})

function btnTop(i) {
  return PLATE_PAD + i * (BTN_H + BTN_GAP)
}

function ledY(i) {
  const top = btnTop(i)
  const inverted = props.hids.length === 5 && i === 4
  return inverted ? top + 9 : top + BTN_H - 9
}

function ledColor(hid) {
  if (!hid.color) return '#9ca3af'
  return props.colors.find(c => c.id === hid.color)?.hex ?? '#9ca3af'
}
</script>
