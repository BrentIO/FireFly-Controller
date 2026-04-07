import requests


class TestVersion:
    def test_get_app_version_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/version")
        assert r.status_code == 200

    def test_get_app_version_has_required_fields(self, base_url):
        r = requests.get(f"{base_url}/api/version")
        body = r.json()
        assert "application" in body
        assert "product_hex" in body

    def test_get_ui_version_returns_200_or_404(self, base_url):
        # 404 is acceptable if no www filesystem is mounted
        r = requests.get(f"{base_url}/ui/version")
        assert r.status_code in (200, 404)
