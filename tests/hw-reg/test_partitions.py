import requests


class TestPartitions:
    def test_get_partitions_returns_200(self, base_url):
        r = requests.get(f"{base_url}/api/partitions")
        assert r.status_code == 200

    def test_get_partitions_returns_list(self, base_url):
        r = requests.get(f"{base_url}/api/partitions")
        assert isinstance(r.json(), list)

    def test_get_partitions_have_required_fields(self, base_url):
        r = requests.get(f"{base_url}/api/partitions")
        for partition in r.json():
            assert "type" in partition
            assert "subtype" in partition
            assert "address" in partition
            assert "size" in partition
            assert "label" in partition

    def test_get_partitions_not_empty(self, base_url):
        r = requests.get(f"{base_url}/api/partitions")
        assert len(r.json()) > 0
