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
        assert "psram_size" in body

