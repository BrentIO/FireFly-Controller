import requests


class TestFiles:
    def test_get_files_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/files", headers=auth_headers)
        assert r.status_code == 200

    def test_get_files_returns_object(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/files", headers=auth_headers)
        assert isinstance(r.json(), dict)

    def test_get_files_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/files")
        assert r.status_code == 401
