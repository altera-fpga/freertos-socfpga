/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Sample application for FCS
 */

/**
 * @defgroup fcs_sample FCS
 * @ingroup samples
 *
 * Sample Application for FCS.
 *
 * @details
 * @subsubsection description Description
 * This is a sample application to demonstrate some of the cryptographic functionalities provided by the SDM. A cryptographic session is to be opened to perform the cryptographic functions. If the functions require a key, it may be imported or created. The user can access these functions through the libFCS library.
 *
 * @subsubsection prerequisites Prerequisites
 * All the required keys should be available in the sdmmc before running the sample. The names of the corresponding keys and their corresponding key IDs should be changed in the sample.
 *
 * @subsubsection fcs_param Configurable Parameters
 * - All key names can be configured at their corrseponding macros
 * - All key IDs can be configured at their corresponding macros
 * - Modes of operation can also be configured for ECC related cryptographic functions
 * - Input size can be configured in @c INPUT_DATA_SIZE macro
 * @note Ensure the correct key ID and modes are provided
 * @subsubsection how_to_run How to Run
 * 1. Follow the common README for build and flashing instructions.
 * 2. Copy all the required key files to the SD card.
 * 3. Ensure key ID matches the keys in the SD card. Change the corresponding macros if it doesnt.
 * 4. Run the sample.
 *
 * @subsubsection expected_results Expected Results
 * - The success/failure logs are displayed in the console.
 * @{
 */
/** @} */

#include <stdint.h>

#include "osal.h"
#include "osal_log.h"
#include "string.h"
#include "socfpga_fcs.h"
#include "socfpga_smmu.h"
#include "ff_sddisk.h"
#include "libfcs.h"

#define READ_MODE              0x01
#define MOUNTED                1
#define UNMOUNTED              0
#define MOUNT_SD_CARD          -1

#define MAX_KEY_SIZE           240U
#define INPUT_DATA_SIZE        128

#define CHIP_SEL_0             0x00000000
#define AES_FILE               "/ae256"
#define AES_AAD_SIZE           32
#define AES_KEY_ID             12
#define AES_IV_SIZE            16U

#define DIGEST_MODE            2U
#define DIGEST_KEY_FILE        "/hm256"
#define DIGEST_SIZE_256        0U
#define DIGEST_KEY_ID          22U

#define ECC_FILE               "/bp384"
#define ECC_FILE_KEY_ID        43U
#define ECC_MODE               4U
#define ECC_EXCHANGE_FILE      "/bpe384"
#define ECC_EXCHANGE_KEY_ID    45U

static FF_Disk_t *xDiskObj = NULL;
static uint8_t ucMountStatus = UNMOUNTED;

static void fat_mount( const char *MountName )
{

    xDiskObj = FF_SDDiskInit(MountName, MOUNT_SD_CARD);
    if (xDiskObj != NULL)
    {
        ucMountStatus = MOUNTED;
    }
    else
    {
        ERROR("Mounting Failed");
    }
}

static void fat_unmount( void )
{
    if (xDiskObj == NULL)
    {
        ERROR("No mounted devices");
        return;
    }
    FF_Unmount(xDiskObj);
    FF_SDDiskDelete(xDiskObj);
    xDiskObj = NULL;
    ucMountStatus = UNMOUNTED;
}
static uint32_t fat_get_size( const char *file )
{
    int ret;
    uint32_t file_size;
    FF_Error_t xError;
    FF_FILE *pxFile;
    fat_mount("root");
    if (xDiskObj == NULL)
    {
        ERROR("Failed to mount");
        return 0;
    }
    pxFile = FF_Open(xDiskObj->pxIOManager, file, FF_MODE_READ, &xError);
    if ((pxFile == NULL) || (xError != FF_ERR_NONE))
    {
        INFO("Failed to open file for reading\r\n");
        FF_Unmount(xDiskObj);
        FF_SDDiskDelete(xDiskObj);
        return 0;
    }

    ret = FF_GetFileSize(pxFile, &file_size);
    if (ret != 0)
    {
        ERROR("Error getting file size ");
        return 0;
    }
    FF_Close(pxFile);
    return file_size;
}
static uint32_t fat_read( const char *file, void *buffer )
{
    FF_Error_t xError;
    uint32_t ulBytesRead = 0;
    FF_FILE *pxFile;
    fat_mount("root");
    if (xDiskObj == NULL)
    {
        ERROR("Failed to mount");
        return 0;
    }
    pxFile = FF_Open(xDiskObj->pxIOManager, file, FF_MODE_READ, &xError);
    if ((pxFile == NULL) || (xError != FF_ERR_NONE))
    {
        INFO("Failed to open file for reading\r\n");
        FF_Unmount(xDiskObj);
        FF_SDDiskDelete(xDiskObj);
        return 0;
    }
    if (pxFile != NULL)
    {
        ulBytesRead = FF_Read(pxFile, 1, pxFile->ulFileSize, (uint8_t*)buffer);
        FF_Close(pxFile);
    }
    fat_unmount();
    return ulBytesRead;
}

