import requests


class TestReboot:
    def test_reboot_mcu_missing_auth_returns_401(self, base_url):
        r = requests.post(f"{base_url}/api/mcu/reboot")
        assert r.status_code == 401

    def test_reboot_mcu_returns_204(self, base_url, auth_headers):
        """Reboots the device. Must run last — the visual token is invalidated on
        reboot, so any subsequent tests requiring auth will fail."""
        r = requests.post(f"{base_url}/api/mcu/reboot", headers=auth_headers)
        assert r.status_code == 204
