import requests


VALID_BINARIES = [
    {"partition": "app", "url": "http://example.com/Controller.ino.bin"},
    {"partition": "ui", "url": "http://example.com/ui.bin"},
]


class TestOTA:
    def test_trigger_ota_missing_binaries_returns_400(self, base_url, auth_headers):
        r = requests.post(f"{base_url}/api/ota", json={}, headers=auth_headers)
        assert r.status_code == 400

    def test_trigger_ota_empty_binaries_returns_400(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/api/ota", json={"binaries": []}, headers=auth_headers
        )
        assert r.status_code == 400

    def test_trigger_ota_invalid_url_returns_400(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/api/ota",
            json={"binaries": [{"partition": "app", "url": "not-a-valid-url"}]},
            headers=auth_headers,
        )
        assert r.status_code == 400

    def test_trigger_ota_missing_auth_returns_401(self, base_url):
        r = requests.post(
            f"{base_url}/api/ota",
            json={"binaries": VALID_BINARIES},
        )
        assert r.status_code == 401

    def test_trigger_ota_duplicate_returns_409(self, base_url, auth_headers):
        r1 = requests.post(
            f"{base_url}/api/ota",
            json={"binaries": VALID_BINARIES},
            headers=auth_headers,
        )
        assert r1.status_code == 202
        r2 = requests.post(
            f"{base_url}/api/ota",
            json={"binaries": VALID_BINARIES},
            headers=auth_headers,
        )
        assert r2.status_code == 409