static void print_data( char *data, int size )
{
    int i;
    for (i = 0; i < (int)size; i++)
    {
        printf("%02x",data[i]);
        if ((i + 1) % 2 == 0)
        {
            printf("\t");
        }
        if ((i + 1) % 8 == 0)
        {
            printf("\r\n");
        }
    }
    printf("\r\n");
}
static void fcs_qspi_sample()
{
    int ret, i;
    uint32_t chip_sel;
    char *input_data = (char *)pvPortMalloc(INPUT_DATA_SIZE);
    if (input_data == NULL)
    {
        ERROR("Failed to allocate memory");
        return;
    }
    for (i = 0; i < INPUT_DATA_SIZE; i++)
    {
        input_data[i] = i;
    }
    INFO("Input data");
    print_data(input_data, INPUT_DATA_SIZE);
    ret = fcs_qspi_open();
    if (ret != 0)
    {
        ERROR("Failed to open QSPI channel");
        vPortFree(input_data);
        return;
    }
    chip_sel = CHIP_SEL_0;
    ret = fcs_qspi_set_cs(chip_sel);
    if (ret)
    {
        ERROR("Failed to perform chip select");
    }
    else
    {
        ret = fcs_qspi_erase(0x1000, 1024);
        if (ret)
        {
            ERROR("Failed to erase");
        }
        ret = fcs_qspi_write(0x1000, input_data, INPUT_DATA_SIZE / 4);
        if (ret)
        {
            ERROR("Failed to write");
        }
        else
        {
            INFO("Resetting input buffer");
            memset(input_data, 0, INPUT_DATA_SIZE);
            ret = fcs_qspi_read(0x1000, input_data, INPUT_DATA_SIZE / 4);
            if (ret)
            {
                ERROR("Failed to read flash");
            }
            else
            {
                INFO("Data from flash");
                print_data(input_data, INPUT_DATA_SIZE);
            }
        }
    }
    ret = fcs_qspi_close();
    if (ret)
    {
        ERROR("Failed to close QSPI channel");
    }
    vPortFree(input_data);
}
static void fcs_sample_encryption(char *session_id)
{
    int ret, i;
    char *aesfile = AES_FILE, *key_buf, *input_data, *resp_data,
            *iv_data;
    uint32_t bytes_read, context_id = 1, cipher_len = INPUT_DATA_SIZE;
    struct fcs_aes_req aes_req;

    key_buf = (char *)pvPortMalloc(fat_get_size(aesfile));
    INFO("Importing AES key");
    bytes_read = fat_read(aesfile, (void*)key_buf);
    ret = fcs_import_service_key( session_id, key_buf, bytes_read,
            NULL, 0 );
    if (ret)
    {
        ERROR("Failed to import AES key");
        return;
    }

    input_data = pvPortMalloc(INPUT_DATA_SIZE);
    resp_data = pvPortMalloc(INPUT_DATA_SIZE);

    /* setting data */
    INFO("Plaintext");
    for (i = 0; i < (int)INPUT_DATA_SIZE; i++)
    {
        input_data[i] = i;
    }
    print_data(input_data, INPUT_DATA_SIZE);
    INFO("Sample using AES256-CBC ....");
    INFO("Generating IV ....");

    iv_data = (char*)pvPortMalloc(16);
    if ((iv_data == NULL))
    {
        ERROR("Failed to allocate memory");
        return;
    }
    ret = fcs_random_number_ext( session_id, context_id,
            iv_data, AES_IV_SIZE );
    if (ret != 0)
    {
        ERROR("Failed to generate IV");
        return;
    }
    INFO("IV generated");
    for (i = 0; i < (int)AES_IV_SIZE; i++)
    {
        printf("%02x",iv_data[i]);
    }
    printf("\n");

    aes_req.crypt_mode = FCS_AES_ENCRYPT_MODE;
    aes_req.block_mode = FCS_AES_CTR;
    aes_req.iv_source = 0;
    aes_req.iv = iv_data;
    aes_req.iv_len = AES_IV_SIZE;
    aes_req.tag = NULL;
    aes_req.tag_len = 0;
    aes_req.aad_len = 0;
    aes_req.aad = NULL;
    aes_req.input = input_data;
    aes_req.ip_len = INPUT_DATA_SIZE;
    aes_req.output = resp_data;
    aes_req.op_len = &cipher_len;

    INFO("Encrypting ....");
    ret = fcs_aes_crypt( session_id, AES_KEY_ID,
            context_id, &aes_req);
    if (ret)
    {
        ERROR("Failed to encrypt data");
        return;
    }
    INFO("Ciphertext");
    print_data(resp_data, cipher_len);

    INFO("Copying ciphertext to input location");
    memcpy(input_data, resp_data, INPUT_DATA_SIZE);

    INFO("Decrypting....");
    aes_req.crypt_mode = FCS_AES_DECRYPT_MODE;
    context_id++;
    ret = fcs_aes_crypt( session_id, AES_KEY_ID,
            context_id, &aes_req);
    INFO("STATUS : %d", ret);

    if (ret)
    {
        ERROR("Failed to decrypt data");
    }
    INFO("Deciphered data");
    print_data(resp_data, INPUT_DATA_SIZE);
    printf("\r\n");
    vPortFree(iv_data);
    ret = fcs_remove_service_key( session_id, AES_KEY_ID);
    vPortFree(input_data);
    vPortFree(resp_data);
    vPortFree(key_buf);
}
static void fcs_sample_get_digest(char *session_id)
{
    char *key_buf, *input_data, *resp_data,
            res_match_status[4];
    uint32_t bytes_read, context_id = 2, digest_size, match_size;
    int i, ret;
    struct fcs_digest_get_req digest_req;
    struct fcs_mac_verify_req mac_verify;

    INFO("Importing HMAC key");
    key_buf = (char*)pvPortMalloc(fat_get_size(DIGEST_KEY_FILE));
    bytes_read = fat_read(DIGEST_KEY_FILE, (void*)key_buf);
    ret = fcs_import_service_key((char *)session_id, key_buf, bytes_read,
            NULL, 0 );
    if (!ret)
    {
        INFO("Input data");
        input_data = (char *)pvPortMalloc(INPUT_DATA_SIZE + 64);
        for (i = 0; i < (int)INPUT_DATA_SIZE; i++)
        {
            input_data[i] = i;
        }
        print_data(input_data, INPUT_DATA_SIZE);
        printf("\r\n");
        resp_data = (char *)pvPortMalloc(64);
        memset(resp_data, 0, 32);

        context_id++;
        digest_req.sha_op_mode = DIGEST_MODE;
        digest_req.sha_digest_sz = DIGEST_SIZE_256;
        digest_req.src = input_data;
        digest_req.src_len = INPUT_DATA_SIZE;
        digest_req.digest = resp_data;
        digest_req.digest_len = &digest_size;
        INFO("Generating Digest ....");
        ret = fcs_get_digest( session_id, context_id,
                DIGEST_KEY_ID, &digest_req );
        INFO("STATUS : %d", ret);

        if (!ret)
        {
            INFO("Digest Data");
            print_data(resp_data,digest_size);
            INFO("Verifying MAC ....");
            context_id++;
            memcpy(input_data + INPUT_DATA_SIZE, resp_data, digest_size);
            mac_verify.op_mode = DIGEST_MODE;
            mac_verify.dig_sz = DIGEST_SIZE_256;
            mac_verify.src = input_data;
            mac_verify.src_sz = INPUT_DATA_SIZE + digest_size;
            mac_verify.dst = res_match_status;
            mac_verify.dst_sz = &match_size;
            mac_verify.user_data_sz = INPUT_DATA_SIZE;
            ret = fcs_mac_verify( session_id, context_id,
                    DIGEST_KEY_ID, &mac_verify);
            if (!ret)
            {
                INFO("MAC verified");
            }
        }
        else
        {
            ERROR("Get digest operation failed");
        }
    }
    else
    {
        ERROR("Failed to import HMAC key");
    }
    vPortFree(key_buf);
}

