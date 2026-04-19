#pragma once
#include <Arduino.h>
#include <FS.h>
#include <mbedtls/gcm.h>
#include <mbedtls/hkdf.h>
#include <mbedtls/md.h>
#include <esp_random.h>

/**
 * AES-256-GCM file encryption using a key derived from the eFuse master secret.
 *
 * Binary file format (37-byte header):
 *   Offset  0 :  4 bytes  magic    = { 0x46, 0x46, 0x43, 0x45 } ("FFCE")
 *   Offset  4 :  1 byte   version  = 0x01
 *   Offset  5 : 12 bytes  nonce    (random per write)
 *   Offset 17 :  4 bytes  plaintext_len (uint32_t, little-endian)
 *   Offset 21 : 16 bytes  GCM authentication tag
 *   Offset 37 :  N bytes  ciphertext (N == plaintext_len)
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

        uint8_t derived[32];
        static const uint8_t kInfo[] = "firefly-configfs-v1";

        int ret = mbedtls_hkdf(
            mbedtls_md_info_from_type(MBEDTLS_MD_SHA256),
            nullptr, 0,
            masterKey, masterKeyLen,
            kInfo, sizeof(kInfo) - 1,
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


    bool isReady() const { return _ready; }


private:

    mbedtls_gcm_context _ctx;
    bool _ready = false;

};
