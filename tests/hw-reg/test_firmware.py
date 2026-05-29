import time
import requests


class TestFirmware:
    def test_get_firmware_missing_auth_returns_401(self, base_url):
        r = requests.get(f"{base_url}/api/firmware")
        assert r.status_code == 401

    def test_get_firmware_post_returns_405(self, base_url, auth_headers):
        r = requests.post(f"{base_url}/api/firmware", headers=auth_headers)
        assert r.status_code == 405

    def test_get_firmware_returns_200_or_202(self, base_url, auth_headers):
        r = requests.get(f"{base_url}/api/firmware", headers=auth_headers)
        assert r.status_code in (200, 202)

    def test_get_firmware_eventually_returns_200(self, base_url, auth_headers):
        """Poll until ready (max 15 s); first call may legitimately return 202."""
        deadline = time.time() + 15
        while time.time() < deadline:
            r = requests.get(f"{base_url}/api/firmware", headers=auth_headers)
            if r.status_code == 200:
                break
            assert r.status_code == 202, f"Unexpected status {r.status_code}"
            time.sleep(1)
        else:
            raise AssertionError("GET /api/firmware never returned 200 within 15 s")

    def test_get_firmware_200_body_has_versions(self, base_url, auth_headers):
        """Wait for a 200 and verify the response shape."""
        deadline = time.time() + 15
        body = None
        while time.time() < deadline:
            r = requests.get(f"{base_url}/api/firmware", headers=auth_headers)
            if r.status_code == 200:
                body = r.json()
                break
            time.sleep(1)
        assert body is not None, "Never received a 200 response"
        assert "versions" in body
        assert isinstance(body["versions"], list)

    def test_get_firmware_versions_have_required_fields(self, base_url, auth_headers):
        deadline = time.time() + 15
        body = None
        while time.time() < deadline:
            r = requests.get(f"{base_url}/api/firmware", headers=auth_headers)
            if r.status_code == 200:
                body = r.json()
                break
            time.sleep(1)
        assert body is not None, "Never received a 200 response"
        for item in body["versions"]:
            assert "version" in item
            assert "application_name" in item
            assert "binaries" in item
            assert isinstance(item["binaries"], list)

    def test_get_firmware_cached_result_returns_200(self, base_url, auth_headers):
        """A second call immediately after the first 200 must also return 200."""
        deadline = time.time() + 15
        while time.time() < deadline:
            r = requests.get(f"{base_url}/api/firmware", headers=auth_headers)
            if r.status_code == 200:
                break
            time.sleep(1)
        else:
            raise AssertionError("Never received a 200 to seed the cache")
        r2 = requests.get(f"{base_url}/api/firmware", headers=auth_headers)
        assert r2.status_code == 200
