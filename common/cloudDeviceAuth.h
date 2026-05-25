#pragma once

#include <esp_efuse.h>
#include "esp_efuse_table.h"
#include <esp_http_client.h>
#include <esp_random.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/sha256.h>
#include <mbedtls/base64.h>
#include "common/deviceIdentity.h"
#include <time.h>
#include <string.h>

static int _cloudDeviceAuth_rng(void*, unsigned char* buf, size_t len) {
  esp_fill_random(buf, len);
  return 0;
}

/**
 * Derives key_auth from the eFuse master key, generates a random nonce and
 * timestamp-based signature, and sets the four device-authentication headers
 * on @p client.
 *
 * Reads the master key directly from eFuse each call so it is safe to use
 * even after deviceIdentity.data.key has been zeroed for security.  All
 * sensitive material (master key, key_auth) is zeroed before returning.
 *
 * @param client  Initialized esp_http_client handle
 * @param uuid    Device UUID string (deviceIdentity.data.uuid)
 * @param now     Current UTC time as time_t
 * @return true on success; false if key derivation or signing fails
 */
static bool cloudDeviceAuth_setHeaders(esp_http_client_handle_t client, const char* uuid, time_t now) {

  uint8_t master_key[32] = {};
  esp_efuse_read_block(EFUSE_BLK3, master_key, EFUSE_KEY_OFFSET_BITS, EFUSE_KEY_SIZE_BITS);

  uint8_t key_auth[32];
  if (mbedtls_hkdf(mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
                   nullptr, 0,
                   master_key, sizeof(master_key),
                   (const uint8_t*)"firefly-auth-v1", 15,
                   key_auth, 32) != 0) {
    memset(master_key, 0, 32);
    memset(key_auth, 0, 32);
    return false;
  }
  memset(master_key, 0, 32);

  uint8_t nonce[32];
  esp_fill_random(nonce, 32);

  char timestampBuf[25];
  struct tm tmNow;
  gmtime_r(&now, &tmNow);
  strftime(timestampBuf, sizeof(timestampBuf), "%Y-%m-%dT%H:%M:%SZ", &tmNow);

  size_t tsLen = strlen(timestampBuf);
  uint8_t sigInput[32 + 25];
  memcpy(sigInput, nonce, 32);
  memcpy(sigInput + 32, timestampBuf, tsLen);

  uint8_t hash[32];
  mbedtls_sha256_context sha_ctx;
  mbedtls_sha256_init(&sha_ctx);
  mbedtls_sha256_starts(&sha_ctx, 0);
  mbedtls_sha256_update(&sha_ctx, sigInput, 32 + tsLen);
  mbedtls_sha256_finish(&sha_ctx, hash);
  mbedtls_sha256_free(&sha_ctx);

  mbedtls_ecdsa_context ecdsa;
  mbedtls_ecdsa_init(&ecdsa);
  mbedtls_ecp_group_load(&ecdsa.MBEDTLS_PRIVATE(grp), MBEDTLS_ECP_DP_SECP256R1);
  mbedtls_mpi_read_binary(&ecdsa.MBEDTLS_PRIVATE(d), key_auth, 32);
  memset(key_auth, 0, 32);

  uint8_t sig[72]; size_t sigLen = 0;
  bool signOk = (mbedtls_ecdsa_write_signature(&ecdsa, MBEDTLS_MD_SHA256,
                                               hash, 32, sig, sizeof(sig), &sigLen,
                                               _cloudDeviceAuth_rng, nullptr) == 0);
  mbedtls_ecdsa_free(&ecdsa);

  if (!signOk) return false;

  uint8_t nonceB64[48]; size_t nonceB64Len = 0;
  uint8_t sigB64[100];  size_t sigB64Len = 0;
  mbedtls_base64_encode(nonceB64, sizeof(nonceB64), &nonceB64Len, nonce, 32);
  mbedtls_base64_encode(sigB64,   sizeof(sigB64),   &sigB64Len,   sig,   sigLen);

  esp_http_client_set_header(client, "X-Device-UUID",      uuid);
  esp_http_client_set_header(client, "X-Device-Nonce",     (char*)nonceB64);
  esp_http_client_set_header(client, "X-Device-Timestamp", timestampBuf);
  esp_http_client_set_header(client, "X-Device-Signature", (char*)sigB64);

  return true;
}
