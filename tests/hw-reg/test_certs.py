import io
import requests
import pytest


TEST_CERT_FILENAME = "test-cert.pem"
TEST_CERT_CONTENT = b"-----BEGIN CERTIFICATE-----\nMIIBIjANBgkqhkiG9w0BAQEFAAOCAQ8AMIIBCgKCAQEA0Z3VS5JJcds3xHn/ygWep4\n-----END CERTIFICATE-----\n"


@pytest.fixture(scope="module", autouse=True)
def cleanup(base_url, auth_headers):
    """Ensure clean state before and after all tests in this module."""
    requests.delete(f"{base_url}/certs/{TEST_CERT_FILENAME}", headers=auth_headers)
    yield
    requests.delete(f"{base_url}/certs/{TEST_CERT_FILENAME}", headers=auth_headers)


class TestCerts:
    def test_list_certs_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/certs", headers=auth_headers)
        assert r.status_code == 200

    def test_list_certs_returns_list(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/certs", headers=auth_headers)
        assert isinstance(r.json(), list)

    def test_list_certs_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/certs")
        assert r.status_code == 401

    def test_upload_cert_returns_201(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/certs",
            files={"file": (TEST_CERT_FILENAME, io.BytesIO(TEST_CERT_CONTENT), "application/x-pem-file")},
            headers=auth_headers,
        )
        assert r.status_code == 201

    def test_upload_duplicate_cert_returns_403(self, base_url, auth_headers):
        r = requests.post(
            f"{base_url}/certs",
            files={"file": (TEST_CERT_FILENAME, io.BytesIO(TEST_CERT_CONTENT), "application/x-pem-file")},
            headers=auth_headers,
        )
        assert r.status_code == 403

    def test_get_cert_returns_200(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/certs/{TEST_CERT_FILENAME}", headers=auth_headers)
        assert r.status_code == 200

    def test_get_nonexistent_cert_returns_404(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/certs/does-not-exist.pem", headers=auth_headers)
        assert r.status_code == 404

    def test_delete_cert_returns_204(self, base_url, auth_headers):
        r = requests.delete(f"{base_url}/certs/{TEST_CERT_FILENAME}", headers=auth_headers)
        assert r.status_code == 204

    def test_get_deleted_cert_returns_404(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/certs/{TEST_CERT_FILENAME}", headers=auth_headers)
        assert r.status_code == 404

    def test_upload_cert_missing_auth_returns_401(self, base_url):
        r = requests.post(
            f"{base_url}/certs",
            files={"file": (TEST_CERT_FILENAME, io.BytesIO(TEST_CERT_CONTENT), "application/x-pem-file")},
        )
        assert r.status_code == 401
