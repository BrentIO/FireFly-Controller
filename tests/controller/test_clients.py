import uuid
import requests
import pytest


TEST_UUID = str(uuid.uuid4())
CLIENT_PAYLOAD = {"name": "Test Client"}
CLIENT_PAYLOAD_UPDATED = {"name": "Updated Client", "area": "Test Area"}


@pytest.fixture(scope="module", autouse=True)
def cleanup(base_url, auth_headers):
    """Delete the test client after all tests in this module complete."""
    yield
    requests.delete(f"{base_url}/api/clients/{TEST_UUID}", headers=auth_headers)


class TestClients:
    def test_list_clients_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/clients", headers=auth_headers)
        assert r.status_code == 200

    def test_list_clients_returns_list(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/clients", headers=auth_headers)
        assert isinstance(r.json(), list)

    def test_list_clients_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/api/clients")
        assert r.status_code == 401

    def test_create_client_returns_204(self, base_url, auth_headers):
        r = requests.put(
            f"{base_url}/api/clients/{TEST_UUID}",
            json=CLIENT_PAYLOAD,
            headers=auth_headers,
        )
        assert r.status_code == 204

    def test_get_client_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/clients/{TEST_UUID}", headers=auth_headers)
        assert r.status_code == 200

    def test_get_client_returns_json_content_type(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/clients/{TEST_UUID}", headers=auth_headers)
        assert "application/json" in r.headers.get("Content-Type", "")

    def test_get_client_has_correct_name(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/clients/{TEST_UUID}", headers=auth_headers)
        assert r.json().get("name") == CLIENT_PAYLOAD["name"]

    def test_update_client_returns_204(self, base_url, auth_headers):
        r = requests.put(
            f"{base_url}/api/clients/{TEST_UUID}",
            json=CLIENT_PAYLOAD_UPDATED,
            headers=auth_headers,
        )
        assert r.status_code == 204

    def test_get_updated_client_reflects_change(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/clients/{TEST_UUID}", headers=auth_headers)
        assert r.json().get("name") == CLIENT_PAYLOAD_UPDATED["name"]
        assert r.json().get("area") == CLIENT_PAYLOAD_UPDATED["area"]

    def test_delete_client_returns_204(self, base_url, auth_headers):
        r = requests.delete(f"{base_url}/api/clients/{TEST_UUID}", headers=auth_headers)
        assert r.status_code == 204

    def test_get_deleted_client_returns_404(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/clients/{TEST_UUID}", headers=auth_headers)
        assert r.status_code == 404

    def test_get_nonexistent_client_returns_404(self, base_url, auth_headers):
        r = requests.get(
            f"{base_url}/api/clients/{uuid.uuid4()}", headers=auth_headers
        )
        assert r.status_code == 404

    def test_create_client_missing_auth_returns_401(self, base_url):
        r = requests.put(f"{base_url}/api/clients/{TEST_UUID}", json=CLIENT_PAYLOAD)
        assert r.status_code == 401

    def test_create_client_missing_name_returns_400(self, base_url, auth_headers):
        r = requests.put(
            f"{base_url}/api/clients/{uuid.uuid4()}",
            json={},
            headers=auth_headers,
        )
        assert r.status_code == 400

    def test_create_client_name_too_long_returns_400(self, base_url, auth_headers):
        r = requests.put(
            f"{base_url}/api/clients/{uuid.uuid4()}",
            json={"name": "A" * 21},
            headers=auth_headers,
        )
        assert r.status_code == 400
