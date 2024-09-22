#include "gen_auth.h"

// Function to generate RSA key and convert to PEM format
// the output is a char* with the key in PEM format and will be allocated on the HEAP
static int generate_rsa_key_pem(char **pem_out);

// Function to generate CSR from an RSA private key in PEM format
// the output is a char* with the csr in PEM format and will be allocated on the HEAP
static int generate_csr_from_rsa_key( char *rsa_pem,  char **csr_out);

esp_err_t generate_auth_stuff( char **csr_buf,  char **key_buf)
{
    esp_err_t toReturn;
    int err = 1;
    err = generate_rsa_key_pem(key_buf);


    if (csr_buf && key_buf && err == 0)
    {
        err = generate_csr_from_rsa_key(*key_buf, csr_buf);
        if (err == 0)
        {
            toReturn = ESP_OK;
        }
        else
        {
            ESP_LOGE(TAG, "Failed to generate CSR");
            toReturn = ESP_FAIL;
        }
        
        
    }
    else
    {
        ESP_LOGE(TAG, "Failed to generate private key");
        // TODO! handle error better maybe
        toReturn = ESP_FAIL;
    }
    return toReturn;
}



// Function to generate RSA key and convert to PEM format
// the output is a char* with the key in PEM format and will be allocated on the HEAP
static int generate_rsa_key_pem(char **pem_out)
{
    mbedtls_pk_context pk;
    mbedtls_entropy_context entropy;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_mpi N, P, Q, D, E, DP, DQ, QP;

    int ret;

    const char *pers = "rsa_genkey";

    mbedtls_ctr_drbg_init(&ctr_drbg);

    mbedtls_pk_init(&pk);
    mbedtls_mpi_init(&N);
    mbedtls_mpi_init(&P);
    mbedtls_mpi_init(&Q);
    mbedtls_mpi_init(&D);
    mbedtls_mpi_init(&E);
    mbedtls_mpi_init(&DP);
    mbedtls_mpi_init(&DQ);
    mbedtls_mpi_init(&QP);

    ESP_LOGI(TAG, "\n  . Seeding the random number generator...");
    mbedtls_entropy_init(&entropy);
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy, (const unsigned char *)pers, strlen(pers))) != 0)
    {
        ESP_LOGI(TAG, " failed\n  ! mbedtls_ctr_drbg_seed returned %d\n", ret);
        goto cleanup;
    }

    if ((ret = mbedtls_pk_setup(&pk, mbedtls_pk_info_from_type(MBEDTLS_PK_RSA))) != 0)
    {
        ESP_LOGI(TAG, "pk_setup failed: %i\n", ret);
        goto cleanup;
    }

    ESP_LOGI(TAG, " ok\n  . Generating the RSA key [ %d-bit ]...", KEY_BUF_SIZE);
    if ((ret = mbedtls_rsa_gen_key(mbedtls_pk_rsa(pk), mbedtls_ctr_drbg_random, &ctr_drbg, KEY_BUF_SIZE, EXPONENT)) != 0)
    {
        ESP_LOGI(TAG, " failed\n  ! mbedtls_rsa_gen_key returned %d\n\n", ret);
        goto cleanup;
    }

    if (mbedtls_rsa_check_privkey(mbedtls_pk_rsa(pk)) != 0)
    {
        ESP_LOGI(TAG, "RSA context does not contain an rsa private key");
        goto cleanup;
    }
    ESP_LOGI(TAG, " ok\n  . Exporting the RSA key in PEM format...");
    unsigned char *privKeyPem = (unsigned char *)malloc(KEY_BUF_SIZE);
    memset(privKeyPem, 0, KEY_BUF_SIZE);
    ret = mbedtls_pk_write_key_pem(&pk, privKeyPem, KEY_BUF_SIZE);
    if (ret != 0)
    {
        ESP_LOGI(TAG, "write private key to string failed with code %04x\n", ret);
        goto cleanup;
    }
    *pem_out = (char*)privKeyPem;

    ESP_LOGI(TAG,"FINISH Successfully generated RSA key in PEM format.");

