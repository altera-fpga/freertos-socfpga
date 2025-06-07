/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Test implementation for FCS
 */

/* Test includes */
#include "unity.h"
#include "unity_fixture.h"
/* Driver includes */
#include "string.h"
#include "osal.h"
#include "osal_log.h"
#include "ff_sddisk.h"
#include "libfcs.h"
#include "socfpga_fcs.h"
#define READ_MODE        0x01
#define MOUNTED          1
#define UNMOUNTED        0
#define MOUNT_SD_CARD    -1

/*-----------------------------------------------------------*/

/* Globals values which can be overwritten by the test
 * framework invoking these tests */
/*-----------------------------------------------------------*/
char sess_id[FCS_UUID_SIZE];
static FF_Disk_t *xDiskObj = NULL;
static uint8_t ucMountStatus = UNMOUNTED;
/*-----------------------------------------------------------*/
/** Static globals */
/*-----------------------------------------------------------*/

static void fcs_mount( const char *MountName )
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

static uint32_t fcs_read( const char *FileName, void *buffer )
{
    FF_Error_t xError;
    uint32_t bytes_read = 0;
    FF_FILE *pxFile = FF_Open(xDiskObj->pxIOManager, FileName, READ_MODE,
            &xError);
    if (pxFile == NULL)
    {
        ERROR("File not opened");
    }
    else
    {
        bytes_read = FF_Read(pxFile, 1, pxFile->ulFileSize, (uint8_t*)buffer);
        FF_Close(pxFile);
    }
    return bytes_read;
}

static void fcs_unmount( void )
{
    FF_Unmount(xDiskObj);
    FF_SDDiskDelete(xDiskObj);
    xDiskObj = NULL;
    ucMountStatus = UNMOUNTED;
}
/*-----------------------------------------------------------*/

/* Define Test Group. */
TEST_GROUP(TEST_FCS);

/*-----------------------------------------------------------*/

/**
 * @brief Setup function called before each test in this group is executed.
 */
TEST_SETUP( TEST_FCS )
{
}

/*-----------------------------------------------------------*/

/**
 * @brief Tear down function called after each test in this group is executed.
 */
TEST_TEAR_DOWN( TEST_FCS )
{
}

/*-----------------------------------------------------------*/

/**
 * @brief Function to define which tests to execute as part of this group.
 */
TEST_GROUP_RUNNER( TEST_FCS )
{
    fcs_mount("root");
    if (xDiskObj != NULL)
    {
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_FcsInit);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_OpenSession);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_CounterValTests);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_HKDFTests);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_QspiTests);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_AttestationGetCert);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_AttestationCertReload);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_MctpSend);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_SdosCryption);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_RandomNumGen);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_DevIdentityData);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_ProvisionData);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_JtagId);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_ChipId);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_KeyTests);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_PublicKeyTests);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_AesCryption);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_HashTests);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_SignTests);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_Sha2SignTests);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_FcsCloseSession);

		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_RandomNumberFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_ImportServiceKeyFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_ExportServiceKeyFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_RemoveServiceKeyFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_GetServiceKeyInfoFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_CreateServiceKeyFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_GetDigestFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_ServiceGetProvisionDataFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_ServiceCounterSetFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_ServiceCounterSetPreauthorizedFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_MacVerifyFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_SdosEncryptFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_SdosDecryptFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_HkdfRequestFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_AesCryptFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_EcdhRequestFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_GetChipIdFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_AttestationGetCertificateFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_MctpCmdSendFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_GetJtagIdcodeFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_GetDeviceIdentityFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_QspiReadFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_QspiWriteFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_QspiEraseFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_EcdsaGetPubKeyFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_EcdsaHashSignFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_EcdsaHashVerifyFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_EcdsaSha2DataSignFuzzing);
		RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_EcdsaSha2DataVerifyFuzzing);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_OpenSessionFuzzing);
        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_CloseSessionFuzzing);

        RUN_TEST_CASE(TEST_FCS, AFQP_Fcs_FcsDeinit);
        fcs_unmount();
    }
    else
    {
        ERROR("Mount failed, unable to read files needed for tests");
        return;
    }
}

/*-----------------------------------------------------------*/

/*Main function to invoke the tests*/
void fcs_test( void )
{
    UNITY_BEGIN();
    TEST_SETUP(TEST_FCS);
    RUN_TEST_GROUP(TEST_FCS);
    UNITY_END();
}

/*-----------------------------------------------------------*/

/**
 * @brief Test function to test fcs_init
 */
