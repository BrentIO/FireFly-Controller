import { createRouter, createWebHashHistory } from 'vue-router'
import LoginView from '../views/LoginView.vue'
import McuView from '../views/McuView.vue'
import NetworkView from '../views/NetworkView.vue'
import IdentityView from '../views/IdentityView.vue'
import RegistrationView from '../views/RegistrationView.vue'
import PartitionsView from '../views/PartitionsView.vue'
import PeripheralsView from '../views/PeripheralsView.vue'
import EventLogView from '../views/EventLogView.vue'
import ErrorLogView from '../views/ErrorLogView.vue'

const routes = [
  { path: '/', redirect: '/mcu' },
  { path: '/login', component: LoginView, meta: { public: true } },
  { path: '/mcu', component: McuView },
  { path: '/network', component: NetworkView },
  { path: '/identity', component: IdentityView },
  { path: '/registration', component: RegistrationView },
  { path: '/partitions', component: PartitionsView },
  { path: '/peripherals', component: PeripheralsView },
  { path: '/events', component: EventLogView },
  { path: '/errors', component: ErrorLogView }
]

const router = createRouter({
  history: createWebHashHistory(),
  routes
})

router.beforeEach((to) => {
  if (to.meta.public) return true
  if (!sessionStorage.getItem('visual_token')) return '/login'
})

export default router
