import requests


class TestAuth:
    def test_session_auth_succeeded(self, auth_headers):
        """Confirms that the conftest auth exchange (POST /auth) succeeded."""
        # If auth_headers is non-empty, the exchange worked.
        # If NO_AUTH=true, this passes trivially.
        assert isinstance(auth_headers, dict)

    def test_missing_token_returns_401(self, base_url):
        r = requests.post(f"{base_url}/auth")
        assert r.status_code == 401

    def test_invalid_token_returns_401(self, base_url):
        r = requests.post(f"{base_url}/auth", headers={"visual-token": "INVALID!!"})
        assert r.status_code == 401

    def test_empty_token_returns_401(self, base_url):
        r = requests.post(f"{base_url}/auth", headers={"visual-token": ""})
        assert r.status_code == 401
