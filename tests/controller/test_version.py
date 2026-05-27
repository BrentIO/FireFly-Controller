import requests


class TestVersion:
    def test_get_app_version_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/version", headers=auth_headers)
        assert r.status_code == 200

    def test_get_app_version_has_required_fields(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/version", headers=auth_headers)
        body = r.json()
        assert "application" in body
        assert "product_hex" in body

    def test_get_app_version_has_ui_field(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/version", headers=auth_headers)
        body = r.json()
        assert "ui" in body
        assert body["ui"] is None or isinstance(body["ui"], str)

    def test_get_app_version_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/api/version")
        assert r.status_code == 401
