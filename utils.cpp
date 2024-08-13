#include <openssl/conf.h>
#include <openssl/evp.h>
#include <openssl/err.h>
#include <string.h>
#include "utils.h"
#include <cstring>
#include <stdlib.h>
#include <openssl/sha.h>
#include <base64.h>
#include <iostream>

using namespace std;

/**
 * The two functions bellow are taken from the openssl official website :
 * https://wiki.openssl.org/index.php/EVP_Symmetric_Encryption_and_Decryption 
 * You can read more there
 */

int encrypt(unsigned char *plaintext, 
            int plaintext_len, 
            unsigned char *key,
            unsigned char *iv, 
            unsigned char *ciphertext) {
    EVP_CIPHER_CTX *ctx;

    int len;

    int ciphertext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the encryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be encrypted, and obtain the encrypted output.
     * EVP_EncryptUpdate can be called multiple times if necessary
     */
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        handleErrors();
    ciphertext_len = len;

    /*
     * Finalise the encryption. Further ciphertext bytes may be written at
     * this stage.
     */
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        handleErrors();
    ciphertext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

int decrypt(unsigned char *ciphertext,
            int ciphertext_len, 
            unsigned char *key,
            unsigned char *iv, 
            unsigned char *plaintext) {
                
    EVP_CIPHER_CTX *ctx;

    int len;

    int plaintext_len;

    /* Create and initialise the context */
    if(!(ctx = EVP_CIPHER_CTX_new()))
        handleErrors();

    /*
     * Initialise the decryption operation. IMPORTANT - ensure you use a key
     * and IV size appropriate for your cipher
     * In this example we are using 256 bit AES (i.e. a 256 bit key). The
     * IV size for *most* modes is the same as the block size. For AES this
     * is 128 bits
     */
    if(1 != EVP_DecryptInit_ex(ctx, EVP_aes_128_cbc(), NULL, key, iv))
        handleErrors();

    /*
     * Provide the message to be decrypted, and obtain the plaintext output.
     * EVP_DecryptUpdate can be called multiple times if necessary.
     */
    if(1 != EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        handleErrors();
    plaintext_len = len;

    /*
     * Finalise the decryption. Further plaintext bytes may be written at
     * this stage.
     */
    if(1 != EVP_DecryptFinal_ex(ctx, plaintext + len, &len))
        handleErrors();
    plaintext_len += len;

    /* Clean up */
    EVP_CIPHER_CTX_free(ctx);

    return plaintext_len;
}

void handleErrors() {
    ERR_print_errors_fp(stderr);
    abort();
}

void hashPassword(const char *string, char outputBuffer[65]) {

    #if OPENSSL_VERSION_NUMBER < 0x10100000L
    #  define EVP_MD_CTX_new   EVP_MD_CTX_create
    #  define EVP_MD_CTX_free  EVP_MD_CTX_destroy
    #endif

    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    unsigned char *hash = (unsigned char *) malloc(EVP_MAX_MD_SIZE * sizeof(char));
    unsigned int hashSize = 65;
    unsigned char * tempOutBuff =  (unsigned char *) malloc(hashSize * sizeof(char));

    EVP_DigestInit(ctx, EVP_sha256());
    EVP_DigestUpdate(ctx, string, strlen(string));
    EVP_DigestFinal(ctx, tempOutBuff, &hashSize);

    for(int i = 0 ; i < hashSize ; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", tempOutBuff[i]);
    }

    outputBuffer[64] = 0;

}

string myEncrypt(string plainText, string pass) {

    // Converting string to unsigned character (bytes) array
    unsigned char *plainTextBytes = (unsigned char *) plainText.c_str();

    // Allocate memory for hash bytes (chars) array with 65 elements
    char *hash = (char *) malloc(sizeof(char) * 65);
    
    // Allocate memory for cipher bytes (chars) array with 1024 elements
    unsigned char *cipher = (unsigned char *) malloc(sizeof(char) * 1024);

    // Here we hash the plain text password
    hashPassword((const char *)pass.c_str(), hash);

    // We convert the char array (hash variable) into an std::string
    string hashStr(hash);
    cout << "SHA256: " << hashStr << endl;

    // The hash contains the encrytion key and the initialization vector -> (iv)
    // so we divide it between the key and the iv using std::string method substr()
    string keyStr = hashStr.substr(0, 16);
    cout << "Key: " << keyStr << endl;

    string iv = hashStr.substr(16, 16);
    cout << "iv: " << iv << endl;

    // here we initiate the encryption process
    int cipherSize = encrypt(
        plainTextBytes, 
        strlen((char *) plainTextBytes), 
        (unsigned char *) keyStr.c_str(), 
        (unsigned char *) iv.c_str(), 
        cipher
    );

    // We encode the cipher into a base64 encoded std::string  
    // string base64Cipher = base64_encode(
    //     cipher,
    //     cipherSize);

    unsigned char base64Cipher[1024];
    EVP_EncodeBlock(base64Cipher, cipher, cipherSize);
    
    // We encode the iv to the base64
    string base64Iv = base64_encode(iv);

    // Append the base64 encode iv to the end of the base64 encoded cipher with a `:` in between
    // string cipherStr((char *) base64Cipher.c_str());
    string cipherStr((char *) base64Cipher);

    return cipherStr;
}

string myDecrypt(string cipher, string pass) {

    // string cipherDecoded = base64_decode(cipher);
    // unsigned char * cipherBytes = (unsigned char *) cipherDecoded.c_str();

    unsigned char *cipherBytes = (unsigned char *) malloc(sizeof(char) * 1024);
    EVP_DecodeBlock(cipherBytes, (unsigned char *) cipher.c_str(), strlen(cipher.c_str()));

    // Converting string to unsigned character (bytes) array
    unsigned char *plainTextBytes = (unsigned char *) malloc(sizeof(char) * 1024);

    // Allocate memory for hash bytes (chars) array with 65 elements
    char *hash = (char *) malloc(sizeof(char) * 65);

    // Here we hash the plain text password
    hashPassword((const char *)pass.c_str(), hash);

    // We convert the char array (hash variable) into an std::string
    string hashStr(hash);

    // The hash contains the encrytion key and the initialization vector -> (iv)
    // so we divide it between the key and the iv using std::string method substr()
    string keyStr = hashStr.substr(0, 16);
    // cout << "Key: " << keyStr << endl;

    string iv = hashStr.substr(16, 16);
    // cout << "iv: " << iv << endl;

    int len = decrypt(
        cipherBytes,
        strlen((char *)cipherBytes),
        (unsigned char *) keyStr.c_str(),
        (unsigned char *) iv.c_str(),
        plainTextBytes
    );

	plainTextBytes[len] = '\0';


    string plainText((char *) plainTextBytes);
    return plainText;
}
