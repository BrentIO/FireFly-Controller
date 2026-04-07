import requests


class TestEvents:
    def test_get_events_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/events", headers=auth_headers)
        assert r.status_code == 200

    def test_get_events_returns_list(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/events", headers=auth_headers)
        assert isinstance(r.json(), list)

    def test_get_events_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/api/events")
        assert r.status_code == 401
