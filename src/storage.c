#include "storage.h"

#include <string.h>
#include <sodium/core.h>
#include <sodium/randombytes.h>
#include <sodium/crypto_pwhash_argon2i.h>
#include <sodium/crypto_stream_chacha20.h>
#include <sodium/crypto_auth_hmacsha256.h>
#include <sodium/crypto_generichash_blake2b.h>
#include <sodium/utils.h>

#include <stdio.h>
#include <assert.h>

static void key_store_empty(key_store * k) {
  randombytes(k->in, sizeof(k->in));
}

static void aont_key_empty(aont_key * a) {
  randombytes((void*)a, sizeof(aont_key));
}

static void challenge_new_nonce(challenge * c) {
  randombytes((void*)c, sizeof(c->pw_nonce) + sizeof(c->tf_nonce));
}

void challenge_empty(challenge * c) {
  challenge_new_nonce(c);
  randombytes((void*)c, sizeof(c->nonce));
  key_store_empty(&c->keys);
  aont_key_empty(&c->aont);
  debugprint("challenge_empty created:", challenge, *c);
}

void challenge_new_random_tf_key(challenge * c) {
  challenge_empty(c);
  randombytes((void*)c->keys.tf, sizeof(c->keys.tf));
  debugprint("\nchallenge_new_with_tf_key created:", challenge, *c);
}

void challenge_new_with_tf_key(challenge * c, const uint8_t * k) {
  challenge_empty(c);
  memcpy(&c->keys.tf, k, sizeof(c->keys.tf));
  debugprint("\nchallenge_new_with_tf_key created:", challenge, *c);
}

#define ARGON2_LEN_CHACHA (crypto_stream_chacha20_KEYBYTES + crypto_stream_chacha20_NONCEBYTES)

void challenge_encode(challenge * c, const uint8_t * pw, uint64_t pwlen) {
  // TODO: check various return codes
  uint8_t tf_key[64] = {0};
  uint8_t tempkey[ARGON2_LEN_CHACHA] = {0};
  uint8_t blind[sizeof(aont_key)] = {0};
  uint8_t tf_resp[crypto_auth_hmacsha256_BYTES] = {0};
  int ret;

  memcpy(tf_key, c->keys.tf, 64); // save the 2f key

  // encrypt the inner AONT
  ret = crypto_pwhash_argon2i((uint8_t*)tempkey, ARGON2_LEN_CHACHA, (char*)c->aont.input, sizeof(c->aont.input), (uint8_t*)&c->aont.nonce,
                              8ULL, 33554432ULL, crypto_pwhash_argon2i_ALG_ARGON2I13);
  crypto_stream_chacha20_xor(c->keys.b, c->keys.b, sizeof(key_store), &tempkey[crypto_stream_chacha20_KEYBYTES], (uint8_t*)&tempkey);
  ret = crypto_generichash_blake2b(blind, sizeof(blind), c->keys.b, sizeof(c->keys), c->pw_nonce, 32);
  for (uint64_t *p = (uint64_t*) blind, *q = (uint64_t*) c->aont.b; p < (uint64_t*)(blind+sizeof(blind)); ++p, ++q) *q ^= *p;

  // encrypt with the 2f challenge response
  ret = crypto_auth_hmacsha256(tf_resp, c->tf_nonce, 16, tf_key);
  ret = crypto_pwhash_argon2i((uint8_t*)tempkey, ARGON2_LEN_CHACHA, (char*)tf_resp, crypto_auth_hmacsha256_BYTES, c->tf_nonce,
                              8ULL, 33554432ULL, crypto_pwhash_argon2i_ALG_ARGON2I13);
  crypto_stream_chacha20_xor(c->keys.b, c->keys.b, sizeof(key_store) + sizeof(aont_key), &tempkey[crypto_stream_chacha20_KEYBYTES], (uint8_t*)&tempkey);

  debugprint("\nencode tf_resp:", uint8_t[crypto_auth_hmacsha256_BYTES], tf_resp);

  // encrypt with the pw hash
  ret = crypto_pwhash_argon2i((uint8_t*)tempkey, ARGON2_LEN_CHACHA, (char*)pw, pwlen, c->pw_nonce,
                              8ULL, 33554432ULL, crypto_pwhash_argon2i_ALG_ARGON2I13);
  crypto_stream_chacha20_xor(c->keys.tf, c->keys.tf, sizeof(key_store) + sizeof(aont_key), &tempkey[crypto_stream_chacha20_KEYBYTES], (uint8_t*)&tempkey);

  sodium_memzero(tf_key , sizeof(tf_key ));
  sodium_memzero(tf_resp, sizeof(tf_resp));
  sodium_memzero(tempkey, sizeof(tempkey));

  debugprint("\nencode result:", challenge, *c);
}

