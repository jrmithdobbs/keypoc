#include "src/storage.c"

static int challenge_test(void) {
  static uint8_t * pw = (uint8_t*) "whowhatwhere";
  static uint64_t pwlen = 0;
  static uint8_t tf_key[32];
  uint8_t tf_resp[crypto_auth_hmacsha256_BYTES] __attribute__((__aligned__((32)))) = {0};

  pwlen = strlen((char*)pw);
  randombytes(tf_key, 32);

  challenge_plugin_hdr *sha2 = load_challenge_plugin("hmacsha2mem", NULL);
  if (sha2 == NULL) {
    printf("Error loading module.");
    return 1;
  }
  sha2->setkey(tf_key, 64);

  challenge c, c1;
  challenge_new_with_tf_key(&c, sha2, tf_key);
  memcpy(&c1, &c, sizeof(challenge));

  challenge_encode(&c, sha2, pw, pwlen);

  sha2->challenge(c.tf_nonce, 16, tf_resp, sizeof(tf_resp));

  challenge_decode(&c, sha2, pw, pwlen);

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
    printf("\n");
    #endif
    printf("Success!\n");
    return 0;
  }
  return 1;
}

int main(void) {
  // Ensure basic #pragma pack(1) assumptions are met.
  bool challenge_size_matches_keystore_plus_aont_key =
    sizeof(challenge) == 32+sizeof(key_store)+sizeof(aont_key);
  assert(challenge_size_matches_keystore_plus_aont_key);

  // Ensure absolute sizing assumptions are met.
  bool challenge_size_is_128b =
    sizeof(challenge) == 128;
  assert(challenge_size_is_128b); // Ensure 128 byte size

  // encode and decode using same material
  return ! ((
    #ifndef DEBUG_PRINT
      printf("storage:") < 0 ||
    #endif
    sodium_init() || challenge_test()) == 0);
}
