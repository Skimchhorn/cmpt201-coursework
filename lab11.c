// lab11.c
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <openssl/evp.h>
#include <openssl/pem.h>
#include <openssl/err.h>

#define NUM_FILES 3

static void print_openssl_errors(const char *msg) {
    fprintf(stderr, "%s\n", msg);
    ERR_print_errors_fp(stderr);
}

static int read_entire_file(const char *path, unsigned char **buffer, size_t *length) {
    FILE *fp = fopen(path, "rb");
    long size;
    size_t bytes_read;

    if (fp == NULL) {
        fprintf(stderr, "Error: could not open file: %s\n", path);
        return 0;
    }

    if (fseek(fp, 0, SEEK_END) != 0) {
        fprintf(stderr, "Error: could not seek file: %s\n", path);
        fclose(fp);
        return 0;
    }

    bytes_read = fread(*buffer, 1, (size_t)size, fp);
    if (bytes_read != (size_t)size) {
        fprintf(stderr, "Error: could not read all bytes from file: %s\n", path);
        free(*buffer);
        *buffer = NULL;
        fclose(fp);
        return 0;
    }

    fclose(fp);

    (*buffer)[size] = '\0';   // helpful for printing text files
    *length = (size_t)size;
    return 1;
}

static void print_message_file(const char *path) {
    unsigned char *data = NULL;
    size_t len = 0;

    if (!read_entire_file(path, &data, &len)) {
        printf("(unable to read message)\n");
        return;
    }

    fwrite(data, 1, len, stdout);
    if (len == 0 || data[len - 1] != '\n') {
        printf("\n");
    }

    free(data);
}

static EVP_PKEY *load_public_key(char *loaded_name, size_t loaded_name_size) {
    const char *candidates[] = {
        "public_key.pem",
        "public.pem",
        "pubkey.pem",
        "publickey.pem"
    };

    size_t i;
    for (i = 0; i < sizeof(candidates) / sizeof(candidates[0]); i++) {
        FILE *fp = fopen(candidates[i], "r");
        EVP_PKEY *pkey = NULL;

        if (fp == NULL) {
            continue;
        }

        pkey = PEM_read_PUBKEY(fp, NULL, NULL, NULL);
        fclose(fp);

        if (pkey != NULL) {
            snprintf(loaded_name, loaded_name_size, "%s", candidates[i]);
            return pkey;
        }
    }

    return NULL;
}

/*
 error
 */
int verify(EVP_PKEY *public_key, const char *message_path, const char *sign_path) {
    unsigned char *message = NULL;
    unsigned char *signature = NULL;
    size_t message_len = 0;
    size_t signature_len = 0;
    EVP_MD_CTX *md_ctx = NULL;
    int rc = -1;

    if (public_key == NULL || message_path == NULL || sign_path == NULL) {
        fprintf(stderr, "Error: verify() got NULL argument\n");
        return -1;
    }

    if (!read_entire_file(message_path, &message, &message_len)) {
        return -1;
    }

    if (!read_entire_file(sign_path, &signature, &signature_len)) {
        free(message);
        return -1;
    }

    md_ctx = EVP_MD_CTX_new();
    if (md_ctx == NULL) {
        print_openssl_errors("Error: EVP_MD_CTX_new() failed");
        free(message);
        free(signature);
        return -1;
    }


    if (EVP_DigestVerifyInit(md_ctx, NULL, EVP_sha256(), NULL, public_key) != 1) {
        print_openssl_errors("Error: EVP_DigestVerifyInit() failed");
        goto cleanup;
    }

    if (EVP_DigestVerifyUpdate(md_ctx, message, message_len) != 1) {
        print_openssl_errors("Error: EVP_DigestVerifyUpdate() failed");
        goto cleanup;
    }

    rc = EVP_DigestVerifyFinal(md_ctx, signature, signature_len);
    if (rc < 0) {
        print_openssl_errors("Error: EVP_DigestVerifyFinal() failed");
        rc = -1;
        goto cleanup;
    }

cleanup:
    EVP_MD_CTX_free(md_ctx);
    free(message);
    free(signature);
    return rc;
}

int main(void) {
    const char *messages[NUM_FILES] = {
        "message1.txt",
        "message2.txt",
        "message3.txt"
    };

    const char *signatures[NUM_FILES] = {
        "signature1.sig",
        "signature2.sig",
        "signature3.sig"
    };

    EVP_PKEY *public_key = NULL;
    char key_name[128];
    int i;

    public_key = load_public_key(key_name, sizeof(key_name));
    if (public_key == NULL) {
        fprintf(stderr, "Error: could not load public key.\n");
        fprintf(stderr, "Make sure your PEM public key file is in the lab folder.\n");
        return EXIT_FAILURE;
    }

    printf("Loaded public key from: %s\n\n", key_name);

    for (i = 0; i < NUM_FILES; i++) {
        int result;

        printf("========== Pair %d ==========\n", i + 1);
        printf("Message file: %s\n", messages[i]);
        printf("Signature file: %s\n\n", signatures[i]);

        printf("Message contents:\n");
        print_message_file(messages[i]);
        printf("\n");

        result = verify(public_key, messages[i], signatures[i]);

        if (result == 1) {
            printf("Verification result: VALID signature\n");
        } else if (result == 0) {
            printf("Verification result: INVALID signature\n");
        } else {
            printf("Verification result: ERROR during verification\n");
        }

        printf("\n");
    }

    EVP_PKEY_free(public_key);
    return EXIT_SUCCESS;
}
