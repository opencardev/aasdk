// This file is part of aasdk library project.
// Copyright (C) 2018 f1x.studio (Michal Szwaj)
// Copyright (C) 2024 CubeOne (Simon Dean - simon.dean@cubeone.co.uk)
// Copyright (C) 2024 OpenCarDev (Matthew Hilton - matthilton2005@gmail.com)
// Copyright (C) 2026 OpenCarDev (Matthew Hilton - matthilton2005@gmail.com)
//
// aasdk is free software: you can redistribute it and/or modify
// it under the terms of the GNU General Public License as published by
// the Free Software Foundation; either version 3 of the License, or
// (at your option) any later version.
//
// aasdk is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
// GNU General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with aasdk. If not, see <http://www.gnu.org/licenses/>.

#include <string>
#include <cerrno>
#include <cstring>
#include <mutex>
#if defined(__has_include) && __has_include(<openssl/engine.h>) && !defined(OPENSSL_NO_ENGINE)
#include <openssl/engine.h>
#define HAVE_ENGINE_H
#endif
#include <openssl/err.h>
#include <openssl/ssl.h>
#include <openssl/conf.h>
#include <aasdk/Transport/SSLWrapper.hpp>
#include <aasdk/Common/Log.hpp>
#include <aasdk/Common/ModernLogger.hpp>

namespace aasdk {
  namespace transport {

    namespace {
      std::once_flag gOpenSslInitOnce;
    }

    static auto sslErrorToString(int sslErrorCode) -> const char* {
      switch (sslErrorCode) {
        case SSL_ERROR_NONE:
          return "SSL_ERROR_NONE";
        case SSL_ERROR_SSL:
          return "SSL_ERROR_SSL";
        case SSL_ERROR_WANT_READ:
          return "SSL_ERROR_WANT_READ";
        case SSL_ERROR_WANT_WRITE:
          return "SSL_ERROR_WANT_WRITE";
        case SSL_ERROR_WANT_X509_LOOKUP:
          return "SSL_ERROR_WANT_X509_LOOKUP";
        case SSL_ERROR_SYSCALL:
          return "SSL_ERROR_SYSCALL";
        case SSL_ERROR_ZERO_RETURN:
          return "SSL_ERROR_ZERO_RETURN";
#if defined(SSL_ERROR_WANT_CONNECT)
        case SSL_ERROR_WANT_CONNECT:
          return "SSL_ERROR_WANT_CONNECT";
#endif
#if defined(SSL_ERROR_WANT_ACCEPT)
        case SSL_ERROR_WANT_ACCEPT:
          return "SSL_ERROR_WANT_ACCEPT";
#endif
        default:
          return "SSL_ERROR_UNKNOWN";
      }
    }

    SSLWrapper::SSLWrapper() {
      std::call_once(gOpenSslInitOnce, []() {
    #if (OPENSSL_VERSION_NUMBER < 0x10100000L)
        SSL_library_init();
        SSL_load_error_strings();
        OpenSSL_add_all_algorithms();
    #else
        OPENSSL_init_ssl(OPENSSL_INIT_LOAD_SSL_STRINGS | OPENSSL_INIT_LOAD_CRYPTO_STRINGS,
             nullptr);
    #endif
      });
    }

    SSLWrapper::~SSLWrapper() {
      // OpenSSL state is process-global. Do not tear it down per wrapper instance.
    }

    X509 *SSLWrapper::readCertificate(const std::string &certificate) {
      auto bio = BIO_new_mem_buf(certificate.c_str(), certificate.size());
      X509 *x509Certificate = PEM_read_bio_X509_AUX(bio, nullptr, nullptr, nullptr);
      BIO_free(bio);

      return x509Certificate;
    }

    EVP_PKEY *SSLWrapper::readPrivateKey(const std::string &privateKey) {
      auto bio = BIO_new_mem_buf(privateKey.c_str(), privateKey.size());
      auto result = PEM_read_bio_PrivateKey(bio, nullptr, nullptr, nullptr);
      BIO_free(bio);

      return result;
    }

    const SSL_METHOD *SSLWrapper::getMethod() {
#if (OPENSSL_VERSION_NUMBER < 0x10100000L)
      return TLSv1_2_client_method();
#else
      return TLS_client_method();
#endif
    }

    SSL_CTX *SSLWrapper::createContext(const SSL_METHOD *method) {
      SSL_CTX *ctx = SSL_CTX_new(method);
      /*
      if (ctx != nullptr) {
        // Configure for Android Auto compatibility with OpenSSL 3.x
        // Set minimum TLS version to 1.2 (Android Auto uses TLS 1.2+)
        SSL_CTX_set_min_proto_version(ctx, TLS1_2_VERSION);
        SSL_CTX_set_max_proto_version(ctx, TLS1_3_VERSION);
        
        // Lower security level for compatibility (Android Auto uses older ciphers)
        SSL_CTX_set_security_level(ctx, 1);
        
        // Set cipher list to include ciphers Android Auto expects
        SSL_CTX_set_cipher_list(ctx, "DEFAULT:@SECLEVEL=1");
        
        // Enable legacy renegotiation for older Android Auto versions
        SSL_CTX_set_options(ctx, SSL_OP_LEGACY_SERVER_CONNECT);
        
        AASDK_LOG(info) << "[SSLWrapper] SSL context configured for Android Auto (TLS 1.2-1.3, SECLEVEL=1)";
      }
      */
      return ctx;
    }

