import os
import requests
import pytest


@pytest.fixture(scope="session")
def base_url():
    ip = os.environ["DEVICE_IP"]
    return f"http://{ip}"


@pytest.fixture(scope="session")
def auth_headers(base_url):
    if os.environ.get("NO_AUTH", "false").lower() == "true":
        return {}
    token = os.environ.get("VISUAL_TOKEN") or input("\nEnter visual token from device OLED: ").strip()
    r = requests.post(f"{base_url}/auth", headers={"visual-token": token})
    assert r.status_code == 204, f"Auth exchange failed: {r.status_code} — check that the token is current"
    # POST /auth promotes the visual token to a long-term token (60-minute TTL).
    # The same token value is now valid for the entire test session.
    return {"visual-token": token}
