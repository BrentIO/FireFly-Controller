import { db } from './useDatabase'

const MAX_NAME = 20
const MAX_AREA = 20
const MAX_ID = 8
const MAX_ICON = 64
const MAX_COLOR = 20
const MAX_TAG = 20
const MAX_URL = 128
const MAX_CERT = 31
const MAX_HID_PER_INPUT_PORT = 4

async function getExtendedClientIds() {
  const all = await db.clients.toArray()
  return all.filter(c => c.extends !== undefined).map(c => c.extends)
}

async function getOTAConfig(deviceType) {
  const ota = await db.settings.where({ setting: `ota_${deviceType}` }).first()
  if (!ota || ota.value.enabled === false) return {}

  const result = {}
  if (ota.value.protocol === 'https' && ota.value.certificate) {
    const cert = await db.certificates.get(ota.value.certificate)
    if (!cert) throw new Error(`Unknown certificate ID ${ota.value.certificate} in OTA configuration.`)
    result.certificate = cert.fileName.substring(0, MAX_CERT)
  }
  result.url = `${ota.value.protocol}://${ota.value.url}`.substring(0, MAX_URL)
  return result
}

export async function buildControllerPayload(controllerId, returnOnlyErrors = false) {
  const errorList = []
  const extendedClientIds = await getExtendedClientIds()

  const [controller] = await db.controllers.where('id').equals(controllerId).toArray()
  if (!controller) throw new Error('Controller not found')

  const area = await db.areas.get(controller.area)

  const resolvedInputs = {}
  for (const [port, clientId] of Object.entries(controller.inputs)) {
    resolvedInputs[port] = await db.clients.get(clientId)
  }

  const resolvedOutputs = {}
  for (const [port, circuitId] of Object.entries(controller.outputs)) {
    const circuit = await db.circuits.get(circuitId)
    circuit.area = await db.areas.get(circuit.area)
    circuit.relay_model = await db.relay_models.get(circuit.relay_model)
    circuit.icon = await db.circuit_icons.get(circuit.icon)
    resolvedOutputs[port] = circuit
  }

  const ports = {}
  for (const [portNumber, client] of Object.entries(resolvedInputs)) {
    const channels = {}

    for (let i = 0; i < client.hids.length; i++) {
      const hid = client.hids[i]
      const channel = {}

      if (extendedClientIds.includes(client.id)) {
        channel.offset = MAX_HID_PER_INPUT_PORT
      }
      if (hid.switch_type !== 'NORMALLY_OPEN') {
        channel.type = hid.switch_type
      }
      if (hid.enabled !== true) {
        channel.enabled = false
      }

      for (const action of hid.actions) {
        const matchedPort = Object.entries(resolvedOutputs).find(
          ([, circuit]) => circuit.id === action.circuit
        )

        if (!matchedPort) {
          const requestedCircuit = await db.circuits.get(action.circuit)
          const circuitArea = await db.areas.get(requestedCircuit?.area)
          errorList.push(
            `Client '${client.id}' channel ${i + 1} has an invalid action for circuit "${circuitArea?.name} ${requestedCircuit?.description} (${requestedCircuit?.name}).\n\nThe client is assigned to this controller, but the circuit is not.\n\nThe action will be ignored.`
          )
          continue
        }

        if (!channel.actions) channel.actions = []
        const act = { ...action }
        if (act.change_state === 'SHORT') delete act.change_state
        act.output = parseInt(matchedPort[0])
        const permitted = ['change_state', 'action', 'output']
        for (const key of Object.keys(act)) {
          if (!permitted.includes(key)) delete act[key]
        }
        channel.actions.push(act)
      }

      channels[i + 1] = channel
    }

    ports[portNumber] = {
      id: client.name.trim().substring(0, MAX_ID),
      name: client.description.trim().substring(0, MAX_NAME),
      channels
    }
  }

  const outputs = {}
  for (const [portNumber, circuit] of Object.entries(resolvedOutputs)) {
    const out = {
      id: circuit.name.trim().substring(0, MAX_ID),
      name: circuit.description.trim().substring(0, MAX_NAME),
      area: circuit.area.name.trim().substring(0, MAX_AREA),
      icon: circuit.icon.icon.trim().substring(0, MAX_ICON)
    }
    if (circuit.relay_model.type !== 'BINARY') {
      out.type = circuit.relay_model.type
    }
    if (circuit.enabled !== true) {
      out.enabled = circuit.enabled
    }
    outputs[portNumber] = out
  }

  const mqtt = await db.settings.where({ setting: 'mqtt' }).first()
  if (!mqtt) {
    if (returnOnlyErrors) { errorList.push('MQTT has not been configured.'); return errorList }
    throw new Error('MQTT has not been configured.')
  }

  const mqttPayload = {}
  for (const field of ['host', 'port', 'username', 'password']) {
    if (mqtt.value[field] !== undefined) mqttPayload[field] = mqtt.value[field]
  }

  const payload = {
    name: controller.name.trim().substring(0, MAX_NAME),
    area: area.name.trim().substring(0, MAX_AREA),
    ports,
    outputs,
    mqtt: mqttPayload
  }

  const ota = await getOTAConfig('controller')
  if (Object.keys(ota).length > 0) payload.ota = ota

  if (returnOnlyErrors) return errorList
  return payload
}

