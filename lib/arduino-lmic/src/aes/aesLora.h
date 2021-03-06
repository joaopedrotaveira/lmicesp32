
#ifndef __aesLora_h__
#define __aesLora_h__
#include "../lmic/config.h"
#include "mbedtls/aes.h"

#include <stdint.h>
#include "../lmic/lorabase.h"

// ======================================================================
// AES support

void lmic_aes_encrypt(uint8_t *data, const uint8_t *key);

#define AES_BLCK_SIZE 16

class AesLora {
private:
  mbedtls_aes_context AESDevCtx;
  // network session key
  mbedtls_aes_context nwkSCtx;
  // application session key
  mbedtls_aes_context appSCtx;

  void micB0(uint32_t devaddr, uint32_t seqno, PktDir dndir,
                    uint8_t len, uint8_t buf[16]);
  void aes_cmac(const uint8_t *buf, uint8_t len, bool prepend_aux,
                       esp_aes_context *ctx, uint8_t result[16]);

public:
  AesLora();
  ~AesLora();
  size_t saveState(uint8_t* buffer);
  size_t loadState(uint8_t* buffer);
  /* Set device key
   * Key is copied.
   */
  void setDevKey(uint8_t key[16]);
  void setNetworkSessionKey(uint8_t key[16]);
  void setApplicationSessionKey(uint8_t key[16]);
  bool verifyMic(uint32_t devaddr, uint32_t seqno, PktDir dndir, uint8_t *pdu,
                 uint8_t len) ;
  bool verifyMic0(uint8_t *pdu, uint8_t len) ;
  void framePayloadEncryption(uint8_t port, uint32_t devaddr, uint32_t seqno,
                              PktDir dndir, uint8_t *payload,
                              uint8_t len) ;
  void encrypt(uint8_t *pdu, uint8_t len) ;
  void sessKeys(uint16_t devnonce, const uint8_t *artnonce);
  void appendMic(uint32_t devaddr, uint32_t seqno, PktDir dndir, uint8_t *pdu,
                 uint8_t len) ;
  void appendMic0(uint8_t *pdu, uint8_t len) ;
};

#endif // __aes_h__