/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * Header file for FCS driver
 */

/*
    File: FCS Low-Level Driver

    Header file for FCS driver.
*/

/*
    Constants: FCS constants

    FCS_MAX_INSTANCES                   - Value: 6U.
    FCS_GET_COMM_SOURCE                 - Expands to (((session_id) & 0xF0000000U) >> 28).
    FCS_COMM_SOURCE_HPS                 - Value: 2U.
    FCS_MAX_SIZE_HPS                    - Value: 0x10000000U.
    FCS_MAX_SIZE_FPGA                   - Value: 0x20000000U.
    FCS_EXPORT_KEY_MAX_SIZE             - Value: 364U.
    FCS_RESP_STATUS                     - Value: 0U.
    FCS_RESP_KEY_STATUS                 - Value: 1U.
    FCS_RESP_CERT_STATUS                - Value: 1U.
    FCS_RESP_SIZE                       - Value: 1U.
    FCS_RESP_DATA                       - Value: 3U.
    FCS_RESP_HEADER_SIZE                - Value: 12U.
    FCS_KEY_HEADER_SIZE                 - Value: 12U.
    FCS_UPDATE                          - Value: 0U.
    FCS_FINALIZE                        - Value: 1U.
    FCS_KEY_INFO_MAX_RESP               - Value: 20U.
    FCS_DIGEST_PARAM_SIZE               - Value: 4U.
    FCS_MAC_PARAM_SIZE                  - Value: 4U.
    FCS_MAC_VERIFY_RESP                 - Value: 16U.
    FCS_DIGEST_MAX_RESP                 - Value: 84.
    FCS_GCM_BLOCK_SIZE                  - Value: 16U.
    FCS_AES_GCM_MAX_SIZE                - Value: 0x10000U.
    FCS_AES_ECB_PARAM_SIZE              - Value: 8U.
    FCS_AES_MAX_PARAM_SIZE              - Value: 28U.
    FCS_AES_IV_SIZE                     - Value: 16U.
    FCS_AAD_MAX_SIZE                    - Value: 0xFFFFU.
    FCS_ECDSA_HASH_SIGN_MAX_RESP        - Value: 108U.
    FCS_ECDSA_HASH_SHA2_SIGN_MAX_RESP   - Value: 108U.
    FCS_ECDSA_HASH_VERIFY_RESP          - Value: 84U.
    FCS_ECDSA_HASH_SHA2_VERIFY_RESP     - Value: 84U.
    FCS_GET_PUBKEY_RESP                 - Value: 184U.
    FCS_ECDSA_PARAM_SIZE                - Value: 4U.
    FCS_OPEN_SESSION                    - Value: 0x4200013AU.
    FCS_CLOSE_SESSION                   - Value: 0x4200013BU.
    FCS_IMPORT_SERVICE_KEY              - Value: 0x4200013CU.
    FCS_EXPORT_SERVICE_KEY              - Value: 0x4200013DU.
    FCS_REMOVE_SERVICE_KEY              - Value: 0x4200013EU.
    FCS_GET_SERVICE_KEY_INFO            - Value: 0x4200013FU.
    FCS_CREATE_SERVICE_KEY              - Value: 0x42000167U.
    FCS_HKDF_REQUEST                    - Value: 0x42000166U.
    FCS_GET_DEVICE_IDENTITY             - Value: 0x420000D2U.
    FCS_GET_IDCODE                      - Value: 0x420000D3U.
    FCS_RANDOM_NUMBER                   - Value: 0x4200012DU.
    FCS_SDOS_CRYPTION                   - Value: 0x4200012FU.
    FCS_SEND_CERTIFICATE                - Value: 0x42000131U.
    FCS_GET_PROVISION_DATA              - Value: 0x42000132U.
    FCS_CNTR_SET_PREAUTH                - Value: 0x42000133U.
    FCS_GET_CHIP_ID                     - Value: 0x42000135U.
    FCS_GET_ATTESTATION_CERT            - Value: 0x42000138U.
    FCS_CREATE_CERT_ON_RELOAD           - Value: 0x42000139U.
    FCS_AES_INIT                        - Value: 0x42000140U.
    FCS_AES_UPDATE                      - Value: 0x42000141U.
    FCS_AES_FINALIZE                    - Value: 0x42000142U.
    FCS_GET_DIGEST_INIT                 - Value: 0x42000143U.
    FCS_GET_DIGEST_UPDATE               - Value: 0x42000144U.
    FCS_GET_DIGEST_FINALIZE             - Value: 0x42000145U.
    FCS_MAC_VERIFY_INIT                 - Value: 0x42000148U.
    FCS_MAC_VERIFY_UPDATE               - Value: 0x42000149U.
    FCS_MAC_VERIFY_FINALIZE             - Value: 0x4200014AU.
    FCS_ECDSA_HASH_SIGN_INIT            - Value: 0x4200014DU.
    FCS_ECDSA_HASH_SIGN_FINALIZE        - Value: 0x4200014EU.
    FCS_ECDSA_SHA2_SIGN_INIT            - Value: 0x4200014FU.
    FCS_ECDSA_SHA2_SIGN_UPDATE          - Value: 0x42000150U.
    FCS_ECDSA_SHA2_SIGN_FINALIZE        - Value: 0x42000151U.
    FCS_ECDSA_HASH_SIGN_VERIFY_INIT     - Value: 0x42000154U.
    FCS_ECDSA_HASH_SIGN_VERIFY_FINALIZE - Value: 0x42000155U.
    FCS_ECDSA_SHA2_SIGN_VERIFY_INIT     - Value: 0x42000156U.
    FCS_ECDSA_SHA2_SIGN_VERIFY_UPDATE   - Value: 0x42000157U.
    FCS_ECDSA_SHA2_SIGN_VERIFY_FINALIZE - Value: 0x42000158U.
    FCS_GET_PUBKEY_INIT                 - Value: 0x42000160U.
    FCS_GET_PUBKEY_FINALIZE             - Value: 0x42000161U.
    FCS_ECDH_INIT                       - Value: 0x42000162U.
    FCS_ECDH_FINALIZE                   - Value: 0x42000163U.
    FCS_MCTP_SEND_MSG                   - Value: 0x42000165U.
    FCS_QSPI_OPEN                       - Value: 0x420000CCU.
    FCS_QSPI_CLOSE                      - Value: 0x420000CDU.
    FCS_QSPI_CHIP_SELECT                - Value: 0x420000CEU.
    FCS_QSPI_ERASE                      - Value: 0x420000CFU.
    FCS_QSPI_WRITE                      - Value: 0x420000D0U.
    FCS_QSPI_READ                       - Value: 0x420000D1U.
    FCS_AES_BLOCK_MODE                  - Expands to (crypto_param & 0xFFU).
    FCS_SET_CRYPT_MODE                  - Expands to ((crypt_mode & 0x1U) << 8U).
    FCS_SET_TAG_LEN                     - Expands to ((crypto_param & 0xFFU) << 16U).
    FCS_SET_DIGEST_SIZE                 - Expands to ((crypto_param & 0xFFU) << 4U).
    FCS_SMMU_GET_ADDR                   - Expands to ((uint64_t)(uintptr_t)(addr - 0x80000000U)).
*/