    bool SSLWrapper::useCertificate(SSL_CTX *context, X509 *certificate) {
      return SSL_CTX_use_certificate(context, certificate) == 1;
    }

    bool SSLWrapper::usePrivateKey(SSL_CTX *context, EVP_PKEY *privateKey) {
      return SSL_CTX_use_PrivateKey(context, privateKey) == 1;
    }

    SSL *SSLWrapper::createInstance(SSL_CTX *context) {
      return SSL_new(context);
    }

    bool SSLWrapper::checkPrivateKey(SSL *ssl) {
      return SSL_check_private_key(ssl) == 1;
    }

    std::pair<BIO *, BIO *> SSLWrapper::createBIOs() {
      auto readBIO = BIO_new(BIO_s_mem());
      auto writeBIO = BIO_new(BIO_s_mem());
      return {readBIO, writeBIO};
    }

    void SSLWrapper::setBIOs(SSL *ssl, const BIOs &bIOs, size_t maxBufferSize) {
      SSL_set_bio(ssl, bIOs.first, bIOs.second);
      BIO_set_write_buf_size(bIOs.first, maxBufferSize);
      BIO_set_write_buf_size(bIOs.second, maxBufferSize);
    }

    void SSLWrapper::setConnectState(SSL *ssl) {
      SSL_set_connect_state(ssl);
      SSL_set_verify(ssl, SSL_VERIFY_NONE, nullptr);
    }

    int SSLWrapper::doHandshake(SSL *ssl) {
      auto result = SSL_do_handshake(ssl);
      auto errorCode = SSL_get_error(ssl, result);

      return errorCode;
    }

    void SSLWrapper::free(SSL *ssl) {
      SSL_free(ssl);
    }

    void SSLWrapper::free(SSL_CTX *context) {
      SSL_CTX_free(context);
    }

    void SSLWrapper::free(BIO *bio) {
      BIO_free(bio);
    }

    void SSLWrapper::free(X509 *certificate) {
      X509_free(certificate);
    }

    void SSLWrapper::free(EVP_PKEY *privateKey) {
      EVP_PKEY_free(privateKey);
    }

    size_t SSLWrapper::bioCtrlPending(BIO *b) {
      return BIO_ctrl_pending(b);
    }

    int SSLWrapper::bioRead(BIO *b, void *data, int len) {
      return BIO_read(b, data, len);
    }

    int SSLWrapper::bioWrite(BIO *b, const void *data, int len) {
      return BIO_write(b, data, len);
    }

    int SSLWrapper::getAvailableBytes(const SSL *ssl) {
      return SSL_pending(ssl);
    }

    int SSLWrapper::sslRead(SSL *ssl, void *buf, int num) {
      return SSL_read(ssl, buf, num);
    }

    int SSLWrapper::sslWrite(SSL *ssl, const void *buf, int num) {
      return SSL_write(ssl, buf, num);
    }

    int SSLWrapper::getError(SSL *ssl, int returnCode) {
      const int sslErrorCode = SSL_get_error(ssl, returnCode);
      const int savedErrno = errno;
      const bool fatalError =
          sslErrorCode != SSL_ERROR_NONE &&
          sslErrorCode != SSL_ERROR_WANT_READ &&
          sslErrorCode != SSL_ERROR_WANT_WRITE &&
          sslErrorCode != SSL_ERROR_WANT_X509_LOOKUP;

      if (fatalError) {
        AASDK_LOG(error) << "[SSLWrapper] getError returnCode=" << returnCode
                         << " ssl_error=" << sslErrorCode
                         << "(" << sslErrorToString(sslErrorCode) << ")"
                         << " errno=" << savedErrno
                         << "(" << std::strerror(savedErrno) << ")"
                         << " state="
                         << (ssl ? SSL_state_string_long(ssl) : "<null-ssl>");
      } else {
        AASDK_LOG(debug) << "[SSLWrapper] getError returnCode=" << returnCode
                         << " ssl_error=" << sslErrorCode
                         << "(" << sslErrorToString(sslErrorCode) << ")"
                         << " errno=" << savedErrno
                         << "(" << std::strerror(savedErrno) << ")"
                         << " state="
                         << (ssl ? SSL_state_string_long(ssl) : "<null-ssl>");
      }

      if (fatalError) {
        while (auto err = ERR_get_error()) {
          AASDK_LOG(error) << "[SSLWrapper] SSL Error " << ERR_error_string(err, NULL);
        }
      }
      return sslErrorCode;
    }

  }
}