void fcs_sample_ecdh( char *session_id )
{
    int ret;
    char *key_buf, *pubkey_data, *resp_data;
    uint32_t bytes_read, context_id = 5, pubkey_size, shared_sec_size;
    struct fcs_ecdh_req ecdh_req;

    INFO("Importing ECDSA Brainpool 384 key");
    key_buf = (char*)pvPortMalloc(fat_get_size(ECC_FILE));
    if (key_buf == NULL)
    {
        ERROR("Failed to allocate memory");
        return;
    }
    bytes_read = fat_read( ECC_FILE, (void*)key_buf );
    ret = fcs_import_service_key( session_id, key_buf,
            bytes_read, NULL, 0 );
    if (ret)
    {
        ERROR("Failed to import key");
        return;
    }
    INFO("Generating public key...");
    pubkey_size = 256;
    pubkey_data = (char *)pvPortMalloc(256);
    if (pubkey_data == NULL)
    {
        ERROR("Failed to allocate memory");
        return;
    }
    ret = fcs_ecdsa_get_pub_key( session_id, context_id,
            ECC_FILE_KEY_ID, ECC_MODE,
            pubkey_data, &pubkey_size );
    if (ret)
    {
        ERROR("Failed to get public key");
    }
    INFO("Public Key Data");
    print_data(pubkey_data, pubkey_size);
    INFO("Importing ECDSA Brainpool 384 Exchange key");
    bytes_read = fat_read( ECC_EXCHANGE_FILE, (void*)key_buf );
    ret = fcs_import_service_key( session_id,
            key_buf, bytes_read, NULL, 0 );
    if (ret)
    {
        ERROR("Failed to import key");
        return;
    }
    resp_data = (char*)0x20;
    ecdh_req.ecc_curve = ECC_MODE;
    ecdh_req.pubkey = pubkey_data;
    ecdh_req.pubkey_len = pubkey_size;
    ecdh_req.shared_secret = resp_data;
    ecdh_req.shared_secret_len = &shared_sec_size;
    INFO("Generating Shared Secret ....");
    memset(resp_data, 0x0, 48);
    ret = fcs_ecdh_request( session_id,
            ECC_EXCHANGE_KEY_ID,
            context_id, &ecdh_req );
    if (ret)
    {
        ERROR("Failed to generate shared secret");
        return;
    }
    INFO("Shared Secret");
    print_data(resp_data, shared_sec_size);
    vPortFree(pubkey_data);
    vPortFree(key_buf);
}

