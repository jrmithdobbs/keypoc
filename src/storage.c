#include "storage.h"
#include "modauth.h"

/*
static void key_store_empty(key_store * k) {
  randombytes(k->b, sizeof(k->b));
}
*/

static void key_store_empty_tf(key_store * k) {
  randombytes(k->tf, sizeof(k->tf));
}

static void key_store_empty_in(key_store * k) {
  randombytes(k->in, sizeof(k->in));
}

static void aont_key_empty(aont_key * a) {
  randombytes(a->b, sizeof(a->b));
}

static void challenge_new_nonce(challenge * c) {
  randombytes(c->nonce, sizeof(c->nonce));
}

static void challenge_empty(challenge * c) {
  sodium_memzero(c, sizeof(challenge));
  challenge_new_nonce(c);
  key_store_empty_in(&c->keys);
  aont_key_empty(&c->aont);
  debugprint("challenge_empty created:", challenge, *c);
}

static void set_auth_key(challenge *c, challenge_plugin_hdr *p) {
  if (sizeof(c->keys.tf) > p->key_max) {
    uint8_t min[p->key_max];
    memset(min, 0, p->key_max);
    crypto_generichash_blake2b(
      min,                // out
      p->key_max,         // outlen
      c->keys.tf,         // in
      sizeof(c->keys.tf), // inlen
      NULL,               // key
      0);                 // keylen
    p->setkey(min, p->key_max);
  } else {
    p->setkey(c->keys.tf, sizeof(c->keys.tf));
  }
}

void challenge_new_random_tf_key(challenge * c, challenge_plugin_hdr *p) {
  challenge_empty(c);
  key_store_empty_tf(&c->keys);
  set_auth_key(c, p);
  p->savekey();
  debugprint("\nchallenge_new_with_tf_key created:", challenge, *c);
}

void challenge_new_with_tf_key(challenge * c,
                              challenge_plugin_hdr *p,
                              const uint8_t * k) {
  challenge_empty(c);
  memcpy(&c->keys.tf, k, sizeof(c->keys.tf));
  set_auth_key(c, p);
  debugprint("\nchallenge_new_with_tf_key created:", challenge, *c);
}

#define ARGON2_LEN_CHACHA \
  (crypto_stream_chacha20_KEYBYTES + crypto_stream_chacha20_NONCEBYTES)

#define ARGON2_CHOSEN_OPTS \
  8ULL /* opslimit */,\
  33554432ULL /* memlimit */,\
  crypto_pwhash_argon2i_ALG_ARGON2I13 /* alg */

typedef struct keybuf {
  union {
    struct {
      uint8_t k[crypto_stream_chacha20_KEYBYTES];
      uint8_t n[crypto_stream_chacha20_NONCEBYTES];
    };
    uint8_t b[ARGON2_LEN_CHACHA];
  };
} keybuf __attribute__((__aligned__(32)));

void challenge_encode(challenge * c,
                      challenge_plugin_hdr *p,
                      const uint8_t * pw,
                      uint64_t pwlen) {

  keybuf  tempkey = {0};
  uint8_t blind[sizeof(aont_key)] = {0};
  uint8_t tf_resp[p->out_max];
  int ret; // TODO: validate argon2i return codes

  sodium_memzero(tf_resp, sizeof(tf_resp));

  // encrypt the inner AONT
  ret = crypto_pwhash_argon2i(
    tempkey.b,                // out
    ARGON2_LEN_CHACHA,        // outlen
    (char*)c->aont.input,     // passwd
    sizeof(c->aont.input),    // passwdlen
    (uint8_t*)&c->aont.nonce, // salt
    ARGON2_CHOSEN_OPTS);

  crypto_stream_chacha20_xor(
    c->keys.b,          // c
    c->keys.b,          // m
    sizeof(c->keys.b),  // mlen
    tempkey.n,          // n
    tempkey.k);         // k

  crypto_generichash_blake2b(
    blind,              // out
    sizeof(blind),      // outlen
    c->keys.b,          // in
    sizeof(c->keys.b),  // inlen
    c->pw_nonce,        // key
    sizeof(blind));     // keylen

  for (uint64_t *p = (uint64_t*) blind, *q = (uint64_t*) c->aont.b;
        p < (uint64_t*)(blind+sizeof(blind)); ++p, ++q) {
    *q ^= *p;
  }

  // encrypt with the 2f challenge response
  p->challenge(c->tf_nonce, 16, tf_resp, p->out_max);

  ret = crypto_pwhash_argon2i(
    tempkey.b,          // out
    ARGON2_LEN_CHACHA,  // outlen
    (char*)tf_resp,     // passwd
    p->out_max,         // passwdlen
    c->tf_nonce,        // salt
    ARGON2_CHOSEN_OPTS);

  crypto_stream_chacha20_xor(
    c->aont.b,                              // c
    c->aont.b,                              // m
    sizeof(c->aont.b) + sizeof(c->keys.b),  // mlen
    tempkey.n,                              // n
    tempkey.k);                             // k

  debugprint("\nencode tf_resp:", tf_resp, tf_resp);

  // encrypt with the pw hash
  ret = crypto_pwhash_argon2i(
    tempkey.b,          // out
    ARGON2_LEN_CHACHA,  // outlen
    (char*)pw,          // passwd
    pwlen,              // passwdlen
    c->pw_nonce,        // salt
    ARGON2_CHOSEN_OPTS);

  crypto_stream_chacha20_xor(
    c->aont.b,                              // c
    c->aont.b,                              // m
    sizeof(c->aont.b) + sizeof(c->keys.b),  // mlen
    tempkey.n,                              // n
    tempkey.k);                             // k

  sodium_memzero(tf_resp, sizeof(tf_resp));
  sodium_memzero(&tempkey, sizeof(tempkey));

  debugprint("\nencode result:", challenge, *c);
}

