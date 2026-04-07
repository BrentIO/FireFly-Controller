import requests


class TestErrors:
    def test_get_errors_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/errors")
        assert r.status_code == 200

    def test_get_errors_returns_list(self, base_url):
        r = requests.get(f"{base_url}/api/errors")
        assert isinstance(r.json(), list)
