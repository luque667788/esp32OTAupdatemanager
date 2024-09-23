#ifndef GEN_AUTH_H
#define GEN_AUTH_H


#include "mbedtls/entropy.h"
#include "mbedtls/ctr_drbg.h"
#include "mbedtls/ecdsa.h"
#include "mbedtls/pem.h"
#include "mbedtls/x509_csr.h"

#include <stdio.h>
#include <string.h>
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "common.h"


// default values - > you shouldnt need to change this ones
#ifndef CSR_BUF_SIZE
    #define CSR_BUF_SIZE 2048
#endif

#ifndef KEY_BUF_SIZE
    #define KEY_BUF_SIZE 2048
#endif
#ifndef EXPONENT
    #define EXPONENT 65537
#endif

// Function to generate the private key and the CSR
// the output is a char* with the csr in PEM format and will be allocated on the HEAP (no need to allocate it before calling this function)
esp_err_t generate_auth_stuff( char **csr_buf,  char **key_buf);


/*
| PRIVATE HELPER FUNCTIONS | -> just for readability purposes I included them as comments in the header file

// Function to generate RSA key and convert it to PEM format
static int generate_rsa_key_pem(char **pem_out);

// Function to generate CSR from an RSA private key in PEM format
static int generate_csr_from_rsa_key(char *rsa_pem, char **csr_out);
*/
#endif 