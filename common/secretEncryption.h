#pragma once
#include <Arduino.h>
#include <FS.h>
#include <mbedtls/gcm.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <esp_random.h>

/**
 * AES-256-GCM file encryption using keys derived from the eFuse master secret.
 *
 * Two independent keys are derived:
 *   key_configfs  — HKDF(master, "firefly-configfs-v1") — for on-device config files
 *   key_backup    — HKDF(master, "firefly-backup-v1")   — for cloud backup payloads
 *
 * Binary file format (37-byte header):
 *   Offset  0 :  4 bytes  magic    = { 0x46, 0x46, 0x43, 0x45 } ("FFCE")
 *   Offset  4 :  1 byte   version  = 0x01
 *   Offset  5 : 12 bytes  nonce    (random per write)
 *   Offset 17 :  4 bytes  plaintext_len (uint32_t, little-endian)
 *   Offset 21 : 16 bytes  GCM authentication tag
 *   Offset 37 :  N bytes  ciphertext (N == plaintext_len)
 *
 * The same binary format is used for both config-fs files and cloud backup payloads.
 */
class SecretEncryption {

public:

    /**
     * Derives the file-encryption key from the 32-byte eFuse master secret using
     * HKDF-SHA256 and loads it into the persistent GCM context. Raw key bytes are
     * zeroed immediately after mbedtls_gcm_setkey(). Call once after
     * deviceIdentity.begin(); zero deviceIdentity.data.key[] after this returns true.
     */
    bool begin(const uint8_t* masterKey, size_t masterKeyLen) {

        mbedtls_gcm_init(&_ctx);
        mbedtls_gcm_init(&_backupCtx);

        uint8_t derived[32];

        // Derive config-fs encryption key
        static const uint8_t kInfoConfigfs[] = "firefly-configfs-v1";
        int ret = mbedtls_hkdf(
            mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
            nullptr, 0,
            masterKey, masterKeyLen,
            kInfoConfigfs, sizeof(kInfoConfigfs) - 1,
            derived, sizeof(derived)
        );
        if (ret != 0) {
            memset(derived, 0, sizeof(derived));
            return false;
        }
        ret = mbedtls_gcm_setkey(&_ctx, MBEDTLS_CIPHER_ID_AES, derived, 256);
        memset(derived, 0, sizeof(derived));
        if (ret != 0) {
            mbedtls_gcm_free(&_ctx);
            return false;
        }

        // Derive backup encryption key
        static const uint8_t kInfoBackup[] = "firefly-backup-v1";
        ret = mbedtls_hkdf(
            mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
            nullptr, 0,
            masterKey, masterKeyLen,
            kInfoBackup, sizeof(kInfoBackup) - 1,
            derived, sizeof(derived)
        );
        if (ret != 0) {
            memset(derived, 0, sizeof(derived));
            mbedtls_gcm_free(&_ctx);
            return false;
        }
        ret = mbedtls_gcm_setkey(&_backupCtx, MBEDTLS_CIPHER_ID_AES, derived, 256);
        memset(derived, 0, sizeof(derived));
        if (ret != 0) {
            mbedtls_gcm_free(&_ctx);
            mbedtls_gcm_free(&_backupCtx);
            return false;
        }

        _ready = true;
        return true;
    }


    /**
     * Encrypts a String and writes the encrypted binary to the given path.
     * Returns false on failure; the partial file is removed.
     */
    bool encryptToFile(fs::FS& fs, const String& path, const String& plaintext) {
        return encryptToFile(fs, path,
                             (const uint8_t*)plaintext.c_str(), plaintext.length());
    }


