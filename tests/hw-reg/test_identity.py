import uuid
import requests
import pytest


TEST_IDENTITY_PAYLOAD = {
    "uuid": str(uuid.uuid4()),
    "product_id": "FFC0806-2305",
    "key": "a" * 64,
}


@pytest.fixture(scope="module", autouse=True)
def cleanup(base_url, auth_headers):
    """Delete test identity data after all tests in this module complete."""
    yield
    requests.delete(f"{base_url}/api/identity", headers=auth_headers)


class TestIdentity:
    def test_delete_identity_for_clean_state(self, base_url, auth_headers):
        """Delete any existing identity config so subsequent tests start clean."""
        r = requests.delete(f"{base_url}/api/identity", headers=auth_headers)
        assert r.status_code in (204, 404)

    def test_get_identity_unconfigured_returns_404(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/identity", headers=auth_headers)
        assert r.status_code == 404

    def test_write_identity_returns_201(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/api/identity", json=TEST_IDENTITY_PAYLOAD, headers=auth_headers
        )
        assert r.status_code == 201

    def test_get_identity_after_write_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/identity", headers=auth_headers)
        assert r.status_code == 200

    def test_get_identity_has_required_fields(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/identity", headers=auth_headers)
        body = r.json()
        assert "uuid" in body
        assert "product_id" in body
        assert "key" in body

    def test_get_identity_matches_written_data(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/identity", headers=auth_headers)
        body = r.json()
        assert body["uuid"] == TEST_IDENTITY_PAYLOAD["uuid"]
        assert body["product_id"] == TEST_IDENTITY_PAYLOAD["product_id"]

    def test_write_identity_when_already_configured_returns_400(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/api/identity", json=TEST_IDENTITY_PAYLOAD, headers=auth_headers
        )
        assert r.status_code == 400

    def test_delete_identity_returns_204(self, base_url, auth_headers):
        r = requests.delete(f"{base_url}/api/identity", headers=auth_headers)
        assert r.status_code == 204

    def test_get_identity_after_delete_returns_404(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/identity", headers=auth_headers)
        assert r.status_code == 404

    def test_get_identity_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/api/identity")
        assert r.status_code == 401

    def test_write_identity_missing_auth_returns_401(self, base_url):
        r = requests.post(f"{base_url}/api/identity", json=TEST_IDENTITY_PAYLOAD)
        assert r.status_code == 401

    def test_delete_identity_missing_auth_returns_401(self, base_url):
        r = requests.delete(f"{base_url}/api/identity")
        assert r.status_code == 401

    def test_write_identity_missing_uuid_returns_400(self, base_url, auth_headers):
        bad_payload = {k: v for k, v in TEST_IDENTITY_PAYLOAD.items() if k != "uuid"}
        r = requests.post(f"{base_url}/api/identity", json=bad_payload, headers=auth_headers)
        assert r.status_code == 400

    def test_write_identity_key_wrong_length_returns_400(self, base_url, auth_headers):
        bad_payload = {**TEST_IDENTITY_PAYLOAD, "uuid": str(uuid.uuid4()), "key": "short"}
        r = requests.post(f"{base_url}/api/identity", json=bad_payload, headers=auth_headers)
        assert r.status_code == 400
