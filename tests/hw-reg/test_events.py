import requests


class TestEvents:
    def test_get_events_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/events")
        assert r.status_code == 200

    def test_get_events_returns_list(self, base_url):
        r = requests.get(f"{base_url}/api/events")
        assert isinstance(r.json(), list)
