import requests


INTERFACE_TYPES = ["ethernet", "wifi", "wifi_ap", "bluetooth"]


class TestNetwork:
    def test_list_network_interfaces_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/network")
        assert r.status_code == 200

    def test_list_network_interfaces_returns_list(self, base_url):
        r = requests.get(f"{base_url}/api/network")
        assert isinstance(r.json(), list)

    def test_list_network_interfaces_have_required_fields(self, base_url):
        r = requests.get(f"{base_url}/api/network")
        for item in r.json():
            assert "mac_address" in item
            assert "interface" in item

    def test_get_ethernet_interface_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/network/ethernet")
        assert r.status_code == 200

    def test_get_wifi_interface_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/network/wifi")
        assert r.status_code == 200

    def test_get_wifi_ap_interface_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/network/wifi_ap")
        assert r.status_code == 200

    def test_get_bluetooth_interface_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/network/bluetooth")
        assert r.status_code == 200

    def test_get_interface_has_required_fields(self, base_url):
        r = requests.get(f"{base_url}/api/network/wifi")
        body = r.json()
        assert "mac_address" in body
        assert "interface" in body

    def test_get_invalid_interface_returns_404(self, base_url):
        r = requests.get(f"{base_url}/api/network/invalid")
        assert r.status_code == 404