cleanup:
    mbedtls_mpi_free(&N);
    mbedtls_mpi_free(&P);
    mbedtls_mpi_free(&Q);
    mbedtls_mpi_free(&D);
    mbedtls_mpi_free(&E);
    mbedtls_mpi_free(&DP);
    mbedtls_mpi_free(&DQ);
    mbedtls_mpi_free(&QP);
    mbedtls_pk_free(&pk);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);
    return ret;
}


// Function to generate CSR from an RSA private key in PEM format
// the output is a char* with the csr in PEM format and will be allocated on the HEAP
static int generate_csr_from_rsa_key( char *rsa_pem,  char **csr_out)
{
    mbedtls_x509write_csr req;
    mbedtls_pk_context key;
    mbedtls_ctr_drbg_context ctr_drbg;
    mbedtls_entropy_context entropy;
    int ret = 0;

    // Initialize the necessary contexts
    mbedtls_x509write_csr_init(&req);
    mbedtls_pk_init(&key);
    mbedtls_ctr_drbg_init(&ctr_drbg);
    mbedtls_entropy_init(&entropy);

    // Seed the random number generator
    const char *personalization = "csr_gen";
    if ((ret = mbedtls_ctr_drbg_seed(&ctr_drbg, mbedtls_entropy_func, &entropy,
                                     (const unsigned char *)personalization, strlen(personalization))) != 0)
    {
        ESP_LOGE(TAG, "Failed to seed the random generator: -0x%x", -ret);
        goto cleanup;
    }

    // Parse the private key
    if (mbedtls_pk_parse_key(&key, (const unsigned char *)rsa_pem, strlen(rsa_pem) + 1,
                             NULL, 0, mbedtls_ctr_drbg_random, &ctr_drbg) != 0)
    {
        
        ESP_LOGE(TAG, "Failed to parse RSA key: -0x%x", -ret);
        goto cleanup;
        // Handle error
    }

    // Set the private key to the CSR object
    mbedtls_x509write_csr_set_key(&req, &key);

    // Set the subject name for the CSR
    // Example: "CN=esp32,O=example,C=US"
    const char *subject_name = "CN=esp32,O=example,C=US";
    if ((ret = mbedtls_x509write_csr_set_subject_name(&req, subject_name)) != 0)
    {
        ESP_LOGE(TAG, "Failed to set subject name: -0x%x", -ret);
        goto cleanup;
    }

    // Set the MD algorithm to use (e.g., SHA-256)
    mbedtls_x509write_csr_set_md_alg(&req, MBEDTLS_MD_SHA256);

    // Allocate memory for the CSR PEM output
    *csr_out = (char *)malloc(CSR_BUF_SIZE);
    if (*csr_out == NULL)
    {
        ESP_LOGE(TAG, "Memory allocation failed for CSR PEM buffer.");
        ret = -1;
        goto cleanup;
    }

    // Write the CSR in PEM format
    if ((ret = mbedtls_x509write_csr_pem(&req, (unsigned char *)*csr_out, CSR_BUF_SIZE, mbedtls_ctr_drbg_random, &ctr_drbg)) != 0)
    {
        ESP_LOGE(TAG, "Failed to write CSR to PEM format: -0x%x", -ret);
        free(*csr_out);
        *csr_out = NULL;
        goto cleanup;
    }

    ESP_LOGI(TAG, "CSR successfully generated in PEM format.");

cleanup:
    // Free the resources
    mbedtls_x509write_csr_free(&req);
    mbedtls_pk_free(&key);
    mbedtls_ctr_drbg_free(&ctr_drbg);
    mbedtls_entropy_free(&entropy);

    if (ret != 0)
    {
        ESP_LOGE(TAG, "Error occurred during CSR generation.");
    }
    return ret;
}