/*
    Macros: GET definitions

    GET_QSPI_CHIP_SEL - Expands to ((chip_sel >> 28UL) & 0xFUL).
    GET_QSPI_MODE     - Expands to ((chip_sel >> 27UL) & 0x1U).
    GET_QSPI_CA       - Expands to ((chip_sel >> 26UL) & 0x1U).
*/

#ifndef _SOCFPGA_FCS_LL_H_
#define _SOCFPGA_FCS_LL_H_

#define FCS_MAX_INSTANCES    6U
#define FCS_GET_COMM_SOURCE(session_id)    (((session_id) & 0xF0000000U) >> 28)
#define FCS_COMM_SOURCE_HPS    2U
/*
 * if source is on HPS max size is usually 256MB
 * if source is on FPGA max size is usually 512MB
 */
#define FCS_MAX_SIZE_HPS           0x10000000U
#define FCS_MAX_SIZE_FPGA          0x20000000U
#define FCS_EXPORT_KEY_MAX_SIZE    364U
#define FCS_RESP_STATUS            0U
#define FCS_RESP_KEY_STATUS        1U
#define FCS_RESP_CERT_STATUS       1U
#define FCS_RESP_SIZE              1U
#define FCS_RESP_DATA              3U
#define FCS_RESP_HEADER_SIZE       12U
#define FCS_KEY_HEADER_SIZE        12U
#define FCS_UPDATE                 0U
#define FCS_FINALIZE               1U
/*0x5 are bits set for init and finalize*/
#define FCS_KEY_INFO_MAX_RESP                20U
#define FCS_DIGEST_PARAM_SIZE                4U
#define FCS_MAC_PARAM_SIZE                   4U
#define FCS_MAC_VERIFY_RESP                  16U
#define FCS_DIGEST_MAX_RESP                  84
#define FCS_GCM_BLOCK_SIZE                   16U
#define FCS_AES_GCM_MAX_SIZE                 0x10000U
#define FCS_AES_ECB_PARAM_SIZE               8U
#define FCS_AES_MAX_PARAM_SIZE               28U
#define FCS_AES_IV_SIZE                      16U
#define FCS_AAD_MAX_SIZE                     0xFFFFU
#define FCS_ECDSA_HASH_SIGN_MAX_RESP         108U
#define FCS_ECDSA_HASH_SHA2_SIGN_MAX_RESP    108U
#define FCS_ECDSA_HASH_VERIFY_RESP           84U
#define FCS_ECDSA_HASH_SHA2_VERIFY_RESP      84U
#define FCS_GET_PUBKEY_RESP                  184U
#define FCS_ECDSA_PARAM_SIZE                 4U
/* Function IDs for SMC calls*/
/* Session Management SMC function ids */
#define FCS_OPEN_SESSION     0x4200013AU
#define FCS_CLOSE_SESSION    0x4200013BU
/* Key Management SMC function ids */
#define FCS_IMPORT_SERVICE_KEY      0x4200013CU
#define FCS_EXPORT_SERVICE_KEY      0x4200013DU
#define FCS_REMOVE_SERVICE_KEY      0x4200013EU
#define FCS_GET_SERVICE_KEY_INFO    0x4200013FU
#define FCS_CREATE_SERVICE_KEY      0x42000167U
#define FCS_HKDF_REQUEST            0x42000166U

