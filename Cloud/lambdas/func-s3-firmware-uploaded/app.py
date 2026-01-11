import json
import boto3
import hashlib
import os
import zipfile
import uuid
import time
from urllib.parse import unquote_plus

s3 = boto3.client("s3")
dynamodb = boto3.resource("dynamodb")

SUCCESS_TABLE = os.environ["DDB_SUCCESS_TABLE"]
ERROR_TABLE = os.environ["DDB_ERROR_TABLE"]

INCOMING_PREFIX = "incoming/"
PROCESSING_PREFIX = "processing/"
PROCESSED_PREFIX = "processed/"
ERROR_PREFIX = "errors/"
TMP_DIR = "/tmp"

success_table = dynamodb.Table(SUCCESS_TABLE)
error_table = dynamodb.Table(ERROR_TABLE)


def sha256_file(path):
    h = hashlib.sha256()
    with open(path, "rb") as f:
        for chunk in iter(lambda: f.read(8192), b""):
            h.update(chunk)
    return h.hexdigest()


def validate_manifest_schema(manifest):
    required_fields = [
        "class", "product_id", "application",
        "branch", "version", "commit",
        "created", "files"
    ]

    for field in required_fields:
        if field not in manifest:
            raise Exception(f"Manifest missing required field: {field}")

    if not isinstance(manifest["files"], list):
        raise Exception("Manifest 'files' must be an array")

    for entry in manifest["files"]:
        if "name" not in entry or "sha256" not in entry:
            raise Exception("Each file entry must contain 'name' and 'sha256'")
        if len(entry["sha256"]) != 64:
            raise Exception(f"Invalid SHA256 for {entry['name']}")


def log_error(zip_name, error_message, original_name=None):
    item = {
        "zip_name": zip_name,
        "error": error_message,
        "timestamp": int(time.time()),
        "deleted": False
    }
    if original_name:
        item["original_name"] = original_name

    error_table.put_item(Item=item)


def move_object(bucket, source_key, dest_key):
    s3.copy_object(
        Bucket=bucket,
        CopySource={"Bucket": bucket, "Key": source_key},
        Key=dest_key,
    )
    s3.delete_object(Bucket=bucket, Key=source_key)


def lambda_handler(event, context):
    record = event["Records"][0]
    bucket = record["s3"]["bucket"]["name"]
    key = unquote_plus(record["s3"]["object"]["key"])

    if not key.endswith(".zip"):
        return

    original_filename = os.path.basename(key)
    uuid_name = f"{uuid.uuid4()}.zip"

    incoming_key = key
    processing_key = f"{PROCESSING_PREFIX}{uuid_name}"
    processed_key = f"{PROCESSED_PREFIX}{uuid_name}"
    error_key = f"{ERROR_PREFIX}{uuid_name}"

    try:
        # Rename incoming/<file> → processing/<uuid>.zip
        move_object(bucket, incoming_key, processing_key)

        # Get metadata
        head = s3.head_object(Bucket=bucket, Key=processing_key)
        zip_size = head["ContentLength"]

        # Download ZIP
        zip_path = os.path.join(TMP_DIR, uuid_name)
        s3.download_file(bucket, processing_key, zip_path)

        zip_sha256 = sha256_file(zip_path)

        extract_dir = os.path.join(TMP_DIR, "unzipped")
        os.makedirs(extract_dir, exist_ok=True)

        with zipfile.ZipFile(zip_path, "r") as z:
            z.extractall(extract_dir)

        manifest_path = os.path.join(extract_dir, "manifest.json")
        if not os.path.exists(manifest_path):
            raise Exception("manifest.json missing")

        with open(manifest_path) as f:
            manifest = json.load(f)

        # Validate manifest structure
        validate_manifest_schema(manifest)

        # Validate file hashes
        for entry in manifest["files"]:
            file_path = os.path.join(extract_dir, entry["name"])
            if not os.path.exists(file_path):
                raise Exception(f"Missing file: {entry['name']}")

            actual_sha256 = sha256_file(file_path)
            if actual_sha256 != entry["sha256"]:
                raise Exception(f"SHA256 mismatch for {entry['name']}")

        # Write success record
        success_table.put_item(
            Item={
                "zip_name": uuid_name,
                "original_name": original_filename,
                "manifest": manifest,
                "zip_sha256": zip_sha256,
                "zip_size": zip_size,
                "uploaded_at": int(time.time()),
                "deleted": False
            }
        )

        # Move to processed/
        move_object(bucket, processing_key, processed_key)

    except Exception as e:
        error_message = str(e)

        log_error(uuid_name, error_message, original_name=original_filename)

        try:
            move_object(bucket, processing_key, error_key)
        except Exception:
            pass

        raise
