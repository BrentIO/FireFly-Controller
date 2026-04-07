import requests


class TestMCU:
    def test_get_mcu_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/mcu")
        assert r.status_code == 200

    def test_get_mcu_has_required_fields(self, base_url):
        r = requests.get(f"{base_url}/api/mcu")
        body = r.json()
        assert "chip_model" in body
        assert "revision" in body
        assert "flash_chip_size" in body
        assert "boot_time" in body

    def test_reboot_mcu_returns_204(self, base_url, auth_headers):
        """Reboots the device. Run this suite in isolation — the device will be
        unreachable for a few seconds after this test, which will cause any
        subsequent tests to fail."""
        r = requests.post(f"{base_url}/api/mcu/reboot", headers=auth_headers)
        assert r.status_code == 204

    def test_reboot_mcu_missing_auth_returns_401(self, base_url):
        r = requests.post(f"{base_url}/api/mcu/reboot")
        assert r.status_code == 401
