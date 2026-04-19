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

    def test_get_mcu_chip_features_is_list(self, base_url):
        r = requests.get(f"{base_url}/api/mcu")
        assert isinstance(r.json().get("chip_features"), list)

    def test_get_mcu_chip_features_items_are_strings(self, base_url):
        r = requests.get(f"{base_url}/api/mcu")
        features = r.json().get("chip_features", [])
        assert all(isinstance(f, str) for f in features)

    def test_get_mcu_flash_chip_mode_is_string(self, base_url):
        r = requests.get(f"{base_url}/api/mcu")
        assert isinstance(r.json().get("flash_chip_mode"), str)