#define FCS_GET_DEVICE_IDENTITY                0x420000D2U
#define FCS_GET_IDCODE                         0x420000D3U
#define FCS_RANDOM_NUMBER                      0x4200012DU
#define FCS_SDOS_CRYPTION                      0x4200012FU
#define FCS_SEND_CERTIFICATE                   0x42000131U
#define FCS_GET_PROVISION_DATA                 0x42000132U
#define FCS_CNTR_SET_PREAUTH                   0x42000133U
#define FCS_GET_CHIP_ID                        0x42000135U
#define FCS_GET_ATTESTATION_CERT               0x42000138U
#define FCS_CREATE_CERT_ON_RELOAD              0x42000139U
#define FCS_AES_INIT                           0x42000140U
#define FCS_AES_UPDATE                         0x42000141U
#define FCS_AES_FINALIZE                       0x42000142U
#define FCS_GET_DIGEST_INIT                    0x42000143U
#define FCS_GET_DIGEST_UPDATE                  0x42000144U
#define FCS_GET_DIGEST_FINALIZE                0x42000145U
#define FCS_MAC_VERIFY_INIT                    0x42000148U
#define FCS_MAC_VERIFY_UPDATE                  0x42000149U
#define FCS_MAC_VERIFY_FINALIZE                0x4200014AU
#define FCS_ECDSA_HASH_SIGN_INIT               0x4200014DU
#define FCS_ECDSA_HASH_SIGN_FINALIZE           0x4200014EU
#define FCS_ECDSA_SHA2_SIGN_INIT               0x4200014FU
#define FCS_ECDSA_SHA2_SIGN_UPDATE             0x42000150U
#define FCS_ECDSA_SHA2_SIGN_FINALIZE           0x42000151U
#define FCS_ECDSA_HASH_SIGN_VERIFY_INIT        0x42000154U
#define FCS_ECDSA_HASH_SIGN_VERIFY_FINALIZE    0x42000155U
#define FCS_ECDSA_SHA2_SIGN_VERIFY_INIT        0x42000156U
#define FCS_ECDSA_SHA2_SIGN_VERIFY_UPDATE      0x42000157U
#define FCS_ECDSA_SHA2_SIGN_VERIFY_FINALIZE    0x42000158U
#define FCS_GET_PUBKEY_INIT                    0x42000160U
#define FCS_GET_PUBKEY_FINALIZE                0x42000161U
#define FCS_ECDH_INIT                          0x42000162U
#define FCS_ECDH_FINALIZE                      0x42000163U
#define FCS_MCTP_SEND_MSG                      0x42000165U
/* QSPI SMC function ids*/
#define FCS_QSPI_OPEN           0x420000CCU
#define FCS_QSPI_CLOSE          0x420000CDU
#define FCS_QSPI_CHIP_SELECT    0x420000CEU
#define FCS_QSPI_ERASE          0x420000CFU
#define FCS_QSPI_WRITE          0x420000D0U
#define FCS_QSPI_READ           0x420000D1U

#define FCS_AES_BLOCK_MODE(crypto_param)     (crypto_param & 0xFFU)
#define FCS_SET_CRYPT_MODE(crypt_mode)       ((crypt_mode & 0x1U) << 8U)
#define FCS_SET_TAG_LEN(crypto_param)        ((crypto_param & 0xFFU) << 16U)
#define FCS_SET_DIGEST_SIZE(crypto_param)    ((crypto_param & 0xFFU) << 4U)
#define FCS_SMMU_GET_ADDR(addr)              ((uint64_t)(uintptr_t)(addr - 0x80000000U))

#define GET_QSPI_CHIP_SEL(chip_sel)    ((chip_sel >> 28UL) & 0xFUL)
#define GET_QSPI_MODE(chip_sel)        ((chip_sel >> 27UL) & 0x1U)
#define GET_QSPI_CA(chip_sel)          ((chip_sel >> 26UL) & 0x1U)

#endif /* _SOCFPGA_FCS_LL_H_ */
