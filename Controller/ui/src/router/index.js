import { createRouter, createWebHashHistory } from 'vue-router'
import ControllersView from '../views/ControllersView.vue'
import ClientsView from '../views/ClientsView.vue'
import ClientDetailView from '../views/ClientDetailView.vue'
import InputsView from '../views/InputsView.vue'
import OutputsView from '../views/OutputsView.vue'
import CircuitsView from '../views/CircuitsView.vue'
import BreakersView from '../views/BreakersView.vue'
import AreasView from '../views/config/AreasView.vue'
import CertificatesView from '../views/config/CertificatesView.vue'
import ColorsView from '../views/config/ColorsView.vue'
import IconsView from '../views/config/IconsView.vue'
import MqttView from '../views/config/MqttView.vue'
import OtaView from '../views/config/OtaView.vue'
import TagsView from '../views/config/TagsView.vue'
import WifiView from '../views/config/WifiView.vue'
import ImportView from '../views/config/ImportView.vue'
import ResetView from '../views/config/ResetView.vue'
import RelayTypesView from '../views/config/RelayTypesView.vue'
import BomView from '../views/reports/BomView.vue'
import ControlCircuitsView from '../views/reports/ControlCircuitsView.vue'

const routes = [
  { path: '/', redirect: '/controllers' },
  { path: '/controllers', component: ControllersView },
  { path: '/clients', component: ClientsView },
  { path: '/clients/:id', component: ClientDetailView },
  { path: '/inputs', component: InputsView },
  { path: '/outputs', component: OutputsView },
  { path: '/circuits', component: CircuitsView },
  { path: '/breakers', component: BreakersView },
  { path: '/config/areas', component: AreasView },
  { path: '/config/certificates', component: CertificatesView },
  { path: '/config/colors', component: ColorsView },
  { path: '/config/icons', component: IconsView },
  { path: '/config/mqtt', component: MqttView },
  { path: '/config/ota', component: OtaView },
  { path: '/config/tags', component: TagsView },
  { path: '/config/wifi', component: WifiView },
  { path: '/config/import', component: ImportView },
  { path: '/config/reset', component: ResetView },
  { path: '/config/relay-types', component: RelayTypesView },
  { path: '/reports/bom', component: BomView },
  { path: '/reports/control-circuits', component: ControlCircuitsView }
]

const router = createRouter({
  history: createWebHashHistory(),
  routes
})


export default router
