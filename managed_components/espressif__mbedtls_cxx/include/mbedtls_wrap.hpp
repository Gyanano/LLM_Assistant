/*
 * SPDX-FileCopyrightText: 2023-2024 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: Apache-2.0
 */
#pragma once

#include <utility>
#include <memory>
#include <mbedtls/timing.h>
#include <mbedtls/ssl_cookie.h>
#include "mbedtls/ssl.h"
#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/error.h"

namespace idf::mbedtls_cxx {

using const_buf = std::pair<const unsigned char *, std::size_t>;
using buf = std::pair<unsigned char *, std::size_t>;

struct TlsConfig {
    bool is_dtls;
    uint32_t timeout;
    const_buf client_id;
};

/**
 * @brief Application wrapper of (D)TLS for authentication and creating encrypted communication channels
 */
class Tls {
public:
    /**
     * High level configs for this class are per below: (server/client, with/out verification, TLS/DTLS)
     */
    enum class is_server : bool {
    };
    enum class do_verify : bool {
    };
    enum class is_dtls : bool {
    };

    Tls();

    virtual ~Tls();

    bool init(is_server server, do_verify verify, TlsConfig *config = nullptr);

    bool init_dtls_cookies();

    bool set_client_id();

    bool deinit();

    int handshake();

    int write(const unsigned char *buf, size_t len);

    int read(unsigned char *buf, size_t len);

    [[nodiscard]] bool set_own_cert(const_buf crt, const_buf key);

    [[nodiscard]] bool set_ca_cert(const_buf crt);

    bool set_hostname(const char *name);

    virtual int send(const unsigned char *buf, size_t len) = 0;

    virtual int recv(unsigned char *buf, size_t len) = 0;

    virtual int recv_timeout(unsigned char *buf, size_t len, int timeout)
    {
        return recv(buf, len);
    }

    size_t get_available_bytes();

protected:
    /**
     * mbedTLS internal structures (available after inheritance)
     */
    mbedtls_ssl_context ssl_{};
    mbedtls_x509_crt public_cert_{};
    mbedtls_pk_context pk_key_{};
    mbedtls_x509_crt ca_cert_{};
    mbedtls_ssl_config conf_{};
    mbedtls_ctr_drbg_context ctr_drbg_{};
    mbedtls_entropy_context entropy_{};
    mbedtls_timing_delay_context timer_{};
    mbedtls_ssl_cookie_ctx cookie_{};
    const_buf client_id_{};

    virtual void delay() {}

    bool is_server_{false};
    bool is_dtls_{false};

    bool set_session();

    bool get_session();

    void reset_session();

    bool is_session_loaded();

private:
    static void print_error(const char *function, int error_code);

    static int bio_write(void *ctx, const unsigned char *buf, size_t len);

    static int bio_read(void *ctx, unsigned char *buf, size_t len);

    static int bio_read_tout(void *ctx, unsigned char *buf, size_t len, uint32_t timeout);

    int mbedtls_pk_parse_key(mbedtls_pk_context *ctx,
                             const unsigned char *key, size_t keylen,
                             const unsigned char *pwd, size_t pwdlen);

    struct unique_session {
        unique_session()
        {
            ::mbedtls_ssl_session_init(&s);
        }

        ~unique_session()
        {
            ::mbedtls_ssl_session_free(&s);
        }

        mbedtls_ssl_session *ptr()
        {
            return &s;
        }

        mbedtls_ssl_session s;
    };

    std::unique_ptr<unique_session> session_;

};
}
