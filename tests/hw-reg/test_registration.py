import requests


class TestRegistration:
    def test_get_registration_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/api/registration")
        assert r.status_code == 401

    def test_get_registration_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/registration", headers=auth_headers)
        assert r.status_code == 200

    def test_get_registration_returns_json_content_type(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/registration", headers=auth_headers)
        assert "application/json" in r.headers.get("Content-Type", "")

    def test_get_registration_has_registered_field(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/registration", headers=auth_headers)
        assert isinstance(r.json().get("registered"), bool)

    def test_get_registration_has_checked_at_field(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/registration", headers=auth_headers)
        assert isinstance(r.json().get("checked_at"), int)

    def test_post_registration_missing_auth_returns_401(self, base_url):
        r = requests.post(f"{base_url}/api/registration", json={})
        assert r.status_code == 401

    def test_post_registration_missing_key_header_returns_400(self, base_url, auth_headers):
        r = requests.post(f"{base_url}/api/registration", json={}, headers=auth_headers)
        assert r.status_code == 400

    def test_post_registration_key_too_short_returns_400(self, base_url, auth_headers):
        headers = {**auth_headers, "X-Registration-Key": "ABC"}
        r = requests.post(f"{base_url}/api/registration", json={}, headers=headers)
        assert r.status_code == 400

    def test_post_registration_key_too_long_returns_400(self, base_url, auth_headers):
        headers = {**auth_headers, "X-Registration-Key": "ABCDEFG"}
        r = requests.post(f"{base_url}/api/registration", json={}, headers=headers)
        assert r.status_code == 400

    def test_post_registration_with_bad_url_returns_502(self, base_url, auth_headers):
        headers = {**auth_headers, "X-Registration-Key": "ABC123"}
        r = requests.post(
            f"{base_url}/api/registration",
            json={"url": "http://127.0.0.1:9/invalid"},
            headers=headers,
        )
        assert r.status_code == 502