void challenge_decode(challenge * c, const uint8_t * pw, uint64_t pwlen, uint8_t * tf_resp, uint64_t resplen) {
  uint8_t tempkey[ARGON2_LEN_CHACHA] = {0};
  uint8_t blind[sizeof(aont_key)] = {0};
  int ret;

  debugprint("\ndecode tf_resp:", uint8_t[32], *tf_resp);

  // decrypt the pw hash
  ret = crypto_pwhash_argon2i((uint8_t*)tempkey, ARGON2_LEN_CHACHA, (char*)pw, pwlen, c->pw_nonce,
                              8ULL, 33554432ULL, crypto_pwhash_argon2i_ALG_ARGON2I13);
  crypto_stream_chacha20_xor(c->keys.b, c->keys.b, sizeof(key_store) + sizeof(aont_key), &tempkey[crypto_stream_chacha20_KEYBYTES], (uint8_t*)&tempkey);

  // decrypt the 2f challange response
  ret = crypto_pwhash_argon2i((uint8_t*)tempkey, ARGON2_LEN_CHACHA, (char*)tf_resp, resplen, c->tf_nonce,
                              8ULL, 33554432ULL, crypto_pwhash_argon2i_ALG_ARGON2I13);
  crypto_stream_chacha20_xor(c->keys.b, c->keys.b, sizeof(key_store) + sizeof(aont_key), &tempkey[crypto_stream_chacha20_KEYBYTES], (uint8_t*)&tempkey);

  // decrypt the inner AONT
  ret = crypto_generichash_blake2b(blind, sizeof(blind), c->keys.b, sizeof(c->keys), c->pw_nonce, 32);
  for (uint64_t *p = (uint64_t*) blind, *q = (uint64_t*) c->aont.b; p < (uint64_t*)(blind+sizeof(blind)); ++p, ++q) *q ^= *p;
  ret = crypto_pwhash_argon2i((uint8_t*)tempkey, ARGON2_LEN_CHACHA, (char*)c->aont.input, sizeof(c->aont.input), (uint8_t*)&c->aont.nonce,
                              8ULL, 33554432ULL, crypto_pwhash_argon2i_ALG_ARGON2I13);
  crypto_stream_chacha20_xor(c->keys.b, c->keys.b, sizeof(key_store), &tempkey[crypto_stream_chacha20_KEYBYTES], (uint8_t*)&tempkey);

  sodium_memzero(tempkey, sizeof(tempkey));
  debugprint("\ndecode result:", challenge, *c);
}

static int challenge_test(void) {
  static uint8_t * pw = (uint8_t*) "whowhatwhere";
  static uint64_t pwlen = 0;
  static uint8_t tf_key[65];
  uint8_t tf_resp[crypto_auth_hmacsha256_BYTES] __attribute__((__aligned__((32)))) = {0};

  if (!pwlen || tf_key[64] == 0) {
    pwlen = strlen((char*)pw);
    randombytes(tf_key, 64);
    tf_key[64] = 1;
  }

  challenge c, c1;
  challenge_new_with_tf_key(&c, tf_key);
  memcpy(&c1, &c, sizeof(challenge));

  challenge_encode(&c, pw, pwlen);

  crypto_auth_hmacsha256(tf_resp, c.tf_nonce, 16, tf_key);

  challenge_decode(&c, pw, pwlen, tf_resp, crypto_auth_hmacsha256_BYTES);

  if (memcmp(&c, &c1, sizeof(challenge)) != 0) {
    #ifdef DEBUG_PRINT
    printf("\nFailed!\n\n");
    #endif
    debugprint("\ntf_resp:", uint8_t[crypto_auth_hmacsha256_BYTES], tf_resp);
    debugprint("\nc1:",challenge, c1);
    return 0;
  }
  else {
    #ifdef DEBUG_PRINT
    printf("\nSuccess!\n");
    #endif
    return 0;
  }
  return 1;
}

#ifdef RUN_TESTS
int main(void) {
  // Sanity check #pragma pack()
  assert(sizeof(challenge) == 32+sizeof(key_store)+sizeof(aont_key));
  // encode and decode using same material
  assert((sodium_init() || challenge_test()) == 0);

  return 0;
}
#endif