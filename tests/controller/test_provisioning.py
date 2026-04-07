import requests
import pytest


@pytest.fixture(scope="module", autouse=True)
def restore_provisioning_state(base_url, auth_headers):
    """Restore provisioning mode to its original state after tests complete."""
    r = requests.get(f"{base_url}/api/provisioning", headers=auth_headers)
    original_enabled = r.json().get("enabled", False) if r.status_code == 200 else False
    yield
    if original_enabled:
        requests.put(f"{base_url}/api/provisioning", headers=auth_headers)
    else:
        requests.delete(f"{base_url}/api/provisioning", headers=auth_headers)


class TestProvisioning:
    def test_get_provisioning_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/provisioning", headers=auth_headers)
        assert r.status_code == 200

    def test_get_provisioning_has_enabled_field(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/provisioning", headers=auth_headers)
        assert "enabled" in r.json()

    def test_enable_provisioning_returns_204(self, base_url, auth_headers):
        r = requests.put(f"{base_url}/api/provisioning", headers=auth_headers)
        assert r.status_code == 204

    def test_get_provisioning_shows_enabled(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/provisioning", headers=auth_headers)
        assert r.json().get("enabled") is True

    def test_disable_provisioning_returns_204(self, base_url, auth_headers):
        r = requests.delete(f"{base_url}/api/provisioning", headers=auth_headers)
        assert r.status_code == 204

    def test_get_provisioning_shows_disabled(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/provisioning", headers=auth_headers)
        assert r.json().get("enabled") is False

    def test_get_provisioning_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/api/provisioning")
        assert r.status_code == 401

    def test_enable_provisioning_missing_auth_returns_401(self, base_url):
        r = requests.put(f"{base_url}/api/provisioning")
        assert r.status_code == 401

    def test_disable_provisioning_missing_auth_returns_401(self, base_url):
        r = requests.delete(f"{base_url}/api/provisioning")
        assert r.status_code == 401