void challenge_decode(challenge * c,
                      challenge_plugin_hdr *p,
                      const uint8_t * pw,
                      uint64_t pwlen) {

  keybuf tempkey = {0};
  uint8_t blind[sizeof(aont_key)] = {0};
  uint8_t tf_resp[p->out_max];
  uint64_t resplen = p->out_max;
  int ret; // TODO: validate argon2i return codes

  p->challenge(c->tf_nonce, 16, tf_resp, p->out_max);
  debugprint("\ndecode tf_resp:", tf_resp, tf_resp);

  // decrypt the pw hash
  ret = crypto_pwhash_argon2i(
    tempkey.b,          // out
    ARGON2_LEN_CHACHA,  // outlen
    (char*)pw,          // passwd
    pwlen,              // passwdlen
    c->pw_nonce,        // salt
    ARGON2_CHOSEN_OPTS);

  ret = crypto_stream_chacha20_xor(
    c->aont.b,                              // c
    c->aont.b,                              // m
    sizeof(c->aont.b) + sizeof(c->keys.b),  // mlen
    tempkey.n,                              // n
    tempkey.k);                             // k

  // decrypt the 2f challange response
  ret = crypto_pwhash_argon2i(
    tempkey.b,          // out
    ARGON2_LEN_CHACHA,  // outlen
    (char*)tf_resp,     // passwd
    resplen,            // passwdlen
    c->tf_nonce,        // salt
    ARGON2_CHOSEN_OPTS);

  ret = crypto_stream_chacha20_xor(
    c->aont.b,                              // c
    c->aont.b,                              // m
    sizeof(c->aont.b) + sizeof(c->keys.b),  // mlen
    tempkey.n,                              // n
    tempkey.k);                             // k

  // decrypt the inner AONT
  ret = crypto_generichash_blake2b(
    blind,              // out
    sizeof(blind),      // outlen
    c->keys.b,          // in
    sizeof(c->keys.b),  // inlen
    c->pw_nonce,        // key
    sizeof(blind));     // keylen

  for (uint64_t *p = (uint64_t*) blind, *q = (uint64_t*) c->aont.b;
      p < (uint64_t*)(blind+sizeof(blind)); ++p, ++q) {
    *q ^= *p;
  }

  ret = crypto_pwhash_argon2i(
    tempkey.b,                // out
    ARGON2_LEN_CHACHA,        // outlen
    (char*)c->aont.input,     // passwd
    sizeof(c->aont.input),    // passwdlen
    (uint8_t*)&c->aont.nonce, // salt
    ARGON2_CHOSEN_OPTS);

  ret = crypto_stream_chacha20_xor(
    c->keys.b,          // c
    c->keys.b,          // m
    sizeof(c->keys.b),  // mlen
    tempkey.n,          // n
    tempkey.k);         // k

  sodium_memzero(&tempkey, sizeof(tempkey));
  debugprint("\ndecode result:", challenge, *c);
}
