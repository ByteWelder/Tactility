// SPDX-License-Identifier: Apache-2.0
#include <mbedtls/module.h>

#include <mbedtls/ctr_drbg.h>
#include <mbedtls/entropy.h>
#include <mbedtls/cipher.h>
#include <mbedtls/md.h>
#include <mbedtls/bignum.h>
#include <mbedtls/rsa.h>
#include <mbedtls/pk.h>
#include <mbedtls/ecp.h>
#include <mbedtls/ecdsa.h>
#include <mbedtls/ecdh.h>
#include <mbedtls/error.h>

extern "C" {

static const ModuleSymbol mbedtls_module_symbols[] = {
    // CTR_DRBG (random number generation)
    DEFINE_MODULE_SYMBOL(mbedtls_ctr_drbg_init),
    DEFINE_MODULE_SYMBOL(mbedtls_ctr_drbg_free),
    DEFINE_MODULE_SYMBOL(mbedtls_ctr_drbg_seed),
    DEFINE_MODULE_SYMBOL(mbedtls_ctr_drbg_random),
    // Entropy
    DEFINE_MODULE_SYMBOL(mbedtls_entropy_init),
    DEFINE_MODULE_SYMBOL(mbedtls_entropy_free),
    DEFINE_MODULE_SYMBOL(mbedtls_entropy_func),
    // Cipher
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_init),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_free),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_setup),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_setkey),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_set_iv),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_reset),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_update),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_finish),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_get_block_size),
    DEFINE_MODULE_SYMBOL(mbedtls_cipher_info_from_type),
    // Message digest / HMAC
    DEFINE_MODULE_SYMBOL(mbedtls_md),
    DEFINE_MODULE_SYMBOL(mbedtls_md_init),
    DEFINE_MODULE_SYMBOL(mbedtls_md_free),
    DEFINE_MODULE_SYMBOL(mbedtls_md_setup),
    DEFINE_MODULE_SYMBOL(mbedtls_md_starts),
    DEFINE_MODULE_SYMBOL(mbedtls_md_update),
    DEFINE_MODULE_SYMBOL(mbedtls_md_finish),
    DEFINE_MODULE_SYMBOL(mbedtls_md_hmac_starts),
    DEFINE_MODULE_SYMBOL(mbedtls_md_hmac_update),
    DEFINE_MODULE_SYMBOL(mbedtls_md_hmac_finish),
    DEFINE_MODULE_SYMBOL(mbedtls_md_info_from_type),
    // Bignum (MPI)
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_init),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_free),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_read_binary),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_write_binary),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_size),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_bitlen),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_lset),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_set_bit),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_fill_random),
    DEFINE_MODULE_SYMBOL(mbedtls_mpi_exp_mod),
    // RSA
    DEFINE_MODULE_SYMBOL(mbedtls_rsa_init),
    DEFINE_MODULE_SYMBOL(mbedtls_rsa_free),
    DEFINE_MODULE_SYMBOL(mbedtls_rsa_copy),
    DEFINE_MODULE_SYMBOL(mbedtls_rsa_get_len),
    DEFINE_MODULE_SYMBOL(mbedtls_rsa_check_pubkey),
    DEFINE_MODULE_SYMBOL(mbedtls_rsa_check_privkey),
    DEFINE_MODULE_SYMBOL(mbedtls_rsa_pkcs1_sign),
    DEFINE_MODULE_SYMBOL(mbedtls_rsa_pkcs1_verify),
    // Public key abstraction
    DEFINE_MODULE_SYMBOL(mbedtls_pk_init),
    DEFINE_MODULE_SYMBOL(mbedtls_pk_free),
    DEFINE_MODULE_SYMBOL(mbedtls_pk_get_type),
    DEFINE_MODULE_SYMBOL(mbedtls_pk_parse_key),
    DEFINE_MODULE_SYMBOL(mbedtls_pk_parse_keyfile),
    // ECP (elliptic curves)
    DEFINE_MODULE_SYMBOL(mbedtls_ecp_group_load),
    DEFINE_MODULE_SYMBOL(mbedtls_ecp_point_init),
    DEFINE_MODULE_SYMBOL(mbedtls_ecp_point_free),
    DEFINE_MODULE_SYMBOL(mbedtls_ecp_point_read_binary),
    DEFINE_MODULE_SYMBOL(mbedtls_ecp_point_write_binary),
    DEFINE_MODULE_SYMBOL(mbedtls_ecp_check_pubkey),
    DEFINE_MODULE_SYMBOL(mbedtls_ecp_check_privkey),
    DEFINE_MODULE_SYMBOL(mbedtls_ecp_mul),
    // ECDSA
    DEFINE_MODULE_SYMBOL(mbedtls_ecdsa_init),
    DEFINE_MODULE_SYMBOL(mbedtls_ecdsa_free),
    DEFINE_MODULE_SYMBOL(mbedtls_ecdsa_genkey),
    DEFINE_MODULE_SYMBOL(mbedtls_ecdsa_from_keypair),
    DEFINE_MODULE_SYMBOL(mbedtls_ecdsa_sign),
    DEFINE_MODULE_SYMBOL(mbedtls_ecdsa_verify),
    // ECDH
    DEFINE_MODULE_SYMBOL(mbedtls_ecdh_compute_shared),
    // Error strings
    DEFINE_MODULE_SYMBOL(mbedtls_strerror),
    MODULE_SYMBOL_TERMINATOR
};

Module mbedtls_module = {
    .name = "mbedtls",
    .symbols = mbedtls_module_symbols
};

}
