import boto3
import os
import time
from urllib.parse import unquote_plus
from botocore.exceptions import ClientError

dynamodb = boto3.resource("dynamodb")

SUCCESS_TABLE = os.environ["DDB_SUCCESS_TABLE"]
ERROR_TABLE = os.environ["DDB_ERROR_TABLE"]

success_table = dynamodb.Table(SUCCESS_TABLE)
error_table = dynamodb.Table(ERROR_TABLE)

TTL_DAYS = 10
TTL_SECONDS = TTL_DAYS * 24 * 3600


def mark_deleted(table, filename):
    expires_at = int(time.time()) + TTL_SECONDS

    try:
        table.update_item(
            Key={"zip_name": filename},
            UpdateExpression="SET deleted = :d, ttl = :ttl",
            ConditionExpression="attribute_exists(zip_name)",
            ExpressionAttributeValues={
                ":d": True,
                ":ttl": expires_at,
            }
        )
    except ClientError as e:
        if e.response["Error"]["Code"] == "ConditionalCheckFailedException":
            # Item does not exist — ignore
            return
        else:
            raise


def lambda_handler(event, context):
    for record in event["Records"]:
        key = unquote_plus(record["s3"]["object"]["key"])
        filename = os.path.basename(key)

        # Ignore non-zip deletions
        if not filename.endswith(".zip"):
            continue

        if key.startswith("processed/"):
            mark_deleted(success_table, filename)

        elif key.startswith("errors/"):
            mark_deleted(error_table, filename)
