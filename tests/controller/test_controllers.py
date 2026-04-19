import uuid
import requests
import pytest


TEST_UUID = str(uuid.uuid4())
CONTROLLER_PAYLOAD = {"name": "Test Controller"}
CONTROLLER_PAYLOAD_UPDATED = {"name": "Updated Controller", "area": "Test Area"}


@pytest.fixture(scope="module", autouse=True)
def cleanup(base_url, auth_headers):
    """Delete the test controller after all tests in this module complete."""
    yield
    requests.delete(f"{base_url}/api/controllers/{TEST_UUID}", headers=auth_headers)


class TestControllers:
    def test_list_controllers_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/controllers", headers=auth_headers)
        assert r.status_code == 200

    def test_list_controllers_returns_list(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/controllers", headers=auth_headers)
        assert isinstance(r.json(), list)

    def test_list_controllers_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/api/controllers")
        assert r.status_code == 401

    def test_create_controller_returns_204(self, base_url, auth_headers):
        r = requests.put(
            f"{base_url}/api/controllers/{TEST_UUID}",
            json=CONTROLLER_PAYLOAD,
            headers=auth_headers,
        )
        assert r.status_code == 204

    def test_get_controller_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/controllers/{TEST_UUID}", headers=auth_headers)
        assert r.status_code == 200

    def test_get_controller_returns_json_content_type(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/controllers/{TEST_UUID}", headers=auth_headers)
        assert "application/json" in r.headers.get("Content-Type", "")

    def test_get_controller_has_correct_name(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/controllers/{TEST_UUID}", headers=auth_headers)
        assert r.json().get("name") == CONTROLLER_PAYLOAD["name"]

    def test_update_controller_returns_204(self, base_url, auth_headers):
        r = requests.put(
            f"{base_url}/api/controllers/{TEST_UUID}",
            json=CONTROLLER_PAYLOAD_UPDATED,
            headers=auth_headers,
        )
        assert r.status_code == 204

    def test_get_updated_controller_reflects_change(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/controllers/{TEST_UUID}", headers=auth_headers)
        assert r.json().get("name") == CONTROLLER_PAYLOAD_UPDATED["name"]
        assert r.json().get("area") == CONTROLLER_PAYLOAD_UPDATED["area"]

    def test_delete_controller_returns_204(self, base_url, auth_headers):
        r = requests.delete(
            f"{base_url}/api/controllers/{TEST_UUID}", headers=auth_headers
        )
        assert r.status_code == 204

    def test_get_deleted_controller_returns_404(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/controllers/{TEST_UUID}", headers=auth_headers)
        assert r.status_code == 404

    def test_get_nonexistent_controller_returns_404(self, base_url, auth_headers):
        r = requests.get(
            f"{base_url}/api/controllers/{uuid.uuid4()}", headers=auth_headers
        )
        assert r.status_code == 404

    def test_create_controller_missing_auth_returns_401(self, base_url):
        r = requests.put(
            f"{base_url}/api/controllers/{TEST_UUID}", json=CONTROLLER_PAYLOAD
        )
        assert r.status_code == 401

    def test_create_controller_missing_name_returns_400(self, base_url, auth_headers):
        r = requests.put(
            f"{base_url}/api/controllers/{uuid.uuid4()}",
            json={},
            headers=auth_headers,
        )
        assert r.status_code == 400

    def test_create_controller_name_too_long_returns_400(self, base_url, auth_headers):
        r = requests.put(
            f"{base_url}/api/controllers/{uuid.uuid4()}",
            json={"name": "A" * 21},
            headers=auth_headers,
        )
        assert r.status_code == 400