void fcs_sample_signing( char *session_id )
{
    int ret, i;
    char *input_data, *resp_data,
            res_match_status[4];
    uint32_t context_id = 4, sign_size, status_size;
    struct fcs_ecdsa_req ecdsa_req;
    struct fcs_ecdsa_verify_req ecdsa_ver_req;

    INFO("Input Data");
    input_data = (char *)pvPortMalloc(INPUT_DATA_SIZE);
    resp_data = (char *)pvPortMalloc(FCS_MAX_SIG_SIZE);
    for (i = 0; i < INPUT_DATA_SIZE; i++)
    {
        input_data[i] = i;
    }
    print_data(input_data, INPUT_DATA_SIZE);
    ecdsa_req.ecc_curve = 4;
    ecdsa_req.src = input_data;
    ecdsa_req.src_len = 48;
    ecdsa_req.dst = resp_data;
    ecdsa_req.dst_len = &sign_size;

    /* hash sign requires same number of bits as in input key*/
    INFO("Generating Signature ....");
    ret = fcs_ecdsa_hash_sign( session_id, context_id,
            ECC_FILE_KEY_ID, &ecdsa_req );
    if (ret)
    {
        ERROR("Failed to generate signature");
        return;
    }

    INFO("Signature");
    print_data(resp_data, sign_size);

    ecdsa_ver_req.ecc_curve = 4;
    ecdsa_ver_req.src = input_data;
    ecdsa_ver_req.src_len = 48;
    ecdsa_ver_req.signature = resp_data;
    ecdsa_ver_req.signature_len = sign_size;
    ecdsa_ver_req.pubkey = NULL;
    ecdsa_ver_req.pubkey_len = 0;
    ecdsa_ver_req.dst = res_match_status;
    ecdsa_ver_req.dst_len = &status_size;

    INFO("Verifying signature ....");
    ret = fcs_ecdsa_hash_verify( session_id, context_id,
            ECC_FILE_KEY_ID, &ecdsa_ver_req );
    if (!ret)
    {
        INFO("Signature is valid");
    }
    /* for sizes greater than key size, use sha2_data_sign */
    INFO("Generating Signature ....");

    for (i = 0; i < INPUT_DATA_SIZE; i++)
    {
        input_data[i] = i;
    }
    ecdsa_req.ecc_curve = ECC_MODE;
    ecdsa_req.src = input_data;
    ecdsa_req.src_len = INPUT_DATA_SIZE;
    ecdsa_req.dst = resp_data;
    ecdsa_req.dst_len = &sign_size;

    ret = fcs_ecdsa_sha2_data_sign( session_id, context_id,
            ECC_FILE_KEY_ID, &ecdsa_req );
    if (ret)
    {
        ERROR("Failed to generate signature");
        return;
    }

    INFO("Signature");
    print_data(resp_data, sign_size);
    INFO("Verifying signature ....");

    ecdsa_ver_req.ecc_curve = ECC_MODE;
    ecdsa_ver_req.src = input_data;
    ecdsa_ver_req.src_len = INPUT_DATA_SIZE;
    ecdsa_ver_req.signature = resp_data;
    ecdsa_ver_req.signature_len = sign_size;
    ecdsa_ver_req.pubkey = NULL;
    ecdsa_ver_req.pubkey_len = 0;
    ecdsa_ver_req.dst = res_match_status;
    ecdsa_ver_req.dst_len = &status_size;

    ret = fcs_ecdsa_sha2_data_verify( session_id,
            context_id, ECC_FILE_KEY_ID,
            &ecdsa_ver_req );
    if (!ret)
    {
        INFO("Signature is valid");
    }
    vPortFree(input_data);
    vPortFree(resp_data);
}

