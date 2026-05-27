import requests


class TestOTA:
    def test_trigger_app_ota_invalid_url_returns_400(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/api/ota/app",
            json={"url": "not-a-valid-url"},
            headers=auth_headers,
        )
        assert r.status_code == 400

    def test_trigger_app_ota_missing_url_returns_400(self, base_url, auth_headers):
        r = requests.post(f"{base_url}/api/ota/app", json={}, headers=auth_headers)
        assert r.status_code == 400

    def test_trigger_app_ota_missing_auth_returns_401(self, base_url):
        r = requests.post(
            f"{base_url}/api/ota/app", json={"url": "http://example.com/fw.bin"}
        )
        assert r.status_code == 401

    def test_trigger_app_ota_duplicate_returns_409(self, base_url, auth_headers):
        r1 = requests.post(
            f"{base_url}/api/ota/app",
            json={"url": "http://example.com/fw.bin"},
            headers=auth_headers,
        )
        assert r1.status_code == 202
        r2 = requests.post(
            f"{base_url}/api/ota/app",
            json={"url": "http://example.com/fw.bin"},
            headers=auth_headers,
        )
        assert r2.status_code == 409

    def test_trigger_ui_ota_invalid_url_returns_400(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/api/ota/ui",
            json={"url": "not-a-valid-url"},
            headers=auth_headers,
        )
        assert r.status_code == 400

    def test_trigger_ui_ota_missing_url_returns_400(self, base_url, auth_headers):
        r = requests.post(f"{base_url}/api/ota/ui", json={}, headers=auth_headers)
        assert r.status_code == 400

    def test_trigger_ui_ota_missing_auth_returns_401(self, base_url):
        r = requests.post(
            f"{base_url}/api/ota/ui", json={"url": "http://example.com/ui.bin"}
        )
        assert r.status_code == 401

    def test_trigger_ui_ota_duplicate_returns_409(self, base_url, auth_headers):
        r1 = requests.post(
            f"{base_url}/api/ota/ui",
            json={"url": "http://example.com/ui.bin"},
            headers=auth_headers,
        )
        assert r1.status_code == 202
        r2 = requests.post(
            f"{base_url}/api/ota/ui",
            json={"url": "http://example.com/ui.bin"},
            headers=auth_headers,
        )
        assert r2.status_code == 409
