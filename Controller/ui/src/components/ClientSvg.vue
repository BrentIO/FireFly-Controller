<template>
  <svg
    xmlns="http://www.w3.org/2000/svg"
    viewBox="0 0 400 600"
    class="w-full drop-shadow"
    style="max-width:160px"
  >
    <defs>
      <radialGradient id="csvBtnGrad">
        <stop offset="10%" stop-color="#C0C0C0" />
        <stop offset="95%" stop-color="#a6a6a6" />
      </radialGradient>
      <linearGradient id="csvSwGrad" gradientTransform="rotate(90)">
        <stop offset="0%"   stop-color="#C0C0C0" />
        <stop offset="50%"  stop-color="#C0C0C0" />
        <stop offset="100%" stop-color="#a6a6a6" />
      </linearGradient>
    </defs>

    <!-- Plate -->
    <rect x="5" y="5" width="390" height="590" rx="30" ry="30"
          fill="#e8e8e8" stroke="#d1d5db" stroke-width="2" />
    <!-- Screw holes -->
    <circle cx="200" cy="30"  r="12" fill="#27272a" />
    <circle cx="200" cy="570" r="12" fill="#27272a" />

    <g v-for="(hid, i) in hids" :key="i" :opacity="hid.enabled === false ? 0.35 : 1">
      <!-- Switch -->
      <template v-if="hid.type === 'switch' || hid.type === 'SWITCH'">
        <rect :x="loc(i)[0] - 50" :y="loc(i)[1] - 25" width="100" height="50" fill="#737373" />
        <rect :x="loc(i)[0] - 50" :y="loc(i)[1] - 25" width="30"  height="50" fill="url(#csvSwGrad)" />
      </template>
      <!-- Button -->
      <template v-else>
        <circle :cx="loc(i)[0]" :cy="loc(i)[1]" r="50" fill="url(#csvBtnGrad)" />
        <circle :cx="loc(i)[0]" :cy="loc(i)[1]" r="33"
                :stroke="ledColor(hid)" stroke-width="10" fill="url(#csvBtnGrad)" />
      </template>
    </g>
  </svg>
</template>

<script setup>
const props = defineProps({
  hids:     { type: Array,   default: () => [] },
  colors:   { type: Array,   default: () => [] },
  inverted: { type: Boolean, default: false }
})

const LOCATIONS = {
  1:   [[200,300]],
  2:   [[200,200],[200,400]],
  3:   [[200,150],[200,300],[200,450]],
  4:   [[125,200],[275,200],[125,400],[275,400]],
  5:   [[125,150],[275,150],[125,300],[275,300],[200,450]],
  6:   [[125,150],[275,150],[125,300],[275,300],[125,450],[275,450]],
  '5i':[[200,150],[125,300],[275,300],[125,450],[275,450]]
}

function loc(i) {
  const n = props.hids.length
  const key = n === 5 && props.inverted ? '5i' : n
  return (LOCATIONS[key] ?? [[200,300]])[i] ?? [200,300]
}

function ledColor(hid) {
  if (!hid.color) return '#9ca3af'
  return props.colors.find(c => c.id === hid.color)?.hex ?? '#9ca3af'
}
</script>