void fcs_task( void* )
{
    int ret;
    char sess_uuid[FCS_UUID_SIZE];
    INFO("FCS Sample");
    INFO("Initialising libFCS");
    ret = libfcs_init("log_off");

    if (!ret)
    {
        INFO("Opening FCS service session");
        ret = fcs_open_service_session(sess_uuid);
        if (ret == 0)
        {
            INFO("QSPI sample");
            fcs_qspi_sample();
            INFO("AES Encryption");
            fcs_sample_encryption(sess_uuid);
            INFO("Digest Generation");
            fcs_sample_get_digest(sess_uuid);
            INFO("Public Key and ECDH");
            fcs_sample_ecdh(sess_uuid);
            /* Using the same keys for the signing tests*/
            INFO("Hash signing and SHA-2 signing");
            fcs_sample_signing(sess_uuid);
            ret = fcs_close_service_session(sess_uuid);
            if (ret)
            {
                ERROR("Failed to close FCS service session");
            }
        }
        else
        {
            ERROR("Failed to open FCS service session");
        }
        ret = fcs_deinit();
        if (ret)
        {
            ERROR("Failed to free FCS resources");
        }
    }
    else
    {
        ERROR("Failed to initialise FCS");
        return;
    }
    INFO("FCS Sample completed");
}

