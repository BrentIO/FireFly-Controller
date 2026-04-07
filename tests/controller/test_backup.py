import requests
import pytest


@pytest.fixture(scope="module")
def existing_backup(base_url, auth_headers):
    """Return the existing backup content if one exists, otherwise None."""
    r = requests.get(f"{base_url}/backup", headers=auth_headers)
    if r.status_code == 200:
        return r.json()
    return None


@pytest.fixture(scope="module", autouse=True)
def restore_backup(base_url, auth_headers, existing_backup):
    """Restore original backup state after tests complete."""
    yield
    if existing_backup is not None:
        requests.put(f"{base_url}/backup", json=existing_backup, headers=auth_headers)
    else:
        requests.delete(f"{base_url}/backup", headers=auth_headers)


SAMPLE_BACKUP = {
    "formatName": "dexie",
    "formatVersion": 1,
    "data": {
        "databaseName": "firefly-test",
        "databaseVersion": 1,
        "tables": [],
        "data": [],
    },
}


class TestBackup:
    def test_get_backup_returns_200_or_404(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/backup", headers=auth_headers)
        assert r.status_code in (200, 404)

    def test_put_backup_returns_204(self, base_url, auth_headers):
        r = requests.put(f"{base_url}/backup", json=SAMPLE_BACKUP, headers=auth_headers)
        assert r.status_code == 204

    def test_get_backup_after_put_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/backup", headers=auth_headers)
        assert r.status_code == 200

    def test_delete_backup_returns_204(self, base_url, auth_headers):
        r = requests.delete(f"{base_url}/backup", headers=auth_headers)
        assert r.status_code == 204

    def test_get_backup_after_delete_returns_404(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/backup", headers=auth_headers)
        assert r.status_code == 404

    def test_get_backup_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/backup")
        assert r.status_code == 401

    def test_put_backup_missing_auth_returns_401(self, base_url):
        r = requests.put(f"{base_url}/backup", json=SAMPLE_BACKUP)
        assert r.status_code == 401

    def test_delete_backup_missing_auth_returns_401(self, base_url):
        r = requests.delete(f"{base_url}/backup")
        assert r.status_code == 401