    /**
     * Encrypts a raw byte buffer and writes the encrypted binary to the given path.
     * Returns false on failure; the partial file is removed.
     */
    bool encryptToFile(fs::FS& fs, const String& path,
                       const uint8_t* plaintext, size_t plaintextLen) {

        if (!_ready) return false;

        uint8_t nonce[12];
        esp_fill_random(nonce, sizeof(nonce));

        uint8_t tag[16];

        uint8_t* buf = nullptr;
        if (plaintextLen > 0) {
            buf = (uint8_t*)(psramFound() ? ps_malloc(plaintextLen) : malloc(plaintextLen));
            if (!buf) return false;
            memcpy(buf, plaintext, plaintextLen);
        }

        int ret = mbedtls_gcm_crypt_and_tag(
            &_ctx, MBEDTLS_GCM_ENCRYPT,
            plaintextLen,
            nonce, sizeof(nonce),
            nullptr, 0,
            buf, buf,
            sizeof(tag), tag
        );

        if (ret != 0) {
            if (buf) { memset(buf, 0, plaintextLen); free(buf); }
            return false;
        }

        File file = fs.open(path.c_str(), "w");
        if (!file) {
            if (buf) { memset(buf, 0, plaintextLen); free(buf); }
            return false;
        }

        const uint8_t magic[4] = { 0x46, 0x46, 0x43, 0x45 };
        const uint8_t version  = 0x01;
        uint32_t len32 = (uint32_t)plaintextLen;

        bool ok = (file.write(magic, 4) == 4)
               && (file.write(&version, 1) == 1)
               && (file.write(nonce, sizeof(nonce)) == sizeof(nonce))
               && (file.write((uint8_t*)&len32, 4) == 4)
               && (file.write(tag, sizeof(tag)) == sizeof(tag))
               && (plaintextLen == 0 || file.write(buf, plaintextLen) == plaintextLen);

        file.close();

        if (buf) { memset(buf, 0, plaintextLen); free(buf); }

        if (!ok) {
            fs.remove(path.c_str());
            return false;
        }

        return true;
    }


    /**
     * Reads and AES-256-GCM decrypts the file at the given path into outPlaintext.
     * Returns false on I/O error, bad magic/version, length mismatch, or tag failure.
     */
    bool decryptFromFile(fs::FS& fs, const String& path, String& outPlaintext) {

        if (!_ready) return false;

        File file = fs.open(path.c_str(), "r");
        if (!file) return false;

        uint8_t magic[4];
        uint8_t version;
        uint8_t nonce[12];
        uint32_t plaintextLen;
        uint8_t tag[16];

        static const uint8_t kMagic[4] = { 0x46, 0x46, 0x43, 0x45 };

        bool headerOk =
            (file.read(magic, 4) == 4)                  &&
            (memcmp(magic, kMagic, 4) == 0)             &&
            (file.read(&version, 1) == 1)               &&
            (version == 0x01)                           &&
            (file.read(nonce, sizeof(nonce)) == sizeof(nonce)) &&
            (file.read((uint8_t*)&plaintextLen, 4) == 4)       &&
            (file.read(tag, sizeof(tag)) == sizeof(tag));

        if (!headerOk) {
            file.close();
            return false;
        }

        if ((size_t)(file.size() - file.position()) != plaintextLen) {
            file.close();
            return false;
        }

        uint8_t* buf = nullptr;
        if (plaintextLen > 0) {
            buf = (uint8_t*)(psramFound() ? ps_malloc(plaintextLen + 1)
                                          : malloc(plaintextLen + 1));
            if (!buf) {
                file.close();
                return false;
            }

            if (file.read(buf, plaintextLen) != plaintextLen) {
                memset(buf, 0, plaintextLen);
                free(buf);
                file.close();
                return false;
            }
        }

        file.close();

        if (plaintextLen == 0) {
            outPlaintext = String();
            return true;
        }

        int ret = mbedtls_gcm_auth_decrypt(
            &_ctx,
            plaintextLen,
            nonce, sizeof(nonce),
            nullptr, 0,
            tag, sizeof(tag),
            buf, buf
        );

        if (ret != 0) {
            memset(buf, 0, plaintextLen);
            free(buf);
            return false;
        }

        buf[plaintextLen] = '\0';
        outPlaintext = String((char*)buf);
        memset(buf, 0, plaintextLen);
        free(buf);

        return true;
    }


