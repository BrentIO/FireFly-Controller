import requests


VALID_PERIPHERAL_TYPES = {"INPUT", "OUTPUT", "TEMPERATURE", "OLED", "EEPROM", "NVS"}


class TestPeripherals:
    def test_get_peripherals_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/peripherals")
        assert r.status_code == 200

    def test_get_peripherals_returns_list(self, base_url):
        r = requests.get(f"{base_url}/api/peripherals")
        assert isinstance(r.json(), list)

    def test_get_peripherals_have_required_fields(self, base_url):
        r = requests.get(f"{base_url}/api/peripherals")
        for peripheral in r.json():
            assert "address" in peripheral
            assert "type" in peripheral
            assert "online" in peripheral

    def test_get_peripherals_type_is_valid(self, base_url):
        r = requests.get(f"{base_url}/api/peripherals")
        for peripheral in r.json():
            assert peripheral["type"] in VALID_PERIPHERAL_TYPES

    def test_get_peripherals_online_is_boolean(self, base_url):
        r = requests.get(f"{base_url}/api/peripherals")
        for peripheral in r.json():
            assert isinstance(peripheral["online"], bool)