TEST( TEST_FCS, AFQP_Fcs_FcsInit )
{
    int ret;

    ret = libfcs_init("log_off");
    TEST_ASSERT_EQUAL( 0, ret );
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test fcs_open_session
 */
TEST( TEST_FCS, AFQP_Fcs_OpenSession )
{
    int ret, i;
	memset(sess_id, 0, sizeof(sess_id));
    ret = fcs_open_service_session(sess_id);

    TEST_ASSERT_EQUAL( 0, ret );
	for( i = 0; i < 4; i++ )
	{
		TEST_ASSERT_NOT_EQUAL(0, sess_id[i]);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test the random number generator
 */
TEST( TEST_FCS, AFQP_Fcs_RandomNumGen )
{
    int ret;
    char *rand_buf = pvPortMalloc(32);
    uint32_t context_id = 1;
    ret = fcs_random_number_ext(sess_id, context_id, rand_buf, 32);

    TEST_ASSERT_EQUAL( 0, ret );
	if(rand_buf != NULL)
	{
    	vPortFree(rand_buf);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test the counter set command
 */
TEST( TEST_FCS, AFQP_Fcs_CounterValTests )
{
    int ret;
	char *cert, *cert_file = "/cn1fs";
	uint32_t bytes_read, status;
    cert = (char*)pvPortMalloc(500);
	TEST_ASSERT_NOT_EQUAL(NULL, cert);

    bytes_read = fcs_read( cert_file, (void*)cert );
    TEST_ASSERT_NOT_EQUAL(0, bytes_read);

	if(TEST_PROTECT())
	{
		ret = fcs_service_counter_set(cert, bytes_read, 1, (char*)&status);
		TEST_ASSERT_EQUAL( 0, ret );

		ret = fcs_service_counter_set_preauthorized(1, 256, 1);
		TEST_ASSERT_EQUAL( 0, ret );
		vPortFree(cert);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test the MCTP send
 */
TEST( TEST_FCS, AFQP_Fcs_MctpSend )
{
    int ret;
	char input[8], *resp;
	uint32_t resp_size = 0;

	resp = (char*)pvPortMalloc(4096);
	TEST_ASSERT_NOT_EQUAL(NULL, resp);
	if(TEST_PROTECT())
	{
		input[0] = 0x0;
		input[1] = 0x0;
		input[2] = 0x13;
		input[3] = 0x05;
		input[4] = 0x10;
		input[5] = 0x84;
		input[6] = 0x0;
		input[7] = 0x0;
    	ret = fcs_mctp_cmd_send(input, sizeof(input), resp, &resp_size);

    	TEST_ASSERT_EQUAL( 0, ret );
		TEST_ASSERT_NOT_EQUAL( 0, resp_size );
    	vPortFree(resp);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test the SDOS Encrypt and Decrypt
 */
TEST( TEST_FCS, AFQP_Fcs_SdosCryption )
{
    int ret, i, own_id_size, iv_size;
	char *input, *cipher, *decrypted, *payload;
	uint32_t cipher_size = 0, decrypted_size = 0, context_id = 1;

	struct fcs_sdos_crypt_header *sdos_header;
	struct fcs_sdos_enc_req sdos_enc_req;
	struct fcs_sdos_dec_req sdos_dec_req; 

	input = (char*)pvPortMalloc(64);
	payload = (char*)pvPortMalloc(64 + sizeof(struct fcs_sdos_crypt_header));
	cipher = (char*)pvPortMalloc(160);
	decrypted = (char*)pvPortMalloc(160);
	TEST_ASSERT_NOT_EQUAL(NULL, input);
	TEST_ASSERT_NOT_EQUAL(NULL, payload);
	TEST_ASSERT_NOT_EQUAL(NULL, cipher);
	TEST_ASSERT_NOT_EQUAL(NULL, decrypted);
	if(TEST_PROTECT())
	{
		for( i = 0; i < 64; i++ )
		{
			input[i] = i + 64;
		}
		sdos_header = (struct fcs_sdos_crypt_header *)payload;
		sdos_header->magic_number = FCS_SDOS_MAGIC_NUMBER;
		sdos_header->data_len = 64;
		sdos_header->pad = 0;
		sdos_header->srk_indx = 0;
		sdos_header->app_spec_obj_info = 0xFEDC;
		own_id_size = sizeof(sdos_header->owner_id);
		for( i = 0; i < own_id_size; i++ )
		{
			sdos_header->owner_id[i] = i;
		}
		sdos_header->hdr_pad = FCS_SDOS_HEADER_PADDING;
		iv_size = sizeof(sdos_header->iv_field);

		for( i = 0; i < iv_size; i++ )
		{
			sdos_header->iv_field[i] = 0;
		}
		memcpy(payload + sizeof(struct fcs_sdos_crypt_header), input, 64);

		sdos_enc_req.op_mode = 1;
		memcpy(&sdos_enc_req.own, sdos_header->owner_id, sizeof(sdos_header->owner_id));
		sdos_enc_req.id = 0xFEDC;
		sdos_enc_req.src = payload;
		sdos_enc_req.src_sz = sizeof(struct fcs_sdos_crypt_header) + 64;
		sdos_enc_req.dst = cipher;
		sdos_enc_req.dst_sz = &cipher_size;

		ret = fcs_sdos_encrypt(sess_id, context_id, &sdos_enc_req);
		TEST_ASSERT_EQUAL( 0, ret );

		sdos_dec_req.op_mode = 0;
		sdos_dec_req.src = cipher;
		sdos_dec_req.src_sz = cipher_size;
		sdos_dec_req.pad = 0;
		sdos_dec_req.dst = decrypted;
		sdos_dec_req.dst_sz = &decrypted_size;

		ret = fcs_sdos_decrypt(sess_id, context_id, &sdos_dec_req);
		TEST_ASSERT_EQUAL( 0, ret );
		TEST_ASSERT_EQUAL( 0, memcmp(input, decrypted + sizeof(struct fcs_sdos_crypt_header), 64) );

		vPortFree(input);
		vPortFree(cipher);
		vPortFree(decrypted);
		vPortFree(payload);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test getting the device identity
 */
TEST( TEST_FCS, AFQP_Fcs_DevIdentityData )
{
    int ret;
    char *dev_data = pvPortMalloc(200);
    uint32_t dev_data_size = 0;
    ret = fcs_get_device_identity(dev_data, &dev_data_size);

    TEST_ASSERT_EQUAL( 0, ret );
    TEST_ASSERT_NOT_EQUAL( 0, dev_data_size );
	if(dev_data != NULL)
    {
		vPortFree(dev_data);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test getting the provision data
 */
TEST( TEST_FCS, AFQP_Fcs_ProvisionData )
{
    int ret;
    char *prov_data = pvPortMalloc(200);
    uint32_t prov_data_size = 0;
    ret = fcs_service_get_provision_data(prov_data, &prov_data_size);

    TEST_ASSERT_EQUAL( 0, ret );
    TEST_ASSERT_NOT_EQUAL( 0, prov_data_size );
	if(prov_data != NULL)
	{
    	vPortFree(prov_data);
	}	
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to get jtag idcode
 */
TEST( TEST_FCS, AFQP_Fcs_JtagId )
{
    int ret;
    uint32_t jtag_id_code = 0;
    ret = fcs_get_jtag_idcode(&jtag_id_code);

    TEST_ASSERT_EQUAL( 0, ret );
    TEST_ASSERT_NOT_EQUAL( 0, jtag_id_code );

}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to get chip id
 */
TEST( TEST_FCS, AFQP_Fcs_ChipId )
{
    int ret;
    uint32_t chip_id_low = 0, chip_id_high = 0;
    ret = fcs_get_chip_id(&chip_id_low, &chip_id_high);

    TEST_ASSERT_EQUAL( 0, ret );
    TEST_ASSERT_NOT_EQUAL( 0, chip_id_low );
    TEST_ASSERT_NOT_EQUAL( 0, chip_id_high );

}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to get chip id
 */
TEST( TEST_FCS, AFQP_Fcs_AttestationGetCert )
{
    int ret;
    uint32_t cert_size = 0;
	char *cert = pvPortMalloc(4096);
    ret = fcs_attestation_get_certificate(1, cert, &cert_size);

    TEST_ASSERT_EQUAL( 0, ret );
    TEST_ASSERT_NOT_EQUAL( 0, cert_size);
	if(cert != NULL)
	{
		vPortFree(cert);
	}

}
TEST( TEST_FCS, AFQP_Fcs_AttestationCertReload )
{
	int ret, i = 1, j;

	j=2;
	ret = fcs_attestation_certificate_reload(i<<j);
	TEST_ASSERT_EQUAL( 0, ret );

	j=3;
	ret = fcs_attestation_certificate_reload(i<<j);
	TEST_ASSERT_EQUAL( 0, ret );

}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test FCS key management functions
 */
TEST( TEST_FCS, AFQP_Fcs_KeyTests )
{
    int ret;
    uint32_t key_size, bytes_read, status_size;
    char *file_name = "/ae128", *key, status[4];
    key = (char*)pvPortMalloc(FCS_MAX_KEY_SIZE);
	TEST_ASSERT_NOT_EQUAL(NULL, key);

    bytes_read = fcs_read( file_name, (void*)key );
    TEST_ASSERT_NOT_EQUAL(0, bytes_read);
    if (TEST_PROTECT())
    {
        INFO("Starting file tests");
        status_size = 0;
        ret = fcs_import_service_key( sess_id, key,
                bytes_read, status, &status_size);
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_NOT_EQUAL( 0, status_size );
        TEST_ASSERT_EQUAL( 0x0, status[0] );

        ret = fcs_remove_service_key( sess_id, 0xB);
        TEST_ASSERT_EQUAL( 0, ret );

        ret = fcs_create_service_key( sess_id, key,
                bytes_read, status, 4);
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 0x0, status[0] );

        ret = fcs_export_service_key( sess_id, 0xB,
                key, &key_size);
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( key_size, 0x88 );

        key_size = 0;
        ret = fcs_get_service_key_info( sess_id, 0xB,
                key, &key_size );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_NOT_EQUAL( 0, key_size );

		vPortFree(key);
    }
}

/*-----------------------------------------------------------*/

/**
 * @brief Test function to QSPI through SDM Mailbox
 */
TEST( TEST_FCS, AFQP_Fcs_QspiTests )
{
	int ret, i;
    char *input_data = (char *)pvPortMalloc(64);
    TEST_ASSERT_NOT_EQUAL(NULL, input_data);
	if(TEST_PROTECT())
	{
		for (i = 0; i < 64; i++)
		{
			input_data[i] = i;
		}
		ret = fcs_qspi_open();
		TEST_ASSERT_EQUAL( 0, ret );
		ret = fcs_qspi_set_cs(0);
		TEST_ASSERT_EQUAL( 0, ret );
		if(TEST_PROTECT())
		{
			ret = fcs_qspi_erase(0x1000, 1024);
			TEST_ASSERT_EQUAL( 0, ret );
			ret = fcs_qspi_write(0x1000, input_data, 64 / 4);
			TEST_ASSERT_EQUAL( 0, ret );
			memset(input_data, 0, 64);
			//ret = fcs_qspi_set_cs(chip_sel);
			ret = fcs_qspi_read(0x1000, input_data, 64 / 4);
			TEST_ASSERT_EQUAL( 0, ret );
		}
		ret = fcs_qspi_close();
		TEST_ASSERT_EQUAL( 0, ret );
	}
    vPortFree(input_data);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test fcs_aes_cryption
 */
TEST( TEST_FCS, AFQP_Fcs_AesCryption )
{
    int ret, i;
    char *file_name = "/ae256", *gcm_key = "/gc256", *plain_text, *cipher_text,
            *decrypted_data, *iv_data, *key;
    uint32_t context_id, bytes_read, output_len;
    struct fcs_aes_req aes_req;

    INFO("Starting AES functional test");
    key = (char*)pvPortMalloc(140);
    TEST_ASSERT_NOT_EQUAL(NULL, key);

    bytes_read = fcs_read( file_name, (void*)key );
    TEST_ASSERT_NOT_EQUAL(0, bytes_read);

    /* not checking status, if failing we abort */
    ret = fcs_import_service_key( sess_id, key, bytes_read,
            NULL, 0 );
    TEST_ASSERT_EQUAL( 0, ret );
	bytes_read = fcs_read( gcm_key, (void*)key );
    TEST_ASSERT_NOT_EQUAL(0, bytes_read);

    /* not checking status, if failing we abort */
    ret = fcs_import_service_key( sess_id, key, bytes_read,
            NULL, 0 );
    TEST_ASSERT_EQUAL( 0, ret );
    plain_text = (char*)pvPortMalloc(96);
    cipher_text = (char*)pvPortMalloc(96);
    decrypted_data = (char*)pvPortMalloc(96);
    TEST_ASSERT_NOT_EQUAL(NULL, plain_text);
	TEST_ASSERT_NOT_EQUAL(NULL, cipher_text);
	TEST_ASSERT_NOT_EQUAL(NULL, decrypted_data);

	if (TEST_PROTECT())
    {
        for (i = 0; i < 64; i++)
        {
            plain_text[i] = i;
        }

        memset(cipher_text, 0x0, 64);

        memset(decrypted_data, 0x0, 64);

        memset(&aes_req, 0x0, sizeof(aes_req));
        aes_req.crypt_mode = FCS_AES_ENCRYPT;
        aes_req.block_mode = FCS_AES_ECB;
        aes_req.input = plain_text;
        aes_req.ip_len = 64;
        aes_req.output = cipher_text;
        aes_req.op_len = &output_len;

        context_id = 1;
        /* ECB mode */
        ret = fcs_aes_crypt( sess_id, 12, context_id,
                &aes_req );

        if (TEST_PROTECT())
        {
            context_id++;
            aes_req.crypt_mode = FCS_AES_DECRYPT;
            aes_req.input = cipher_text;
            aes_req.output = decrypted_data;
            ret = fcs_aes_crypt(sess_id, 12, context_id,
                    &aes_req);

            if (TEST_PROTECT())
            {
                ret = memcmp( plain_text, decrypted_data, 64 );
                TEST_ASSERT_EQUAL( 0, ret );
            }
        }
        /* CBC mode */
        iv_data = (char*)pvPortMalloc(16);
        if (iv_data != NULL)
        {
            ret = fcs_random_number_ext(sess_id, context_id, 
					iv_data, 16);
            TEST_ASSERT_EQUAL( 0, ret );
            if (TEST_PROTECT())
            {
				memset(cipher_text, 0x0, 64);
				aes_req.block_mode = FCS_AES_CBC;
				aes_req.iv = iv_data;
				aes_req.iv_len = 16;
				aes_req.crypt_mode = FCS_AES_DECRYPT;
				aes_req.input = plain_text;
        		aes_req.output = cipher_text;
                ret = fcs_aes_crypt( sess_id, 12, context_id,
                        &aes_req);
                TEST_ASSERT_EQUAL( 0, ret );

                if (TEST_PROTECT())
                {
                    memset(decrypted_data, 0x0, 64);
                    context_id++;
                    aes_req.crypt_mode = FCS_AES_ENCRYPT;
                    aes_req.input = cipher_text;
                    aes_req.output = decrypted_data;
                    ret = fcs_aes_crypt( sess_id, 12, context_id,
                            &aes_req);
                    TEST_ASSERT_EQUAL( 0, ret );

                    if (TEST_PROTECT())
                    {
                        ret = memcmp( plain_text, decrypted_data, 64 );
                        TEST_ASSERT_EQUAL( 0, ret );
                    }
                }
                /* CTR mode */
				memset(cipher_text, 0x0, 64);
				aes_req.block_mode = FCS_AES_CTR;
				aes_req.iv = iv_data;
				aes_req.iv_len = 16;
				aes_req.crypt_mode = FCS_AES_ENCRYPT;
				aes_req.input = plain_text;
        		aes_req.output = cipher_text;
                ret = fcs_aes_crypt( sess_id, 12, context_id,
                        &aes_req);
                TEST_ASSERT_EQUAL( 0, ret );

                if (TEST_PROTECT())
                { 
                    memset(decrypted_data, 0x0, 64);
                    context_id++;
					aes_req.crypt_mode = FCS_AES_DECRYPT;
                    aes_req.input = cipher_text;
                    aes_req.output = decrypted_data;
                    ret = fcs_aes_crypt(sess_id, 12, context_id,
                            &aes_req);
                    TEST_ASSERT_EQUAL( 0, ret );

                    if (TEST_PROTECT())
                    {
                        ret = memcmp( plain_text, decrypted_data, 64 );
                        TEST_ASSERT_EQUAL( 0, ret );
                    }
                }
            }
			vPortFree(iv_data);
        }
    }
	if (key != NULL)
	{
		vPortFree(key);
	}
	if (plain_text != NULL)
	{
		vPortFree(plain_text);
	}
	if (cipher_text != NULL)
	{
		vPortFree(cipher_text);
	}
	if (decrypted_data != NULL)
	{
		vPortFree(decrypted_data);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test hash generation and verification
 * functions
 */
TEST( TEST_FCS, AFQP_Fcs_HashTests )
{
    int ret, i;
    uint32_t hash_size, context_id, bytes_read, res_size;
    char *file_name = "/hm256", *src_data, *hash_data, *key,
            res_data[4];
	struct fcs_digest_get_req digest_req;
	struct fcs_mac_verify_req mac_verify_req;

    key = (char*)pvPortMalloc(240);
    TEST_ASSERT_NOT_EQUAL(NULL, key);
	src_data = (char*)pvPortMalloc(256);
	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	hash_data = pvPortMalloc(64);
	TEST_ASSERT_NOT_EQUAL(NULL, hash_data);
    if (TEST_PROTECT())
    {
        context_id = 1;
        for (i = 0; i < 64; i++)
        {
            src_data[i] = i;
        }
        /* SHA-2 Digest */
		digest_req.sha_op_mode = FCS_DIGEST_OPMODE_SHA2;
		digest_req.sha_digest_sz = FCS_DIGEST_SIZE_256;
		digest_req.src = src_data;
		digest_req.src_len = 64;
		digest_req.digest = hash_data;
		digest_req.digest_len = &hash_size;
        ret = fcs_get_digest( sess_id, context_id, 0, &digest_req );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 32, hash_size );
        
		hash_size = 0;
        context_id++;
		digest_req.sha_digest_sz = FCS_DIGEST_SIZE_384;
        ret = fcs_get_digest( sess_id, context_id, 0, &digest_req );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 48, hash_size );

        hash_size = 0;
        context_id++;
		digest_req.sha_digest_sz = FCS_DIGEST_SIZE_512;
        ret = fcs_get_digest( sess_id, context_id, 0, &digest_req );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 64, hash_size );

        /* HMAC Digest */
        bytes_read = fcs_read( file_name, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );

        hash_size = 0;
        context_id++;
		digest_req.sha_op_mode = FCS_DIGEST_OPMODE_HMAC;
		digest_req.sha_digest_sz = FCS_DIGEST_SIZE_256;
        ret = fcs_get_digest( sess_id, context_id, 22, &digest_req );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 32, hash_size );
		/* copying HMAC data after input data */
		memcpy(src_data + 64, hash_data, hash_size);

        context_id++;
		mac_verify_req.op_mode = FCS_DIGEST_OPMODE_HMAC;
		mac_verify_req.dig_sz = FCS_DIGEST_SIZE_256;
		mac_verify_req.src = src_data;
		mac_verify_req.src_sz = 64 + hash_size;
		mac_verify_req.dst = res_data;
		mac_verify_req.dst_sz = &res_size;
		mac_verify_req.user_data_sz = 64;
        ret = fcs_mac_verify( sess_id, context_id, 22, &mac_verify_req );
        TEST_ASSERT_EQUAL( 0, ret );
        ret = fcs_remove_service_key( sess_id, 22);
        TEST_ASSERT_EQUAL( 0, ret );

        file_name = "/hm384";
        bytes_read = fcs_read( file_name, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);

        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );
        hash_size = 0;
        
		context_id++;
		digest_req.sha_digest_sz = FCS_DIGEST_SIZE_384;
        ret = fcs_get_digest( sess_id, context_id, 23, &digest_req );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 48, hash_size );
		/* copying HMAC data after input data */
		memcpy(src_data + 64, hash_data, hash_size);

        context_id++;
		mac_verify_req.dig_sz = FCS_DIGEST_SIZE_384;
		mac_verify_req.src_sz = 64 + hash_size;
		ret = fcs_mac_verify( sess_id, context_id, 23, &mac_verify_req );
        TEST_ASSERT_EQUAL( 0, ret );
        ret = fcs_remove_service_key( sess_id, 23);
        TEST_ASSERT_EQUAL( 0, ret );

        file_name = "/hm512";
        bytes_read = fcs_read( file_name, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );

        hash_size = 0;
        context_id++;
		digest_req.sha_digest_sz = FCS_DIGEST_SIZE_512;
        ret = fcs_get_digest( sess_id, context_id, 25, &digest_req );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 64, hash_size );
		/* copying HMAC data after input data */
		memcpy(src_data + 64, hash_data, hash_size);

        context_id++;
		mac_verify_req.dig_sz = FCS_DIGEST_SIZE_512;
		mac_verify_req.src_sz = 64 + hash_size;
		ret = fcs_mac_verify( sess_id, context_id, 25, &mac_verify_req );
        TEST_ASSERT_EQUAL( 0, ret );
        ret = fcs_remove_service_key( sess_id, 25);
        TEST_ASSERT_EQUAL( 0, ret );
    }
	if( key != NULL )
	{
		vPortFree(key);
	}
	if (hash_data != NULL)
	{
		vPortFree(hash_data);
	}
	if( src_data != NULL )
	{
		vPortFree(src_data);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test signing and verification
 * functions
 */
TEST( TEST_FCS, AFQP_Fcs_SignTests )
{
    int ret, i;
    uint32_t sign_size, context_id, pubkey_size, bytes_read, status_size;
    char *file_name = "/ns256", *src_data, *sign_data, *key,
            *pubkey_data, res_match_status[4];
	struct fcs_ecdsa_req ecdsa_req;
	struct fcs_ecdsa_verify_req ecdsa_ver_req;

    INFO("Starting signing tests");
    key = (char*)pvPortMalloc(FCS_MAX_KEY_SIZE);
    TEST_ASSERT_NOT_EQUAL(NULL, key);
    sign_data = (char*)pvPortMalloc(FCS_MAX_SIG_SIZE);
    TEST_ASSERT_NOT_EQUAL(NULL, sign_data);
	src_data = (char*)pvPortMalloc(64);
	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
    if (TEST_PROTECT())
    {
        context_id = 1;
        for (i = 0; i < 64; i++)
        {
            src_data[i] = i;
        }
        bytes_read = fcs_read( file_name, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );

		ecdsa_req.ecc_curve = FCS_ECC_NISTP_256;
		ecdsa_req.src = src_data;
		ecdsa_req.src_len = 32;
		ecdsa_req.dst = sign_data;
		ecdsa_req.dst_len = &sign_size;
        ret = fcs_ecdsa_hash_sign( sess_id, context_id, 32, &ecdsa_req );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 0x40, sign_size );

        context_id++;
		ecdsa_ver_req.ecc_curve = FCS_ECC_NISTP_256;
    	ecdsa_ver_req.src = src_data;
    	ecdsa_ver_req.src_len = 32; 
    	ecdsa_ver_req.signature = sign_data;
    	ecdsa_ver_req.signature_len = sign_size;
    	ecdsa_ver_req.pubkey = NULL;
    	ecdsa_ver_req.pubkey_len = 0;
    	ecdsa_ver_req.dst = res_match_status;
    	ecdsa_ver_req.dst_len = &status_size;
        ret = fcs_ecdsa_hash_verify( sess_id, context_id, 32, &ecdsa_ver_req );
        TEST_ASSERT_EQUAL( 0, ret );
        ret = fcs_remove_service_key( sess_id, 32);
        TEST_ASSERT_EQUAL( 0, ret );

        /* testing these algorithms with public key method */
        pubkey_data = (char *)pvPortMalloc(FCS_MAX_PUBKEY_SIZE);
        TEST_ASSERT_NOT_EQUAL(NULL, pubkey_data);
        if (pubkey_data != NULL)
        {
            file_name = "/bp256";
            bytes_read = fcs_read( file_name, (void*)key );
            TEST_ASSERT_NOT_EQUAL(0, bytes_read);
            ret = fcs_import_service_key( sess_id, key,
                    bytes_read, NULL, 0 );
            TEST_ASSERT_EQUAL( 0, ret );

			ecdsa_req.ecc_curve = FCS_ECC_BRAINPOOL_256;
			ecdsa_req.src_len = 32;
			ret = fcs_ecdsa_hash_sign( sess_id, context_id, 42, 
					&ecdsa_req );
            TEST_ASSERT_EQUAL( 0, ret );
            TEST_ASSERT_EQUAL( 0x40, sign_size );

            context_id++;
            ret = fcs_ecdsa_get_pub_key( sess_id, context_id,
                    42, 0x3, pubkey_data, &pubkey_size );
            TEST_ASSERT_EQUAL( 0, ret );
            if (TEST_PROTECT())
            {
				context_id++;
				ecdsa_ver_req.ecc_curve = FCS_ECC_BRAINPOOL_256;
				ecdsa_ver_req.src_len = 32; 
				ecdsa_ver_req.signature_len = sign_size;
				ecdsa_ver_req.pubkey = pubkey_data;
				ecdsa_ver_req.pubkey_len = pubkey_size;
				ret = fcs_ecdsa_hash_verify( sess_id, context_id, 0, &ecdsa_ver_req );
                TEST_ASSERT_EQUAL( 0, ret );
            }
            ret = fcs_remove_service_key( sess_id, 42);
            TEST_ASSERT_EQUAL( 0, ret );
			if(pubkey_data != NULL)
			{
				vPortFree(pubkey_data);
			}
        }
    }
	if (key != NULL)
	{
		vPortFree(key);
	}
	if (src_data != NULL)
	{
		vPortFree(src_data);
	}
	if (sign_data != NULL)
	{
		vPortFree(sign_data);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test SHA2 signing and verification
 * functions
 */
TEST( TEST_FCS, AFQP_Fcs_Sha2SignTests )
{
    int ret, i;
    uint32_t sign_size, context_id, pubkey_size, bytes_read,
            status_size;
    char *file_name = "/ns256", *src_data, *sign_data,
            *pubkey_data, *key, res_match_status[4];
	struct fcs_ecdsa_req ecdsa_req;
	struct fcs_ecdsa_verify_req ecdsa_ver_req;

	key = (char*)pvPortMalloc(240);
    TEST_ASSERT_NOT_EQUAL(NULL, key);
	src_data = (char*)pvPortMalloc(300);
	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
    sign_data = (char*)pvPortMalloc(100);
    TEST_ASSERT_NOT_EQUAL(NULL, sign_data);
    if (TEST_PROTECT())
    {
        context_id = 1;
        for (i = 0; i < 128; i++)
        {
            src_data[i] = i;
        }
        bytes_read = fcs_read( file_name, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );

		ecdsa_req.ecc_curve = FCS_ECC_NISTP_256;
		ecdsa_req.src = src_data;
		ecdsa_req.src_len = 128;
		ecdsa_req.dst = sign_data;
		ecdsa_req.dst_len = &sign_size;
        ret = fcs_ecdsa_sha2_data_sign( sess_id, context_id, 32,
                &ecdsa_req );
        TEST_ASSERT_EQUAL( 0, ret );
        TEST_ASSERT_EQUAL( 0x40, sign_size );
        
		context_id++;
		ecdsa_ver_req.ecc_curve = FCS_ECC_NISTP_256;
    	ecdsa_ver_req.src = src_data;
    	ecdsa_ver_req.src_len = 128; 
    	ecdsa_ver_req.signature = sign_data;
    	ecdsa_ver_req.signature_len = sign_size;
    	ecdsa_ver_req.pubkey = NULL;
    	ecdsa_ver_req.pubkey_len = 0;
    	ecdsa_ver_req.dst = res_match_status;
    	ecdsa_ver_req.dst_len = &status_size;
        ret = fcs_ecdsa_sha2_data_verify( sess_id, context_id,
                32, &ecdsa_ver_req );
        TEST_ASSERT_EQUAL( 0, ret );
        
		ret = fcs_remove_service_key( sess_id, 32);
        TEST_ASSERT_EQUAL( 0, ret );

        /* testing these algorithms with public key method */
        pubkey_data = (char *)pvPortMalloc(100);
        TEST_ASSERT_NOT_EQUAL(NULL, pubkey_data);
        if (pubkey_data != NULL)
        {
            file_name = "/bp256";
            bytes_read = fcs_read( file_name, (void*)key );
            TEST_ASSERT_NOT_EQUAL(0, bytes_read);
            ret = fcs_import_service_key( sess_id, key,
                    bytes_read, NULL, 0 );
            TEST_ASSERT_EQUAL( 0, ret );

			ecdsa_req.ecc_curve = FCS_ECC_BRAINPOOL_256;
			ret = fcs_ecdsa_sha2_data_sign( sess_id, context_id, 42,
                &ecdsa_req );
            TEST_ASSERT_EQUAL( 0, ret );
            TEST_ASSERT_EQUAL( 0x40, sign_size );

            context_id++;
            ret = fcs_ecdsa_get_pub_key( sess_id, context_id,
                    42, 0x3, pubkey_data, &pubkey_size );
            TEST_ASSERT_EQUAL( 0, ret );
            if (TEST_PROTECT())
            {
				context_id++;
				ecdsa_ver_req.ecc_curve = FCS_ECC_BRAINPOOL_256;
				ecdsa_ver_req.signature_len = sign_size;
				ecdsa_ver_req.pubkey = pubkey_data;
				ecdsa_ver_req.pubkey_len = pubkey_size;
                ret = fcs_ecdsa_sha2_data_verify( sess_id,
                        context_id, 0, &ecdsa_ver_req );
                TEST_ASSERT_EQUAL( 0, ret );
            }
            ret = fcs_remove_service_key( sess_id, 42);
            TEST_ASSERT_EQUAL( 0, ret );
			if(pubkey_data != NULL)
			{
				vPortFree(pubkey_data);
			}
        }
    }
	if (key != NULL)
	{
		vPortFree(key);
	}
	if (src_data != NULL)
	{
		vPortFree(src_data);
	}
	if (sign_data != NULL)
	{
		vPortFree(sign_data);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test FCS public key request and shared
 * secret generation
 */
TEST( TEST_FCS, AFQP_Fcs_PublicKeyTests )
{
    INFO("Importing ECDSA NIST-P 256 key");
    int ret;
    uint32_t context_id = 1, shrd_sec_size, pubkey_size = 256,
            bytes_read;
    char *pubkey_data, *ecdsakeyfile = "/ns256",
            *ecdsaexchangefile = "/nse256",
            *shrd_secret, *key;
	struct fcs_ecdh_req ecdh_req;
    
	pubkey_data = (char *)pvPortMalloc(pubkey_size);
    TEST_ASSERT_NOT_EQUAL(NULL, pubkey_data);
    key = (char*)pvPortMalloc(240);
    TEST_ASSERT_NOT_EQUAL( NULL, key );
	shrd_secret = (char *)pvPortMalloc(128);
	TEST_ASSERT_NOT_EQUAL(NULL, shrd_secret);
    if (pubkey_data != NULL && shrd_secret != NULL)
    {
        bytes_read = fcs_read( ecdsakeyfile, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );
        if (TEST_PROTECT())
        {
            ret = fcs_ecdsa_get_pub_key( sess_id, context_id,
                    32, FCS_ECC_NISTP_256, pubkey_data, &pubkey_size );
            TEST_ASSERT_EQUAL( 0, ret );
            if (TEST_PROTECT())
            {
                bytes_read = fcs_read( ecdsaexchangefile, (void*)key );
                TEST_ASSERT_NOT_EQUAL(0, bytes_read);
                ret = fcs_import_service_key( sess_id, key,
                        bytes_read, NULL, 0 );
                TEST_ASSERT_EQUAL( 0, ret );

                context_id++;
				ecdh_req.pubkey = pubkey_data;
				ecdh_req.pubkey_len = pubkey_size;
				ecdh_req.shared_secret = shrd_secret;
				ecdh_req.shared_secret_len = &shrd_sec_size;
				ecdh_req.ecc_curve = FCS_ECC_NISTP_256;
                ret = fcs_ecdh_request( sess_id, 34, context_id,
                        &ecdh_req );
                TEST_ASSERT_EQUAL( 0, ret );
                ret = fcs_remove_service_key( sess_id, 34);
                TEST_ASSERT_EQUAL( 0, ret );
            }
            ret = fcs_remove_service_key( sess_id, 32);
            TEST_ASSERT_EQUAL( 0, ret );
        }
        ecdsakeyfile = "/ns384";
        ecdsaexchangefile = "/nse384";
        bytes_read = fcs_read( ecdsakeyfile, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );
        if (TEST_PROTECT())
        {
            ret = fcs_ecdsa_get_pub_key( sess_id, context_id,
                    33, FCS_ECC_NISTP_384, pubkey_data, &pubkey_size );
            TEST_ASSERT_EQUAL( 0, ret );
            if (TEST_PROTECT())
            {
                bytes_read = fcs_read( ecdsaexchangefile, (void*)key );
                TEST_ASSERT_NOT_EQUAL(0, bytes_read);
                ret = fcs_import_service_key( sess_id, key,
                        bytes_read, NULL, 0 );
                TEST_ASSERT_EQUAL( 0, ret );

                context_id++;
				ecdh_req.pubkey = pubkey_data;
				ecdh_req.pubkey_len = pubkey_size;
				ecdh_req.shared_secret = shrd_secret;
				ecdh_req.shared_secret_len = &shrd_sec_size;
				ecdh_req.ecc_curve = FCS_ECC_NISTP_384;
                ret = fcs_ecdh_request( sess_id, 35, context_id,
					&ecdh_req );
                TEST_ASSERT_EQUAL( 0, ret );
                ret = fcs_remove_service_key( sess_id, 35);
                TEST_ASSERT_EQUAL( 0, ret );
            }
            ret = fcs_remove_service_key( sess_id, 33);
            TEST_ASSERT_EQUAL( 0, ret );
        }

        ecdsakeyfile = "/bp256";
        ecdsaexchangefile = "/bpe256";
        bytes_read = fcs_read( ecdsakeyfile, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );
        if (TEST_PROTECT())
        {
            ret = fcs_ecdsa_get_pub_key( sess_id, context_id,
                    42, FCS_ECC_BRAINPOOL_256, pubkey_data, &pubkey_size );
            TEST_ASSERT_EQUAL( 0, ret );
            if (TEST_PROTECT())
            {
                bytes_read = fcs_read( ecdsaexchangefile, (void*)key );
                TEST_ASSERT_NOT_EQUAL(0, bytes_read);
                ret = fcs_import_service_key( sess_id, key,
                        bytes_read, NULL, 0 );
                TEST_ASSERT_EQUAL( 0, ret );

                context_id++;
				ecdh_req.pubkey = pubkey_data;
				ecdh_req.pubkey_len = pubkey_size;
				ecdh_req.shared_secret = shrd_secret;
				ecdh_req.shared_secret_len = &shrd_sec_size;
				ecdh_req.ecc_curve = FCS_ECC_BRAINPOOL_256;
                ret = fcs_ecdh_request( sess_id, 44, context_id,
					&ecdh_req );
                TEST_ASSERT_EQUAL( 0, ret );
                ret = fcs_remove_service_key( sess_id, 44);
                TEST_ASSERT_EQUAL( 0, ret );
            }
            ret = fcs_remove_service_key( sess_id, 42);
            TEST_ASSERT_EQUAL( 0, ret );
        }

        ecdsakeyfile = "/bp384";
        ecdsaexchangefile = "/bpe384";
        bytes_read = fcs_read( ecdsakeyfile, (void*)key );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );
        if (TEST_PROTECT())
        {
            ret = fcs_ecdsa_get_pub_key( sess_id, context_id,
                    43, FCS_ECC_BRAINPOOL_384, pubkey_data, &pubkey_size );
            TEST_ASSERT_EQUAL( 0, ret );
            if (TEST_PROTECT())
            {
                bytes_read = fcs_read( ecdsaexchangefile, (void*)key );
                TEST_ASSERT_NOT_EQUAL(0, bytes_read);
                ret = fcs_import_service_key( sess_id, key,
                        bytes_read, NULL, 0 );
                TEST_ASSERT_EQUAL( 0, ret );

                context_id++;

				ecdh_req.pubkey = pubkey_data;
				ecdh_req.pubkey_len = pubkey_size;
				ecdh_req.shared_secret = shrd_secret;
				ecdh_req.shared_secret_len = &shrd_sec_size;
				ecdh_req.ecc_curve = FCS_ECC_BRAINPOOL_384;
                ret = fcs_ecdh_request( sess_id, 45, context_id,
					&ecdh_req );
                TEST_ASSERT_EQUAL( 0, ret );
                ret = fcs_remove_service_key( sess_id, 45);
                TEST_ASSERT_EQUAL( 0, ret );
            }
            ret = fcs_remove_service_key( sess_id, 43);
            TEST_ASSERT_EQUAL( 0, ret );
        }
    }
	if (key != NULL)
    {
	   vPortFree(key);
    }
	if (shrd_secret != NULL)
	{
		vPortFree(shrd_secret);
	}
	if(pubkey_data != NULL)
	{
		vPortFree(pubkey_data);
	}
	
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test FCS HKDF request
 */
TEST( TEST_FCS, AFQP_Fcs_HKDFTests )
{
	char *output_key = "/hme384", *ecdsakeyfile = "/bp384",
			*ecdsaexchangefile = "/bpe384",
			*key_buf, *input_data,
            *fixed_info = "48||HMAC||test_key||0x5||0x1", *pubkey;
	uint32_t pubkey_size, shrd_sec_size;
    struct fcs_hkdf_req hkdf_req;
	struct fcs_ecdh_req ecdh_req;
    uint32_t bytes_read, context_id = 1, status;
    int ret;

	/* store both the shared_secret and input_data */
    input_data = (char *)pvPortMalloc(96);
	pubkey = (char *)pvPortMalloc(120);
	key_buf = (char *)pvPortMalloc(FCS_MAX_KEY_SIZE); 
    TEST_ASSERT_NOT_EQUAL(NULL, input_data);
	TEST_ASSERT_NOT_EQUAL(NULL, pubkey);
	TEST_ASSERT_NOT_EQUAL(NULL, key_buf);
    memset(input_data, 0, 80);
	memset(key_buf, 0, FCS_MAX_KEY_SIZE);

	if(TEST_PROTECT())
	{
        bytes_read = fcs_read( ecdsakeyfile, (void*)key_buf );
        TEST_ASSERT_NOT_EQUAL(0, bytes_read);
        ret = fcs_import_service_key( sess_id, key_buf, bytes_read,
                NULL, 0 );
        TEST_ASSERT_EQUAL( 0, ret );
        if (TEST_PROTECT())
        {
            ret = fcs_ecdsa_get_pub_key( sess_id, context_id,
                    43, FCS_ECC_BRAINPOOL_384, pubkey, &pubkey_size );
            TEST_ASSERT_EQUAL( 0, ret );
            if (TEST_PROTECT())
            {
                bytes_read = fcs_read( ecdsaexchangefile, (void*)key_buf );
                TEST_ASSERT_NOT_EQUAL(0, bytes_read);
                ret = fcs_import_service_key( sess_id, key_buf,
                        bytes_read, NULL, 0 );
                TEST_ASSERT_EQUAL( 0, ret );

                context_id++;
				ecdh_req.pubkey = pubkey;
				ecdh_req.pubkey_len = pubkey_size;
				ecdh_req.shared_secret = input_data;
				ecdh_req.shared_secret_len = &shrd_sec_size;
				ecdh_req.ecc_curve = FCS_ECC_BRAINPOOL_384;
                ret = fcs_ecdh_request( sess_id, 45, context_id,
					&ecdh_req );
                TEST_ASSERT_EQUAL( 0, ret );
                ret = fcs_remove_service_key( sess_id, 45);
                TEST_ASSERT_EQUAL( 0, ret );
            }
            ret = fcs_remove_service_key( sess_id, 43);
            TEST_ASSERT_EQUAL( 0, ret );
		}
    	bytes_read = fcs_read(output_key, (void*)key_buf);
		TEST_ASSERT_NOT_EQUAL(0, bytes_read);

		TEST_ASSERT_EQUAL( 0, ret );

    	hkdf_req.step_type = FCS_EXTRACT_AND_EXPAND; 
    	hkdf_req.mac_mode = FCS_HKDF_384;
    	hkdf_req.input1 = input_data;
    	hkdf_req.input1_len = FCS_SHARED_SEC_SIZE;
    	hkdf_req.input2 = fixed_info;
   		hkdf_req.input2_len = strlen(fixed_info);
    	hkdf_req.output_key_obj = key_buf;
    	hkdf_req.output_key_obj_len = bytes_read;
		hkdf_req.hkdf_resp = &status;
 
    	ret = fcs_hkdf_request(sess_id, 0, &hkdf_req);
		TEST_ASSERT_EQUAL( 0, ret );
		vPortFree(key_buf);
		vPortFree(input_data);
		vPortFree(pubkey);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_open_service_session
 */
TEST( TEST_FCS, AFQP_Fcs_OpenSessionFuzzing )
{
    int32_t ret;

    ret = fcs_open_service_session(NULL);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_close_service_session
 */
TEST( TEST_FCS, AFQP_Fcs_CloseSessionFuzzing )
{
    int32_t ret;

    ret = fcs_close_service_session(NULL);
    TEST_ASSERT_NOT_EQUAL(0, ret);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to peform fuzzing tests on fcs_random_number_ext
 */
TEST( TEST_FCS, AFQP_Fcs_RandomNumberFuzzing )
{
	int ret;
	uint32_t context_id = 1;
	char *random_data = NULL;
	ret = fcs_random_number_ext(sess_id, context_id,
			NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	
	random_data = (char*)pvPortMalloc(FCS_RNG_MAX_SIZE);
	TEST_ASSERT_NOT_EQUAL(NULL, random_data);
	ret = fcs_random_number_ext(NULL, context_id,
		random_data, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	ret = fcs_random_number_ext(sess_id, context_id,
			random_data, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	
	/* exceeding size limit */
	ret = fcs_random_number_ext(sess_id, context_id,
			random_data, FCS_RNG_MAX_SIZE + 4);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(random_data != NULL)
	{
		vPortFree(random_data);
	}
}
/*-----------------------------------------------------------*/


/**
 * @brief Test Function to test fuzzing of fcs_import_service_key
 */
TEST( TEST_FCS, AFQP_Fcs_ImportServiceKeyFuzzing )
{
	int ret;
	char *key = NULL;

	key = (char*)pvPortMalloc(FCS_MAX_KEY_SIZE);
	TEST_ASSERT_NOT_EQUAL(NULL, key);

	ret = fcs_import_service_key(NULL, key, 88,
		NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	ret = fcs_import_service_key(sess_id, NULL, 0,
			NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	ret = fcs_import_service_key(sess_id, key, 0,
			NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/*greater than maximum supported key size*/
	ret = fcs_import_service_key(NULL, key, 0xA9,
		NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(key != NULL)
	{
		vPortFree(key);

	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_export_service_key
 */
TEST( TEST_FCS, AFQP_Fcs_ExportServiceKeyFuzzing )
{
	int ret;
	uint32_t key_id = 12;
	char *key = NULL;
	uint32_t key_len = 0;

	key = (char*)pvPortMalloc(FCS_MAX_KEY_SIZE);
	TEST_ASSERT_NOT_EQUAL(NULL, key);

	/* Test with NULL session ID */
	ret = fcs_export_service_key(NULL, key_id, key, &key_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL key buffer */
	ret = fcs_export_service_key(sess_id, key_id, NULL, &key_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL key length pointer */
	ret = fcs_export_service_key(sess_id, key_id, key, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(key != NULL)
	{
		vPortFree(key);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_remove_service_key
 */
TEST( TEST_FCS, AFQP_Fcs_RemoveServiceKeyFuzzing )
{
	int ret;

	/* Test with NULL session ID */
	ret = fcs_remove_service_key(NULL, 0xB);
	TEST_ASSERT_NOT_EQUAL(0, ret);

}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_get_service_key_info
 */
TEST( TEST_FCS, AFQP_Fcs_GetServiceKeyInfoFuzzing )
{
	int ret;
	uint32_t key_id = 0xB;
	char *key_info = (char *)pvPortMalloc(64);
	uint32_t key_info_len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, key_info);

	/* Test with NULL session ID */
	ret = fcs_get_service_key_info(NULL, key_id, key_info, &key_info_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL key_info buffer */
	ret = fcs_get_service_key_info(sess_id, key_id, NULL, &key_info_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL key_info_len pointer */
	ret = fcs_get_service_key_info(sess_id, key_id, key_info, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(key_info != NULL)
	{
		vPortFree(key_info);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_import_service_key
 */
TEST( TEST_FCS, AFQP_Fcs_CreateServiceKeyFuzzing )
{
	int ret;
	char *key = NULL;

	key = (char*)pvPortMalloc(FCS_MAX_KEY_SIZE);
	TEST_ASSERT_NOT_EQUAL(NULL, key);

	ret = fcs_create_service_key(NULL, key, 88,
		NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	ret = fcs_create_service_key(sess_id, NULL, 0,
			NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	ret = fcs_create_service_key(sess_id, key, 0,
			NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/*greater than maximum supported key size*/
	ret = fcs_create_service_key(NULL, key, 0xA9,
		NULL, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(key != NULL)
	{
		vPortFree(key);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_get_digest
 */
TEST( TEST_FCS, AFQP_Fcs_GetDigestFuzzing )
{
	int32_t ret;
	struct fcs_digest_get_req digest_req;
	uint32_t context_id = 1;
	char *src_data = (char *)pvPortMalloc(64);
	char *digest = (char *)pvPortMalloc(64);
	uint32_t digest_len = 0;

	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	TEST_ASSERT_NOT_EQUAL(NULL, digest);

	memset(src_data, 0xA5, 64);

	/* Test with NULL session ID */
	ret = fcs_get_digest(NULL, context_id, 12, &digest_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL source buffer */
	digest_req.src = NULL;
	digest_req.src_len = 64;
	digest_req.digest = digest;
	digest_req.digest_len = &digest_len;
	digest_req.sha_digest_sz = FCS_DIGEST_SIZE_256;
	digest_req.sha_op_mode = FCS_DIGEST_OPMODE_SHA2;
	ret = fcs_get_digest(sess_id, context_id, 12, &digest_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source size */
	digest_req.src = src_data;
	digest_req.src_len = 0;
	ret = fcs_get_digest(sess_id, context_id, 12, &digest_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL digest buffer */
	digest_req.src_len = 64;
	digest_req.digest = NULL;
	ret = fcs_get_digest(sess_id, context_id, 12, &digest_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL digest length pointer */
	digest_req.digest = digest;
	digest_req.digest_len = NULL;
	ret = fcs_get_digest(sess_id, context_id, 12, &digest_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	
	/* Test with invalid op_mode */
	digest_req.digest_len = &digest_len;
	digest_req.sha_op_mode = 0xFF; // Invalid op_mode
	ret = fcs_get_digest(sess_id, context_id, 12, &digest_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);
	
	/* Test with invalid dig_sz */
	digest_req.sha_op_mode = FCS_DIGEST_OPMODE_SHA2;
	digest_req.sha_digest_sz = 0xFF; // Invalid dig_sz
	ret = fcs_get_digest(sess_id, context_id, 12, &digest_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with no key but HMAC mode*/
	digest_req.sha_op_mode = FCS_DIGEST_OPMODE_HMAC;
	digest_req.sha_digest_sz = FCS_DIGEST_SIZE_256; // Invalid dig_sz
	ret = fcs_get_digest(sess_id, context_id, 0, &digest_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(src_data);
	vPortFree(digest);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_service_get_provision_data
 */
TEST( TEST_FCS, AFQP_Fcs_ServiceGetProvisionDataFuzzing )
{
	int32_t ret;
	char *buffer = (char *)pvPortMalloc(64);
	uint32_t pd_size = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, buffer);

	/* Test with NULL buffer */
	ret = fcs_service_get_provision_data(NULL, &pd_size);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL size pointer */
	ret = fcs_service_get_provision_data(buffer, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(buffer != NULL)
	{
		vPortFree(buffer);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_service_counter_set
 */
TEST( TEST_FCS, AFQP_Fcs_ServiceCounterSetFuzzing )
{
	int32_t ret;
	char *buffer = (char *)pvPortMalloc(64);
	char *cert_status = (char *)pvPortMalloc(64);
	uint32_t size = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, buffer);
	TEST_ASSERT_NOT_EQUAL(NULL, cert_status);

	memset(buffer, 0xA5, 64);

	/* Test with NULL buffer */
	ret = fcs_service_counter_set(NULL, size, 0, cert_status);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with size 0 */
	ret = fcs_service_counter_set(buffer, 0, 0, cert_status);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid test parameter */
	ret = fcs_service_counter_set(buffer, size, 2, cert_status);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(buffer != NULL)
	{
		vPortFree(buffer);
	}
	if(cert_status != NULL)
	{
		vPortFree(cert_status);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_service_counter_set_preauthorized
 */
TEST( TEST_FCS, AFQP_Fcs_ServiceCounterSetPreauthorizedFuzzing )
{
	int32_t ret;
	uint32_t value = 0;
	int32_t test = 1;

	/* Test with invalid type */
	ret = fcs_service_counter_set_preauthorized(0, value, test);
	TEST_ASSERT_NOT_EQUAL(0, ret);
}

/**
 * @brief Test Function to test fuzzing of fcs_mac_verify
 */
TEST( TEST_FCS, AFQP_Fcs_MacVerifyFuzzing )
{
	int32_t ret;
	struct fcs_mac_verify_req mac_verify_req;
	uint32_t context_id = 1;
	char *src_data = (char *)pvPortMalloc(64);
	char *dst_data = (char *)pvPortMalloc(64);
	uint32_t dst_len = 0;

	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	TEST_ASSERT_NOT_EQUAL(NULL, dst_data);

	memset(src_data, 0xA5, 64);

	/* Test with NULL session ID */
	ret = fcs_mac_verify(NULL, context_id, 12, &mac_verify_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL source buffer */
	mac_verify_req.src = NULL;
	mac_verify_req.src_sz = 64;
	mac_verify_req.dst = dst_data;
	mac_verify_req.dst_sz = &dst_len;
	mac_verify_req.user_data_sz = 32;
	
	ret = fcs_mac_verify(sess_id, context_id, 12, &mac_verify_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source size */
	mac_verify_req.src = src_data;
	mac_verify_req.src_sz = 0;
	ret = fcs_mac_verify(sess_id, context_id, 12, &mac_verify_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);


	/* Test with NULL destination buffer */
	mac_verify_req.src_sz = 64;
	mac_verify_req.dst = NULL;
	ret = fcs_mac_verify(sess_id, context_id, 12, &mac_verify_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination length pointer */
	mac_verify_req.dst = dst_data;
	mac_verify_req.dst_sz = NULL;
	ret = fcs_mac_verify(sess_id, context_id, 12, &mac_verify_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/*Test with 0 user data size*/
	mac_verify_req.dst_sz = NULL;
	mac_verify_req.user_data_sz = 0;
	ret = fcs_mac_verify(sess_id, context_id, 0, &mac_verify_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test to get invalid mac size*/
	mac_verify_req.user_data_sz = mac_verify_req.src_sz - 7;
	ret = fcs_mac_verify(sess_id, context_id, 0, &mac_verify_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/*Test with invalid digest size*/
	mac_verify_req.user_data_sz = 32;
	mac_verify_req.dig_sz = 0xFF;
	ret = fcs_mac_verify(sess_id, context_id, 0, &mac_verify_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(src_data != NULL)
	{
		vPortFree(src_data);
	}
	if(dst_data != NULL)
	{
		vPortFree(dst_data);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_sdos_encrypt
 */
TEST( TEST_FCS, AFQP_Fcs_SdosEncryptFuzzing )
{
	int32_t ret;
	struct fcs_sdos_enc_req enc_req;
	uint32_t context_id = 1;
	char *src_data = (char *)pvPortMalloc(64);
	char *dst_data = (char *)pvPortMalloc(64);
	uint32_t dst_len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	TEST_ASSERT_NOT_EQUAL(NULL, dst_data);

	memset(src_data, 0xA5, 64);

	/* Test with NULL session ID */
	ret = fcs_sdos_encrypt(NULL, context_id, &enc_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL source buffer */
	enc_req.src = NULL;
	enc_req.src_sz = 64;
	enc_req.dst = dst_data;
	enc_req.dst_sz = &dst_len;
	ret = fcs_sdos_encrypt(sess_id, context_id, &enc_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source size */
	enc_req.src = src_data;
	enc_req.src_sz = 0;
	ret = fcs_sdos_encrypt(sess_id, context_id, &enc_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination buffer */
	enc_req.src_sz = 64;
	enc_req.dst = NULL;
	ret = fcs_sdos_encrypt(sess_id, context_id, &enc_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination length pointer */
	enc_req.dst = dst_data;
	enc_req.dst_sz = NULL;
	ret = fcs_sdos_encrypt(sess_id, context_id, &enc_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(src_data != NULL)
	{
		vPortFree(src_data);
	}
	if(dst_data != NULL)
	{
		vPortFree(dst_data);
	}
}

/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_sdos_decrypt
 */
TEST( TEST_FCS, AFQP_Fcs_SdosDecryptFuzzing )
{
	int32_t ret;
	struct fcs_sdos_dec_req dec_req;
	uint32_t context_id = 1;
	char *src_data = (char *)pvPortMalloc(64);
	char *dst_data = (char *)pvPortMalloc(64);
	uint32_t dst_len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	TEST_ASSERT_NOT_EQUAL(NULL, dst_data);

	memset(src_data, 0xA5, 64);

	/* Test with NULL session ID */
	ret = fcs_sdos_decrypt(NULL, context_id, &dec_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL source buffer */
	dec_req.src = NULL;
	dec_req.src_sz = 64;
	dec_req.dst = dst_data;
	dec_req.dst_sz = &dst_len;
	ret = fcs_sdos_decrypt(sess_id, context_id, &dec_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source size */
	dec_req.src = src_data;
	dec_req.src_sz = 0;
	ret = fcs_sdos_decrypt(sess_id, context_id, &dec_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination buffer */
	dec_req.src = src_data;
	dec_req.dst = NULL;
	ret = fcs_sdos_decrypt(sess_id, context_id, &dec_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination length pointer */
	dec_req.dst = dst_data;
	dec_req.dst_sz = NULL;
	ret = fcs_sdos_decrypt(sess_id, context_id, &dec_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(src_data != NULL)
	{
		vPortFree(src_data);
	}
	if(dst_data != NULL)
	{
		vPortFree(dst_data);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_hkdf_request
 */
TEST( TEST_FCS, AFQP_Fcs_HkdfRequestFuzzing )
{
	int32_t ret;
	struct fcs_hkdf_req hkdf_req;
	uint32_t key_id = 1;
	char *input1 = (char *)pvPortMalloc(64);
	char *input2 = (char *)pvPortMalloc(64);
	char *output_key_obj = (char *)pvPortMalloc(64);
	uint32_t output_key_obj_len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, input1);
	TEST_ASSERT_NOT_EQUAL(NULL, input2);
	TEST_ASSERT_NOT_EQUAL(NULL, output_key_obj);

	memset(input1, 0xA5, 64);
	memset(input2, 0x5A, 64);

	/* Test with NULL session ID */
	ret = fcs_hkdf_request(NULL, key_id, &hkdf_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL input1 buffer */
	hkdf_req.input1 = NULL;
	hkdf_req.input1_len = 64;
	hkdf_req.input2 = input2;
	hkdf_req.input2_len = 64;
	hkdf_req.output_key_obj = output_key_obj;
	hkdf_req.output_key_obj_len = output_key_obj_len;
	hkdf_req.hkdf_resp = NULL;
	ret = fcs_hkdf_request(sess_id, key_id, &hkdf_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL output_key_obj buffer */
	hkdf_req.input2 = input2;
	hkdf_req.output_key_obj = NULL;
	ret = fcs_hkdf_request(sess_id, key_id, &hkdf_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 output_key_obj_len */
	hkdf_req.input2_len = 64;
	hkdf_req.output_key_obj_len = 0;
	ret = fcs_hkdf_request(sess_id, key_id, &hkdf_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	if(input1 != NULL)
	{
		vPortFree(input1);
	}
	if(input2 != NULL)
	{
		vPortFree(input2);
	}
	if(output_key_obj != NULL)
	{
		vPortFree(output_key_obj);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_aes_crypt
 */
TEST( TEST_FCS, AFQP_Fcs_AesCryptFuzzing )
{
	int32_t ret;
	uint32_t output_len, context_id = 1;
	struct fcs_aes_req aes_req;
	char *input_data, *output_data, *iv;

	input_data = (char *)pvPortMalloc(64);
	output_data = (char *)pvPortMalloc(64);
	iv = (char *)pvPortMalloc(16);

	TEST_ASSERT_NOT_EQUAL(NULL, input_data);
	TEST_ASSERT_NOT_EQUAL(NULL, output_data);
	TEST_ASSERT_NOT_EQUAL(NULL, iv);

	/* Test with NULL session ID */
	ret = fcs_aes_crypt(NULL, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL input buffer */
	aes_req.input = NULL;
	aes_req.ip_len = 64;
	aes_req.output = output_data;
	aes_req.op_len = &output_len;
	aes_req.crypt_mode = FCS_AES_ENCRYPT;
	aes_req.block_mode = FCS_AES_ECB;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 input length */
	aes_req.input = input_data;
	aes_req.ip_len = 0;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL output buffer */
	aes_req.ip_len = 64;
	aes_req.output = NULL;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL output length pointer */
	aes_req.output = output_data;
	aes_req.op_len = NULL;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid crypt mode */
	aes_req.op_len = &output_len;
	aes_req.crypt_mode = 0xFF; // Invalid mode
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid block mode */
	aes_req.crypt_mode = FCS_AES_ENCRYPT;
	aes_req.block_mode = 0xFF; // Invalid block mode
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid iv */
	aes_req.block_mode = FCS_AES_GCM;
	aes_req.iv = NULL; // Invalid IV
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	aes_req.iv = iv; // Invalid IV size
	aes_req.iv_len = 15;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	aes_req.iv_len = 16;
	aes_req.iv_source = 0xFF;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/*Test with invalid tag*/
	aes_req.iv_source = FCS_IV_EXTERNAL;
	/* only 16 byte tag is suppported*/
	aes_req.tag_len = FCS_AES_TAG_32;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/*Test with invalid AAD*/
	aes_req.tag_len = FCS_AES_TAG_128;
	/* one byte more than maximum supported*/
	aes_req.aad_len = 65536;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	aes_req.aad_len = 64;
	aes_req.aad = NULL;
	ret = fcs_aes_crypt(sess_id, 1, context_id, &aes_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(input_data);
	vPortFree(output_data);
	if(iv != NULL)
	{
		vPortFree(iv);
	}
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_ecdh_request
 */
TEST( TEST_FCS, AFQP_Fcs_EcdhRequestFuzzing )
{
	int32_t ret;
	struct fcs_ecdh_req ecdh_req;
	uint32_t context_id = 1, key_id = 1;
	char *pubkey = (char *)pvPortMalloc(64);
	char *shared_secret = (char *)pvPortMalloc(64);
	uint32_t shared_secret_len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, pubkey);
	TEST_ASSERT_NOT_EQUAL(NULL, shared_secret);

	memset(pubkey, 0xA5, 64);

	/* Test with NULL session ID */
	ret = fcs_ecdh_request(NULL, key_id, context_id, &ecdh_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL public key */
	ecdh_req.pubkey = NULL;
	ecdh_req.pubkey_len = 64;
	ecdh_req.shared_secret = shared_secret;
	ecdh_req.shared_secret_len = &shared_secret_len;
	ecdh_req.ecc_curve = FCS_ECC_NISTP_256;
	ret = fcs_ecdh_request(sess_id, key_id, context_id, &ecdh_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 public key length */
	ecdh_req.pubkey = pubkey;
	ecdh_req.pubkey_len = 0;
	ret = fcs_ecdh_request(sess_id, key_id, context_id, &ecdh_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL shared secret buffer */
	ecdh_req.pubkey_len = 64;
	ecdh_req.shared_secret = NULL;
	ret = fcs_ecdh_request(sess_id, key_id, context_id, &ecdh_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL shared secret length pointer */
	ecdh_req.shared_secret = shared_secret;
	ecdh_req.shared_secret_len = NULL;
	ret = fcs_ecdh_request(sess_id, key_id, context_id, &ecdh_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid ECC curve */
	ecdh_req.shared_secret_len = &shared_secret_len;
	ecdh_req.ecc_curve = 0xFF; // Invalid ECC curve
	ret = fcs_ecdh_request(sess_id, key_id, context_id, &ecdh_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(pubkey);
	vPortFree(shared_secret);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_get_chip_id
 */
TEST( TEST_FCS, AFQP_Fcs_GetChipIdFuzzing )
{
	int32_t ret;
	uint32_t chip_id_lo = 0, chip_id_hi = 0;

	/* Test with NULL lower chip ID pointer */
	ret = fcs_get_chip_id(NULL, &chip_id_hi);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL higher chip ID pointer */
	ret = fcs_get_chip_id(&chip_id_lo, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with both pointers NULL */
	ret = fcs_get_chip_id(NULL, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_attestation_get_certificate
 */
TEST( TEST_FCS, AFQP_Fcs_AttestationGetCertificateFuzzing )
{
	int32_t ret;
	char *cert = (char *)pvPortMalloc(256);
	uint32_t cert_size = 256;

	TEST_ASSERT_NOT_EQUAL(NULL, cert);

	/* Test with NULL certificate buffer */
	ret = fcs_attestation_get_certificate(0, NULL, &cert_size);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL certificate size pointer */
	ret = fcs_attestation_get_certificate(0, cert, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(cert);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_mctp_cmd_send
 */
TEST( TEST_FCS, AFQP_Fcs_MctpCmdSendFuzzing )
{
	int32_t ret;
	char *src = (char *)pvPortMalloc(64);
	char *dst = (char *)pvPortMalloc(64);
	uint32_t src_len = 64, dst_len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, src);
	TEST_ASSERT_NOT_EQUAL(NULL, dst);

	memset(src, 0xA5, 64);

	/* Test with NULL source buffer */
	ret = fcs_mctp_cmd_send(NULL, src_len, dst, &dst_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination buffer */
	ret = fcs_mctp_cmd_send(src, src_len, NULL, &dst_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination length pointer */
	ret = fcs_mctp_cmd_send(src, src_len, dst, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source length */
	ret = fcs_mctp_cmd_send(src, 0, dst, &dst_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(src);
	vPortFree(dst);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_get_jtag_idcode
 */
TEST( TEST_FCS, AFQP_Fcs_GetJtagIdcodeFuzzing )
{
	int32_t ret;

	/* Test with NULL jtag_idcode pointer */
	ret = fcs_get_jtag_idcode(NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_get_device_identity
 */
TEST( TEST_FCS, AFQP_Fcs_GetDeviceIdentityFuzzing )
{
	int32_t ret;
	char *dev_identity = (char *)pvPortMalloc(64);
	uint32_t dev_identity_length = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, dev_identity);

	/* Test with NULL device identity buffer */
	ret = fcs_get_device_identity(NULL, &dev_identity_length);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL device identity length pointer */
	ret = fcs_get_device_identity(dev_identity, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with both pointers NULL */
	ret = fcs_get_device_identity(NULL, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(dev_identity);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_qspi_read
 */
TEST( TEST_FCS, AFQP_Fcs_QspiReadFuzzing )
{
	int32_t ret;
	char *buffer = (char *)pvPortMalloc(64);
	uint32_t len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, buffer);

	/* Test with NULL buffer */
	ret = fcs_qspi_read(0x1000, NULL, len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 length */
	ret = fcs_qspi_read(0x1000, buffer, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with length exceeding maximum */
	ret = fcs_qspi_read(0x1000, buffer, 1025);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(buffer);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_qspi_write
 */
TEST( TEST_FCS, AFQP_Fcs_QspiWriteFuzzing )
{
	int32_t ret;
	char *buffer = (char *)pvPortMalloc(64);
	uint32_t len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, buffer);

	/* Test with NULL buffer */
	ret = fcs_qspi_write(0x1000, NULL, len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 length */
	ret = fcs_qspi_write(0x1000, buffer, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(buffer);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_qspi_erase
 */
TEST( TEST_FCS, AFQP_Fcs_QspiEraseFuzzing )
{
	int32_t ret;

	/* Test with unaligned QSPI address */
	ret = fcs_qspi_erase(0x1001, 0x1000);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with size not a multiple of 4KB */
	ret = fcs_qspi_erase(0x1000, 0x500);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with size 0 */
	ret = fcs_qspi_erase(0x1000, 0);
	TEST_ASSERT_NOT_EQUAL(0, ret);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_ecdsa_get_pub_key
 */
TEST( TEST_FCS, AFQP_Fcs_EcdsaGetPubKeyFuzzing )
{
	int32_t ret;
	uint32_t context_id = 1, key_id = 1, ecc_curve = FCS_ECC_NISTP_256;
	char *pubkey = (char *)pvPortMalloc(64);
	uint32_t pubkey_len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, pubkey);

	/* Test with NULL session ID */
	ret = fcs_ecdsa_get_pub_key(NULL, context_id, key_id, ecc_curve, pubkey, &pubkey_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL public key buffer */
	ret = fcs_ecdsa_get_pub_key(sess_id, context_id, key_id, ecc_curve, NULL, &pubkey_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL public key length pointer */
	ret = fcs_ecdsa_get_pub_key(sess_id, context_id, key_id, ecc_curve, pubkey, NULL);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid ECC curve */
	ret = fcs_ecdsa_get_pub_key(sess_id, context_id, key_id, 0xFF, pubkey, &pubkey_len);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(pubkey);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_ecdsa_hash_sign
 */
TEST( TEST_FCS, AFQP_Fcs_EcdsaHashSignFuzzing )
{
	int32_t ret;
	struct fcs_ecdsa_req ecdsa_req;
	uint32_t context_id = 1, key_id = 1;
	char *src_data = (char *)pvPortMalloc(64);
	char *dst_data = (char *)pvPortMalloc(64);
	uint32_t dst_len = 64;

	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	TEST_ASSERT_NOT_EQUAL(NULL, dst_data);

	/* Test with NULL session ID */
	ret = fcs_ecdsa_hash_sign(NULL, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL source buffer */
	ecdsa_req.src = NULL;
	ecdsa_req.src_len = 64;
	ecdsa_req.dst = dst_data;
	ecdsa_req.dst_len = &dst_len;
	ecdsa_req.ecc_curve = FCS_ECC_NISTP_256;
	ret = fcs_ecdsa_hash_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source size */
	ecdsa_req.src = src_data;
	ecdsa_req.src_len = 0;
	ret = fcs_ecdsa_hash_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination buffer */
	ecdsa_req.src_len = 64;
	ecdsa_req.dst = NULL;
	ret = fcs_ecdsa_hash_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination length pointer */
	ecdsa_req.dst = dst_data;
	ecdsa_req.dst_len = NULL;
	ret = fcs_ecdsa_hash_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid ECC curve */
	ecdsa_req.dst_len = &dst_len;
	ecdsa_req.ecc_curve = 0xFF; // Invalid ECC curve
	ret = fcs_ecdsa_hash_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(src_data);
	vPortFree(dst_data);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_ecdsa_hash_verify
 */
TEST( TEST_FCS, AFQP_Fcs_EcdsaHashVerifyFuzzing )
{
	int32_t ret;
	struct fcs_ecdsa_verify_req ecdsa_ver_req;
	uint32_t context_id = 1, key_id = 1;
	char *src_data = (char *)pvPortMalloc(64);
	char *signature = (char *)pvPortMalloc(64);
	char *pubkey = (char *)pvPortMalloc(64);
	uint32_t signature_len = 64, pubkey_len = 64;
	char res_match_status[4];
	uint32_t status_len = sizeof(res_match_status);

	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	TEST_ASSERT_NOT_EQUAL(NULL, signature);
	TEST_ASSERT_NOT_EQUAL(NULL, pubkey);

	/* Test with NULL session ID */
	ret = fcs_ecdsa_hash_verify(NULL, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL source buffer */
	ecdsa_ver_req.src = NULL;
	ecdsa_ver_req.src_len = 64;
	ecdsa_ver_req.signature = signature;
	ecdsa_ver_req.signature_len = signature_len;
	ecdsa_ver_req.pubkey = pubkey;
	ecdsa_ver_req.pubkey_len = pubkey_len;
	ecdsa_ver_req.dst = res_match_status;
	ecdsa_ver_req.dst_len = &status_len;
	ecdsa_ver_req.ecc_curve = FCS_ECC_NISTP_256;
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source size */
	ecdsa_ver_req.src = src_data;
	ecdsa_ver_req.src_len = 0;
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL signature buffer */
	ecdsa_ver_req.src_len = 64;
	ecdsa_ver_req.signature = NULL;
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 signature length */
	ecdsa_ver_req.signature = signature;
	ecdsa_ver_req.signature_len = 0;
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* 
	 * Test with NULL public key buffer 
	 * Public Key is only checked if we dont provide a key
	 */
	ecdsa_ver_req.signature_len = signature_len;
	ecdsa_ver_req.pubkey = NULL;
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, 0, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 public key length */
	ecdsa_ver_req.pubkey = pubkey;
	ecdsa_ver_req.pubkey_len = 0;
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, 0, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination buffer */
	ecdsa_ver_req.pubkey_len = pubkey_len;
	ecdsa_ver_req.dst = NULL;
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination length pointer */
	ecdsa_ver_req.dst = res_match_status;
	ecdsa_ver_req.dst_len = NULL;
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid ECC curve */
	ecdsa_ver_req.dst_len = &status_len;
	ecdsa_ver_req.ecc_curve = 0xFF; // Invalid ECC curve
	ret = fcs_ecdsa_hash_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(src_data);
	vPortFree(signature);
	vPortFree(pubkey);
}
/*-----------------------------------------------------------*/


/**
 * @brief Test Function to test fuzzing of fcs_ecdsa_sha2_data_sign
 */
TEST( TEST_FCS, AFQP_Fcs_EcdsaSha2DataSignFuzzing )
{
	int32_t ret;
	struct fcs_ecdsa_req ecdsa_req;
	uint32_t context_id = 1, key_id = 1;
	char *src_data = (char *)pvPortMalloc(128);
	char *dst_data = (char *)pvPortMalloc(128);
	uint32_t dst_len = 128;

	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	TEST_ASSERT_NOT_EQUAL(NULL, dst_data);

	memset(src_data, 0xA5, 128);

	/* Test with NULL session ID */
	ret = fcs_ecdsa_sha2_data_sign(NULL, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL source buffer */
	ecdsa_req.src = NULL;
	ecdsa_req.src_len = 128;
	ecdsa_req.dst = dst_data;
	ecdsa_req.dst_len = &dst_len;
	ecdsa_req.ecc_curve = FCS_ECC_NISTP_256;
	ret = fcs_ecdsa_sha2_data_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source size */
	ecdsa_req.src = src_data;
	ecdsa_req.src_len = 0;
	ret = fcs_ecdsa_sha2_data_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination buffer */
	ecdsa_req.src_len = 128;
	ecdsa_req.dst = NULL;
	ret = fcs_ecdsa_sha2_data_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination length pointer */
	ecdsa_req.dst = dst_data;
	ecdsa_req.dst_len = NULL;
	ret = fcs_ecdsa_sha2_data_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid ECC curve */
	ecdsa_req.dst_len = &dst_len;
	ecdsa_req.ecc_curve = 0xFF; // Invalid ECC curve
	ret = fcs_ecdsa_sha2_data_sign(sess_id, context_id, key_id, &ecdsa_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(src_data);
	vPortFree(dst_data);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test Function to test fuzzing of fcs_ecdsa_sha2_data_verify
 */
TEST( TEST_FCS, AFQP_Fcs_EcdsaSha2DataVerifyFuzzing )
{
	int32_t ret;
	struct fcs_ecdsa_verify_req ecdsa_ver_req;
	uint32_t context_id = 1, key_id = 1;
	char *src_data = (char *)pvPortMalloc(128);
	char *signature = (char *)pvPortMalloc(128);
	char *pubkey = (char *)pvPortMalloc(128);
	uint32_t signature_len = 128, pubkey_len = 128;
	char res_match_status[4];
	uint32_t status_len = sizeof(res_match_status);

	TEST_ASSERT_NOT_EQUAL(NULL, src_data);
	TEST_ASSERT_NOT_EQUAL(NULL, signature);
	TEST_ASSERT_NOT_EQUAL(NULL, pubkey);

	/* Test with NULL session ID */
	ret = fcs_ecdsa_sha2_data_verify(NULL, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL source buffer */
	ecdsa_ver_req.src = NULL;
	ecdsa_ver_req.src_len = 128;
	ecdsa_ver_req.signature = signature;
	ecdsa_ver_req.signature_len = signature_len;
	ecdsa_ver_req.pubkey = pubkey;
	ecdsa_ver_req.pubkey_len = pubkey_len;
	ecdsa_ver_req.dst = res_match_status;
	ecdsa_ver_req.dst_len = &status_len;
	ecdsa_ver_req.ecc_curve = FCS_ECC_NISTP_256;
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 source size */
	ecdsa_ver_req.src = src_data;
	ecdsa_ver_req.src_len = 0;
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL signature buffer */
	ecdsa_ver_req.src_len = 128;
	ecdsa_ver_req.signature = NULL;
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 signature length */
	ecdsa_ver_req.signature = signature;
	ecdsa_ver_req.signature_len = 0;
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL public key buffer */
	ecdsa_ver_req.signature_len = signature_len;
	ecdsa_ver_req.pubkey = NULL;
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, 0, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with 0 public key length */
	ecdsa_ver_req.pubkey = pubkey;
	ecdsa_ver_req.pubkey_len = 0;
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, 0, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination buffer */
	ecdsa_ver_req.pubkey_len = pubkey_len;
	ecdsa_ver_req.dst = NULL;
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with NULL destination length pointer */
	ecdsa_ver_req.dst = res_match_status;
	ecdsa_ver_req.dst_len = NULL;
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	/* Test with invalid ECC curve */
	ecdsa_ver_req.dst_len = &status_len;
	ecdsa_ver_req.ecc_curve = 0xFF; // Invalid ECC curve
	ret = fcs_ecdsa_sha2_data_verify(sess_id, context_id, key_id, &ecdsa_ver_req);
	TEST_ASSERT_NOT_EQUAL(0, ret);

	vPortFree(src_data);
	vPortFree(signature);
	vPortFree(pubkey);
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test fcs_close_session
 */
TEST( TEST_FCS, AFQP_Fcs_FcsCloseSession )
{
    int ret;

    ret = fcs_close_service_session(sess_id);

    TEST_ASSERT_EQUAL( 0, ret );
}
/*-----------------------------------------------------------*/

/**
 * @brief Test function to test fcs_deinit
 */
TEST( TEST_FCS, AFQP_Fcs_FcsDeinit )
{
    int ret;

    ret = fcs_deinit();
    TEST_ASSERT_EQUAL( 0, ret );
}
/*-----------------------------------------------------------*/