    /**
     * Encrypts a raw byte buffer for cloud backup using key_backup.
     * Returns an allocated buffer containing the full FFCE-format ciphertext blob.
     * The caller must free() the returned pointer.  outLen receives the total byte count.
     * Returns nullptr on failure.
     */
    uint8_t* encryptBackup(const uint8_t* plaintext, size_t plaintextLen, size_t& outLen) {

        if (!_ready) return nullptr;

        uint8_t nonce[12];
        esp_fill_random(nonce, sizeof(nonce));

        uint8_t tag[16];
        uint8_t* cipherBuf = nullptr;

        if (plaintextLen > 0) {
            cipherBuf = (uint8_t*)(psramFound() ? ps_malloc(plaintextLen) : malloc(plaintextLen));
            if (!cipherBuf) return nullptr;
            memcpy(cipherBuf, plaintext, plaintextLen);
        }

        int ret = mbedtls_gcm_crypt_and_tag(
            &_backupCtx, MBEDTLS_GCM_ENCRYPT,
            plaintextLen,
            nonce, sizeof(nonce),
            nullptr, 0,
            cipherBuf, cipherBuf,
            sizeof(tag), tag
        );

        if (ret != 0) {
            if (cipherBuf) { memset(cipherBuf, 0, plaintextLen); free(cipherBuf); }
            return nullptr;
        }

        // Assemble the FFCE blob: 37-byte header + ciphertext
        const size_t headerLen = 4 + 1 + 12 + 4 + 16; // = 37
        outLen = headerLen + plaintextLen;
        uint8_t* blob = (uint8_t*)(psramFound() ? ps_malloc(outLen) : malloc(outLen));
        if (!blob) {
            if (cipherBuf) { memset(cipherBuf, 0, plaintextLen); free(cipherBuf); }
            return nullptr;
        }

        const uint8_t magic[4] = { 0x46, 0x46, 0x43, 0x45 };
        const uint8_t version  = 0x01;
        uint32_t len32 = (uint32_t)plaintextLen;

        uint8_t* p = blob;
        memcpy(p, magic, 4);    p += 4;
        *p++ = version;
        memcpy(p, nonce, 12);   p += 12;
        memcpy(p, &len32, 4);   p += 4;
        memcpy(p, tag, 16);     p += 16;
        if (plaintextLen > 0) {
            memcpy(p, cipherBuf, plaintextLen);
        }

        if (cipherBuf) { memset(cipherBuf, 0, plaintextLen); free(cipherBuf); }

        return blob;
    }


    /**
     * Decrypts a FFCE-format cloud backup blob using key_backup.
     * outPlaintext receives the decrypted payload.
     * Returns false on any error (bad magic, tag mismatch, etc.).
     */
    bool decryptBackup(const uint8_t* blob, size_t blobLen, String& outPlaintext) {

        if (!_ready) return false;

        const size_t kHeaderLen = 4 + 1 + 12 + 4 + 16; // = 37
        if (blobLen < kHeaderLen) return false;

        const uint8_t kMagic[4] = { 0x46, 0x46, 0x43, 0x45 };
        if (memcmp(blob, kMagic, 4) != 0) return false;
        if (blob[4] != 0x01) return false;

        const uint8_t* nonce = blob + 5;
        uint32_t plaintextLen;
        memcpy(&plaintextLen, blob + 17, 4);
        const uint8_t* tag = blob + 21;
        const uint8_t* ciphertext = blob + kHeaderLen;

        if ((size_t)(blobLen - kHeaderLen) != plaintextLen) return false;

        if (plaintextLen == 0) {
            outPlaintext = String();
            return true;
        }

        uint8_t* buf = (uint8_t*)(psramFound() ? ps_malloc(plaintextLen + 1)
                                               : malloc(plaintextLen + 1));
        if (!buf) return false;
        memcpy(buf, ciphertext, plaintextLen);

        int ret = mbedtls_gcm_auth_decrypt(
            &_backupCtx,
            plaintextLen,
            nonce, 12,
            nullptr, 0,
            tag, 16,
            buf, buf
        );

        if (ret != 0) {
            memset(buf, 0, plaintextLen);
            free(buf);
            return false;
        }

        buf[plaintextLen] = '\0';
        outPlaintext = String((char*)buf);
        memset(buf, 0, plaintextLen);
        free(buf);

        return true;
    }


    bool isReady() const { return _ready; }


private:

    mbedtls_gcm_context _ctx;
    mbedtls_gcm_context _backupCtx;
    bool _ready = false;

};
