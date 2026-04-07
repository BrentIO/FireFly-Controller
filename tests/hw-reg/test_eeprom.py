import uuid
import requests
import pytest


TEST_EEPROM_PAYLOAD = {
    "uuid": str(uuid.uuid4()),
    "product_id": "FFC0806-2305",
    "key": "a" * 64,
}


@pytest.fixture(scope="module", autouse=True)
def cleanup(base_url, auth_headers):
    """Delete test EEPROM data after all tests in this module complete."""
    yield
    requests.delete(f"{base_url}/api/eeprom", headers=auth_headers)


class TestEEPROM:
    def test_delete_eeprom_for_clean_state(self, base_url, auth_headers):
        """Delete any existing EEPROM config so subsequent tests start clean."""
        r = requests.delete(f"{base_url}/api/eeprom", headers=auth_headers)
        assert r.status_code in (204, 404)

    def test_get_eeprom_unconfigured_returns_404(self, base_url):
        r = requests.get(f"{base_url}/api/eeprom")
        assert r.status_code == 404

    def test_write_eeprom_returns_201(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/api/eeprom", json=TEST_EEPROM_PAYLOAD, headers=auth_headers
        )
        assert r.status_code == 201

    def test_get_eeprom_after_write_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/eeprom")
        assert r.status_code == 200

    def test_get_eeprom_has_required_fields(self, base_url):
        r = requests.get(f"{base_url}/api/eeprom")
        body = r.json()
        assert "uuid" in body
        assert "product_id" in body
        assert "key" in body

    def test_get_eeprom_matches_written_data(self, base_url):
        r = requests.get(f"{base_url}/api/eeprom")
        body = r.json()
        assert body["uuid"] == TEST_EEPROM_PAYLOAD["uuid"]
        assert body["product_id"] == TEST_EEPROM_PAYLOAD["product_id"]

    def test_write_eeprom_when_already_configured_returns_400(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/api/eeprom", json=TEST_EEPROM_PAYLOAD, headers=auth_headers
        )
        assert r.status_code == 400

    def test_delete_eeprom_returns_204(self, base_url, auth_headers):
        r = requests.delete(f"{base_url}/api/eeprom", headers=auth_headers)
        assert r.status_code == 204

    def test_get_eeprom_after_delete_returns_404(self, base_url):
        r = requests.get(f"{base_url}/api/eeprom")
        assert r.status_code == 404

    def test_write_eeprom_missing_auth_returns_401(self, base_url):
        r = requests.post(f"{base_url}/api/eeprom", json=TEST_EEPROM_PAYLOAD)
        assert r.status_code == 401

    def test_delete_eeprom_missing_auth_returns_401(self, base_url):
        r = requests.delete(f"{base_url}/api/eeprom")
        assert r.status_code == 401

    def test_write_eeprom_missing_uuid_returns_400(self, base_url, auth_headers):
        bad_payload = {k: v for k, v in TEST_EEPROM_PAYLOAD.items() if k != "uuid"}
        r = requests.post(f"{base_url}/api/eeprom", json=bad_payload, headers=auth_headers)
        assert r.status_code == 400

    def test_write_eeprom_key_wrong_length_returns_400(self, base_url, auth_headers):
        bad_payload = {**TEST_EEPROM_PAYLOAD, "uuid": str(uuid.uuid4()), "key": "short"}
        r = requests.post(f"{base_url}/api/eeprom", json=bad_payload, headers=auth_headers)
        assert r.status_code == 400
