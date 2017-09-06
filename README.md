This is a simple POC for storing a long term crypto-graphic key in such a way
that authentication factors can be changed without ever revealing them to a
third party. Initially this is just as simple unit test that will build into
./tests/storage and do an encrypt and decrypt operation and show that the
original value is recovered.

A server could store a derived public key along-side this blob and require that
the client mutate on every authentication to prevent replayability with no
extra storage burden.