export async function buildClientPayload(clientId) {
  let client = await db.clients.get(clientId)
  if (!client) throw new Error('Client not found')

  if (client.extends !== undefined) {
    const parent = await db.clients.get(client.extends)
    client = { ...client, hids: [...client.hids, ...(parent?.hids ?? [])] }
  }

  const area = await db.areas.get(client.area)
  const hids = {}

  for (let i = 0; i < client.hids.length; i++) {
    const hid = client.hids[i]
    hids[i + 1] = {}

    if (hid.color !== undefined) {
      const color = await db.colors.get(hid.color)
      if (color) hids[i + 1].color = color.name.trim().substring(0, MAX_COLOR)
    }

    if (hid.tags?.length > 0) {
      hids[i + 1].tags = []
      for (const tagId of hid.tags) {
        const tag = await db.tags.get(tagId)
        if (tag) hids[i + 1].tags.push(tag.name.trim().substring(0, MAX_TAG))
      }
    }
  }

  const wifi = await db.settings.where({ setting: 'wifi' }).first()
  if (!wifi) throw new Error('WiFi has not been configured.')

  const mqtt = await db.settings.where({ setting: 'mqtt' }).first()
  if (!mqtt) throw new Error('MQTT has not been configured.')

  const payload = {
    id: client.name.trim().substring(0, MAX_ID),
    name: client.description?.trim().substring(0, MAX_NAME) ?? '',
    area: area.name.trim().substring(0, MAX_AREA),
    mac: client.mac,
    hids,
    wifi: { ssid: wifi.value.ssid, password: wifi.value.password },
    mqtt: { host: mqtt.value.host, username: mqtt.value.username, password: mqtt.value.password }
  }

  const ota = await getOTAConfig('client')
  if (Object.keys(ota).length > 0) payload.ota = ota

  return payload
}

export async function checkConfiguration() {
  const errorList = []
  const assignedClientIds = []
  const assignedCircuitIds = []

  const controllers = await db.controllers.toArray()
  const extendedClientIds = await getExtendedClientIds()
  const clients = await db.clients.where('id').noneOf(extendedClientIds).toArray()
  const breakers = await db.breakers.toArray()

  for (const controller of controllers) {
    const errors = await buildControllerPayload(controller.id, true)
    errors.forEach(e => errorList.push(e))
    Object.values(controller.inputs || {}).forEach(id => assignedClientIds.push(id))
    Object.values(controller.outputs || {}).forEach(id => assignedCircuitIds.push(id))
  }

  for (const client of clients) {
    if (client.mac === 'ff:ff:ff:ff:ff:ff') {
      errorList.push(`Client "${client.name}" has an invalid MAC address and will not be able to be provisioned.`)
    }
    if (!client.hids?.length) {
      errorList.push(`Client "${client.name}" does not have any buttons or switches defined.`)
    }
  }

  for (const breaker of breakers) {
    const circuits = await db.circuits.where('breaker').equals(breaker.id).toArray()
    const load = circuits.reduce((sum, c) => sum + Number(c.load_amperage || 0), 0)
    const utilization = Math.round((load / breaker.amperage) * 100)
    if (utilization > 80) {
      errorList.push(`Breaker "${breaker.name}" has a utilization of ${utilization}%, which may not be safe.`)
    }
  }

  const unassignedClients = await db.clients.where('id').noneOf(assignedClientIds).toArray()
  unassignedClients.forEach(c => errorList.push(`Client ${c.name} has not been assigned to an input.`))

  const unassignedCircuits = await db.circuits.where('id').noneOf(assignedCircuitIds).toArray()
  unassignedCircuits.forEach(c => errorList.push(`Circuit ${c.name} has not been assigned to an output.`))

  return errorList
}
