/*
 * SPDX-FileCopyrightText: Copyright (C) 2025 Altera Corporation
 *
 * SPDX-License-Identifier: MIT-0
 *
 * HAL Implementation for FCS driver
 */

#include <string.h>
#include <errno.h>
#include "socfpga_cache.h"
#include "socfpga_mbox_client.h"
#include "socfpga_fcs.h"
#include "socfpga_fcs_ll.h"
#include "osal.h"
#include "osal_log.h"

#define FCS_STATUS_MASK    0x7FFU

typedef struct
{
    char uuid[FCS_UUID_SIZE];
    uint32_t session_id;
    sdm_client_handle pxCryptoHandle;
} SessionHandleStruct;

struct fcs_service_descriptor
{
    osal_semaphore_t fcs_sem;
    SessionHandleStruct session_map[FCS_MAX_INSTANCES];
    sdm_client_handle security_handle;
    int session_count;
};
static struct fcs_service_descriptor *fcs_descriptor = NULL;

void fcs_callback( uint64_t *resp_values );

int fcs_init(void)
{
    int ret;
    if (fcs_descriptor == NULL)
    {
        fcs_descriptor = pvPortMalloc(sizeof(struct fcs_service_descriptor));
        if (fcs_descriptor == NULL)
        {
            ERROR("Failed to initialise FCS");
            return -ENOMEM;
        }
        fcs_descriptor->session_count = 0;
        (void) memset(fcs_descriptor->session_map, 0,
                sizeof(fcs_descriptor->session_map));
        ret = mbox_init();
        if (ret != 0)
        {
            ERROR("Aborting FCS initialization");
            (void) memset(fcs_descriptor, 0,
                    sizeof(struct fcs_service_descriptor));
            vPortFree(fcs_descriptor);
            fcs_descriptor = NULL;
            return -EIO;
        }

        fcs_descriptor->security_handle = mbox_open_client();
        if (fcs_descriptor->security_handle == NULL)
        {
            ERROR("Failed to open mailbox client");
            (void) memset(fcs_descriptor, 0,
                    sizeof(struct fcs_service_descriptor));
            vPortFree(fcs_descriptor);
            fcs_descriptor = NULL;
            return -EIO;
        }
        ret = mbox_set_callback(fcs_descriptor->security_handle, fcs_callback);
        fcs_descriptor->fcs_sem = osal_semaphore_create(NULL);
        if (fcs_descriptor->fcs_sem == NULL)
        {
            ERROR("Failed to initialise semaphore");
            (void) memset(fcs_descriptor, 0,
                    sizeof(struct fcs_service_descriptor));
            vPortFree(fcs_descriptor);
            fcs_descriptor = NULL;
            return -EIO;
        }
    }
    else
    {
        INFO("FCS already initialised");
    }
    return 0;
}
int fcs_deinit(void)
{
    int ret;
    if (fcs_descriptor == NULL)
    {
        ERROR("FCS not initialised");
        return -EIO;
    }
    else
    {
        if (fcs_descriptor->session_count != 0)
        {
            WARN("%d pending sessions to be closed",
                    fcs_descriptor->session_count);
        }
        ret = mbox_close_client( fcs_descriptor->security_handle );
        if (ret != 0)
        {
            ERROR("Failed to close malibox client");
            return -EIO;
        }
        if (ret != 0)
        {
            WARN("Failed to free mailbox resources");
        }
        ret = mbox_deinit();

        (void) memset(fcs_descriptor, 0, sizeof(struct fcs_service_descriptor));
        vPortFree(fcs_descriptor);
        fcs_descriptor = NULL;
    }
    return 0;
}

static sdm_client_handle get_client_handle( char *uuid, uint32_t *session_id )
{
    int i;
    sdm_client_handle fcs_handle = NULL;

    if (fcs_descriptor->session_count == 0)
    {
        ERROR("No opened sessions");
    }
    else
    {
        for (i = 0; i < (int)FCS_MAX_INSTANCES; i++)
        {
            if (memcmp(fcs_descriptor->session_map[i].uuid,
                    uuid, FCS_UUID_SIZE) == 0)
            {
                fcs_handle = fcs_descriptor->session_map[i].pxCryptoHandle;
                *session_id = fcs_descriptor->session_map[i].session_id;
            }
        }
    }
    return fcs_handle;
}
static void generate_uuid( sdm_client_handle fcs_handle, uint32_t session_id,
        char *uuid )
{
    uint64_t rng_args[4], rng_resp[2];
    int ret;
    bool sem_ret;
    uint32_t *rand_num_buf;

    rand_num_buf =
            (uint32_t*)pvPortMalloc(FCS_UUID_SIZE + FCS_RESP_HEADER_SIZE);
    if (rand_num_buf == NULL)
    {
        ERROR("Failed to allocate memory");
        uuid = NULL;
        return;
    }
    rng_args[0] = session_id;
    rng_args[1] = 1;
    rng_args[2] = (uint64_t)rand_num_buf;
    rng_args[3] = FCS_UUID_SIZE;
    cache_force_invalidate(rand_num_buf, FCS_UUID_SIZE + FCS_RESP_HEADER_SIZE);

    ret = sip_svc_send(fcs_handle, FCS_RANDOM_NUMBER, rng_args,
            sizeof(rng_args), rng_resp, sizeof(rng_resp));
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) && (rng_resp[FCS_RESP_STATUS] == 0UL))
        {
            cache_force_invalidate(rand_num_buf,
                    FCS_UUID_SIZE + FCS_RESP_HEADER_SIZE);
            (void) memcpy((void *) uuid, (void *) &rand_num_buf[FCS_RESP_DATA],
                    FCS_UUID_SIZE);
        }
    }
    vPortFree(rand_num_buf);
}
int run_fcs_open_service_session( char *uuid )
{
    int ret, i;
    uint16_t status;
    bool sem_ret;
    uint64_t open_session_resp[2];
    uint32_t session_id;
    sdm_client_handle fcs_handle;

    if (fcs_descriptor->session_count >= (int)FCS_MAX_INSTANCES)
    {
        ERROR("Session Limit Reaced");
        return -EIO;
    }
    if (uuid == NULL)
    {
        ERROR("Invalid parameters");
        return -EINVAL;
    }
    for (i = 0; i < (int)FCS_MAX_INSTANCES; i++)
    {
        if (fcs_descriptor->session_map[i].session_id == 0U)
        {
            break;
        }
    }
    fcs_descriptor->session_map[i].pxCryptoHandle = mbox_open_client();
    if (fcs_descriptor->session_map[i].pxCryptoHandle == NULL)
    {
        ERROR("Failed to open Mailbox Client");
        return -EIO;
    }
    fcs_handle =
            fcs_descriptor->session_map[fcs_descriptor->session_count].
            pxCryptoHandle;
    ret = mbox_set_callback(fcs_handle, fcs_callback);
    if (ret != 0)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }

    ret = sip_svc_send( fcs_handle, FCS_OPEN_SESSION, NULL, 0,
            open_session_resp, sizeof(open_session_resp));
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (open_session_resp[FCS_RESP_STATUS] == 0UL))
        {
            session_id = (uint32_t) open_session_resp[FCS_RESP_SIZE];
            fcs_descriptor->session_map[i].session_id = session_id;
            generate_uuid(fcs_handle, session_id, uuid);
            if (uuid == NULL)
            {
                ERROR("Failed to generate UUID");
                return -EIO;
            }
            uuid[6] = (char) (((uint8_t) uuid[6] & 0x0FU) | 0x40U);
            uuid[8] = (char) (((uint8_t) uuid[8] & 0x3FU) | 0x80U);
            (void) memcpy(fcs_descriptor->session_map[i].uuid, uuid,
                    FCS_UUID_SIZE);
            fcs_descriptor->session_count++;
        }
        status =
                (uint16_t)(open_session_resp[FCS_RESP_STATUS] &
                FCS_STATUS_MASK);
        return (int) status;
    }
    return ret;
}
int run_fcs_close_service_session( char *uuid )
{
    int ret, i;
    uint16_t status;
    uint64_t close_session_arg, resp_err;
    uint32_t session_id = 0U;
    sdm_client_handle fcs_handle;

    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    close_session_arg = session_id;
    ret = sip_svc_send(fcs_handle, FCS_CLOSE_SESSION,
            &close_session_arg, sizeof(close_session_arg),
            &resp_err, sizeof(resp_err));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            if (resp_err == 0UL)
            {
                ret = mbox_close_client(fcs_handle);
                if (ret == 0)
                {
                    INFO("Client closed");
                }
                else
                {
                    ERROR("Failed to close client");
                    return ret;
                }
                for (i = 0; i < (int)FCS_MAX_INSTANCES; i++)
                {
                    if (fcs_descriptor->session_map[i].session_id == session_id)
                    {
                        (void) memset(&fcs_descriptor->session_map[i], 0x0,
                                sizeof(fcs_descriptor->session_map[i]));
                        fcs_descriptor->session_count--;
                    }
                }
            }
            status = (uint16_t) resp_err;
            return (int) status;
        }
    }
    return ret;
}

int run_fcs_random_number_ext( char *rand_buf, char *uuid,
        uint32_t context_id, uint32_t rand_size )
{
    sdm_client_handle fcs_handle;
    uint64_t rng_args[4], rng_resp[2];
    int ret;
    uint16_t status;
    bool sem_ret;
    uint32_t session_id = 0U, extra_data = 0U, *rand_num_buf;

    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Invalid session id");
        return -ENOMEM;
    }
    if ((rand_size % 4U) != 0U)
    {
        extra_data = 4U - (rand_size % 4U);
    }
    if ((rand_size == 0U) || ((rand_size + extra_data) > FCS_RNG_MAX_SIZE))
    {
        ERROR("Invalid random number size");
        return -EINVAL;
    }
    rand_num_buf = (uint32_t*)pvPortMalloc(
            rand_size + extra_data + FCS_RESP_HEADER_SIZE);
    if (rand_num_buf == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    rng_args[0] = session_id;
    rng_args[1] = context_id;
    rng_args[2] = (uint64_t)rand_num_buf;
    rng_args[3] = (uint64_t) rand_size + (uint64_t) extra_data;
    cache_force_invalidate(rand_num_buf, rand_size + FCS_RESP_HEADER_SIZE);

    ret = sip_svc_send(fcs_handle, FCS_RANDOM_NUMBER, rng_args,
            sizeof(rng_args), rng_resp, sizeof(rng_resp));
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) && (rng_resp[FCS_RESP_STATUS] == 0UL))
        {
            cache_force_invalidate(rand_num_buf,
                    rand_size + FCS_RESP_HEADER_SIZE);
            status = (uint16_t) rng_resp[FCS_RESP_STATUS];
            ret = (int) status;
            (void) memcpy((void *) rand_buf,
                    (void *) &rand_num_buf[FCS_RESP_DATA], rand_size);
        }
    }
    vPortFree(rand_num_buf);
    return ret;
}

int run_fcs_import_service_key( char *uuid, char *key,
        uint32_t key_size, char *status, unsigned int *status_size )
{
    sdm_client_handle fcs_handle;
    int ret;
    uint16_t resp_stat;
    uint64_t import_key_args[2], import_key_resp[2];
    uint32_t session_id = 0U, *import_key_payload;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        return -EIO;
    }
    if ((key == NULL) || (key_size == 0))
    {
        return -EINVAL;
    }
    if (key_size > FCS_MAX_KEY_SIZE)
    {
        return -EINVAL;
    }
    import_key_payload = (uint32_t*)pvPortMalloc(
            ((size_t) key_size + FCS_KEY_HEADER_SIZE));
    if (import_key_payload == NULL)
    {
        return -ENOMEM;
    }
    (void) memset(import_key_payload, 0,
            ((size_t) key_size + FCS_KEY_HEADER_SIZE));
    import_key_payload[0] = session_id;
    /* 8 bytes reserved */
    (void) memcpy((void *) &import_key_payload[3], (void *) key, key_size);

    import_key_args[0] = (uint64_t)import_key_payload;
    import_key_args[1] = (uint64_t)key_size + FCS_KEY_HEADER_SIZE;
    cache_force_write_back((void *) import_key_payload,
            ((size_t)key_size + FCS_KEY_HEADER_SIZE));

    ret = sip_svc_send(fcs_handle, FCS_IMPORT_SERVICE_KEY,
            import_key_args, sizeof(import_key_args),
            import_key_resp, sizeof(import_key_resp));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            *status_size = MBOX_WORD_SIZE;
            (void) memcpy((void *) status,
                    (void *) &import_key_resp[FCS_RESP_KEY_STATUS],
                    *status_size);
            resp_stat = (uint16_t) import_key_resp[FCS_RESP_STATUS];
            ret = (int) resp_stat;
        }
    }
    vPortFree(import_key_payload);
    return ret;
}

int run_fcs_export_service_key( char *uuid, uint32_t key_id,
        char *key_dest, unsigned int *key_size )
{
    sdm_client_handle fcs_handle;
    uint64_t export_key_args[4], export_key_resp[2];
    uint32_t session_id = 0U, *key_data;
    int ret;
    uint16_t status;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((key_dest == NULL) || (key_size == NULL))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    key_data = pvPortMalloc(FCS_MAX_KEY_SIZE);
    if (key_data == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    export_key_args[0] = session_id;
    export_key_args[1] = key_id;
    export_key_args[2] = (uint64_t)key_data;
    export_key_args[3] = *key_size;
    cache_force_invalidate(key_data, FCS_EXPORT_KEY_MAX_SIZE);
    ret = sip_svc_send(fcs_handle, FCS_EXPORT_SERVICE_KEY,
            export_key_args, sizeof(export_key_args),
            export_key_resp, sizeof(export_key_resp));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            if (export_key_resp[FCS_RESP_STATUS] == 0UL)
            {
                cache_force_invalidate((void *) key_data,
                        (size_t) export_key_resp[FCS_RESP_SIZE]);
                /* ignoring the key status word of size 4*/
                *key_size = (uint32_t) export_key_resp[FCS_RESP_SIZE] -
                        MBOX_WORD_SIZE;
                (void) memcpy((void *) key_dest, (void *) &key_data[1],
                        *key_size);
            }
            status = (uint16_t) export_key_resp[FCS_RESP_STATUS];
            ret = (int) status;
        }
    }
    return ret;
}
int run_fcs_remove_service_key( char *uuid, uint32_t key_id )
{
    sdm_client_handle fcs_handle;
    uint64_t remove_key_args[2], remove_key_resp[2];
    uint32_t session_id = 0U;
    int ret;
    uint16_t status;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    remove_key_args[0] = session_id;
    remove_key_args[1] = key_id;

    ret = sip_svc_send(fcs_handle, FCS_REMOVE_SERVICE_KEY,
            remove_key_args, sizeof(remove_key_args),
            remove_key_resp, sizeof(remove_key_resp));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            status = (uint16_t) remove_key_resp[FCS_RESP_STATUS];
            ret = (int) status;
        }
    }

    return ret;
}
int run_fcs_get_service_key_info( char *uuid, uint32_t key_id,
        char *key_info, unsigned int *key_info_size )
{
    sdm_client_handle fcs_handle;
    uint64_t get_key_info_args[4], get_key_info_resp[2];
    uint32_t session_id = 0U;
    int ret;
    uint16_t status;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((key_info == NULL) || (key_info_size == NULL))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }

    get_key_info_args[0] = session_id;
    get_key_info_args[1] = key_id;
    get_key_info_args[2] = (uint64_t)key_info;
    get_key_info_args[3] = *key_info_size;
    cache_force_write_back(key_info, FCS_KEY_INFO_MAX_RESP);

    ret = sip_svc_send(fcs_handle, FCS_GET_SERVICE_KEY_INFO,
            get_key_info_args, sizeof(get_key_info_args),
            get_key_info_resp, sizeof(get_key_info_resp));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            if (get_key_info_resp[FCS_RESP_STATUS] == 0UL)
            {
                *key_info_size = (uint32_t) get_key_info_resp[FCS_RESP_SIZE];
                cache_force_invalidate(key_info, *key_info_size);
            }
            status =
                    (uint16_t)(get_key_info_resp[FCS_RESP_STATUS] &
                    FCS_STATUS_MASK);
            return (int)status;
        }
    }
    return ret;
}
int run_fcs_create_service_key( char *uuid, char *key,
        uint32_t key_size, char *status, unsigned int *status_size )
{
    sdm_client_handle fcs_handle;
    uint64_t create_key_args[2], create_key_resp[2];
    int ret;
    uint16_t resp_stat;
    uint32_t session_id = 0U, *create_key_payload;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((key == NULL) || (key_size == 0))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    if (key_size > FCS_MAX_KEY_SIZE)
    {
        ERROR("Key size exceeds maximum limit");
        return -EINVAL;
    }
    create_key_payload = (uint32_t*)pvPortMalloc(
            ((size_t) key_size + FCS_KEY_HEADER_SIZE));
    if (create_key_payload == NULL)
    {
        return -ENOMEM;
    }
    (void) memset(create_key_payload, 0,
            ((size_t) key_size + FCS_KEY_HEADER_SIZE));
    create_key_payload[0] = session_id;
    /* 8 bytes reserved */
    (void) memcpy((void *) &create_key_payload[3], (void *) key, key_size);

    create_key_args[0] = (uint64_t)create_key_payload;
    create_key_args[1] = ((uint64_t) key_size + FCS_KEY_HEADER_SIZE);
    cache_force_write_back((void *) create_key_payload,
            ((size_t)key_size + FCS_KEY_HEADER_SIZE));

    ret = sip_svc_send(fcs_handle, FCS_CREATE_SERVICE_KEY,
            create_key_args, sizeof(create_key_args),
            create_key_resp, sizeof(create_key_resp));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            *status_size = MBOX_WORD_SIZE;
            (void) memcpy((void *) status,
                    (void *) &create_key_resp[FCS_RESP_KEY_STATUS],
                    *status_size);
            resp_stat =
                    (uint16_t)(create_key_resp[FCS_RESP_STATUS] &
                    FCS_STATUS_MASK);
            return (int)resp_stat;
        }
    }
    vPortFree(create_key_payload);
    return ret;
}

int run_fcs_service_get_provision_data(char *prov_data,
        uint32_t *prov_data_size)
{
    int ret;
    uint16_t status;
    uint64_t prov_data_arg, prov_data_resp[2];
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    if ((prov_data == NULL) || (prov_data_size == NULL))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    cache_force_invalidate(prov_data, FCS_PROV_DATA_MAX_SIZE);

    prov_data_arg = (uint64_t)prov_data;
    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_GET_PROVISION_DATA,
            &prov_data_arg, sizeof(prov_data_arg), prov_data_resp,
            sizeof(prov_data_resp));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            if (prov_data_resp[FCS_RESP_STATUS] == 0UL)
            {
                *prov_data_size = (uint32_t) prov_data_resp[FCS_RESP_SIZE];
            }
            status = (uint16_t) prov_data_resp[FCS_RESP_STATUS];
            ret = (int) status;
        }
    }
    return ret;
}
int run_fcs_send_certificate( char *cert_data, uint32_t cert_size,
        uint32_t *status )
{
    uint64_t send_cert_args[3], send_cert_resp[2];
    int ret;
    uint16_t resp_stat;
    /* 4 bytes reserved data required before the actual certificate data */
    uint32_t *send_cert_data = (uint32_t*)pvPortMalloc(cert_size + 4U);
    if (send_cert_data == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }

    memset(send_cert_data, 0, cert_size + sizeof(uint32_t));
    memcpy(&send_cert_data[1], cert_data, cert_size);
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }

    send_cert_args[0] = (uint64_t)send_cert_data;
    send_cert_args[1] = (uint64_t)cert_size + sizeof(uint32_t);
    cache_force_write_back( send_cert_data, cert_size + sizeof(uint32_t));

    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_SEND_CERTIFICATE,
            send_cert_args, sizeof(send_cert_args), send_cert_resp,
            sizeof(send_cert_resp));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            if (send_cert_resp[FCS_RESP_STATUS] == 0UL)
            {
                (void) memcpy((void *) status,
                        (void *) &send_cert_resp[FCS_RESP_CERT_STATUS],
                        sizeof(uint32_t));
            }
            resp_stat =
                    (uint16_t)(send_cert_resp[FCS_RESP_STATUS] &
                    FCS_STATUS_MASK);
            return (int)resp_stat;
        }
    }
    return ret;
}
int run_fcs_service_counter_set_preauthorized( uint8_t type, uint32_t value,
        uint32_t test)
{
    uint64_t cntr_set_preauth_args[3], cntr_set_preauth_err;
    int ret;
    uint16_t status;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }

    cntr_set_preauth_args[0] = type;
    cntr_set_preauth_args[1] = value;
    cntr_set_preauth_args[2] = test;

    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_CNTR_SET_PREAUTH,
            cntr_set_preauth_args, sizeof(cntr_set_preauth_args),
            &cntr_set_preauth_err, sizeof(cntr_set_preauth_err));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            status = (uint16_t)(cntr_set_preauth_err & FCS_STATUS_MASK);
            return (int)status;
        }
    }
    return ret;
}

static int run_fcs_get_digest_init( char *uuid,
        uint32_t context_id, uint32_t key_id,
        uint32_t op_mode, uint32_t dig_size )
{
    uint64_t fcs_digest_init_args[5];
    uint32_t session_id = 0U, param_data;
    sdm_client_handle fcs_handle;

    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((op_mode != FCS_DIGEST_OPMODE_SHA2) &&
            (op_mode != FCS_DIGEST_OPMODE_HMAC))
    {
        ERROR("Invalid Operation Mode for DIGEST_REQ");
        return -EINVAL;
    }
    if ((dig_size != FCS_DIGEST_SIZE_256) &&
            (dig_size != FCS_DIGEST_SIZE_384) &&
            (dig_size != FCS_DIGEST_SIZE_512))
    {
        ERROR("Invalid Digest Size for DIGEST_REQ");
        return -EINVAL;
    }
    if (op_mode == FCS_DIGEST_OPMODE_SHA2)
    {
        key_id = 0;
    }
    param_data = op_mode;
    param_data |= FCS_SET_DIGEST_SIZE( dig_size );

    fcs_digest_init_args[0] = session_id;
    fcs_digest_init_args[1] = context_id;
    fcs_digest_init_args[2] = key_id;
    fcs_digest_init_args[3] = FCS_DIGEST_PARAM_SIZE;
    fcs_digest_init_args[4] = param_data;

    return sip_svc_send(fcs_handle, FCS_GET_DIGEST_INIT,
            fcs_digest_init_args, sizeof(fcs_digest_init_args), NULL, 0);
}

static int run_fcs_get_digest_update(
        char *uuid, uint32_t context_id,
        char *src_data, uint32_t src_size,
        char *digest_data, uint32_t *digest_size,
        uint8_t final )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    uint64_t fcs_digest_update_args[7], fcs_digest_update_smc_resp[2];
    uint32_t session_id = 0U, *fcs_digest_mbox_resp;
    sdm_client_handle fcs_handle;

    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    fcs_digest_mbox_resp = (uint32_t*)pvPortMalloc(
            FCS_DIGEST_MAX_RESP );
    if (fcs_digest_mbox_resp == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    cache_force_invalidate(fcs_digest_mbox_resp, FCS_DIGEST_MAX_RESP);
    cache_force_write_back(src_data, src_size);

    fcs_digest_update_args[0] = session_id;
    fcs_digest_update_args[1] = context_id;
    /*
     * We shall provide the smmu address instead of the
     * regular source address for the SMC call
     */
    fcs_digest_update_args[2] = (uint64_t)src_data;
    fcs_digest_update_args[3] = src_size;
    fcs_digest_update_args[4] = (uint64_t)fcs_digest_mbox_resp;
    fcs_digest_update_args[5] = FCS_DIGEST_MAX_RESP;
    fcs_digest_update_args[6] = (uint64_t)FCS_SMMU_GET_ADDR(src_data);
    cache_force_write_back(src_data, src_size);

    if (final == FCS_FINALIZE)
    {
        ret = sip_svc_send(fcs_handle, FCS_GET_DIGEST_FINALIZE,
                fcs_digest_update_args, sizeof(fcs_digest_update_args),
                fcs_digest_update_smc_resp, sizeof(fcs_digest_update_smc_resp));
    }
    else
    {
        ret = sip_svc_send(fcs_handle, FCS_GET_DIGEST_UPDATE,
                fcs_digest_update_args, sizeof(fcs_digest_update_args),
                fcs_digest_update_smc_resp, sizeof(fcs_digest_update_smc_resp));
    }
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) && (final == FCS_FINALIZE) &&
                (fcs_digest_update_smc_resp[FCS_RESP_STATUS] == 0UL))
        {
            cache_force_invalidate((void *) fcs_digest_mbox_resp,
                    (size_t) fcs_digest_update_smc_resp[FCS_RESP_SIZE]);
            *digest_size =
                    (uint32_t) fcs_digest_update_smc_resp[FCS_RESP_SIZE] -
                    FCS_RESP_HEADER_SIZE;
            (void) memcpy((void *) digest_data,
                    (void *) &fcs_digest_mbox_resp[FCS_RESP_DATA],
                    *digest_size);
        }
        status = (uint16_t) fcs_digest_update_smc_resp[FCS_RESP_STATUS];
        ret = (int) status;
    }

    vPortFree(fcs_digest_mbox_resp);
    return ret;
}
int run_fcs_get_digest( char *uuid, uint32_t context_id,
        uint32_t key_id, uint32_t op_mode, uint32_t dig_size,
        char *src_data, uint32_t src_size, char *digest_data,
        uint32_t *digest_size )
{
    int ret;
    uint32_t session_id = 0U, max_size, remaining_data = src_size, data_written;
    sdm_client_handle fcs_handle;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if (FCS_GET_COMM_SOURCE(session_id) == FCS_COMM_SOURCE_HPS)
    {
        max_size = FCS_MAX_SIZE_HPS;
    }
    else
    {
        max_size = FCS_MAX_SIZE_FPGA;
    }
    /* check if 8 byte aligned */
    if ((((uint64_t)(uintptr_t)src_data % 8UL) != 0UL) ||
            (((uint64_t)(uintptr_t)digest_data % 8UL) != 0UL) ||
            (digest_size == NULL))
    {
        ERROR("Invalid address");
        return -EINVAL;
    }
    if ((src_size < 8U) || ((src_size % 8U) != 0U))
    {
        ERROR("Invalid Size");
        return -EINVAL;
    }

    ret = run_fcs_get_digest_init( uuid, context_id, key_id,
            op_mode, dig_size );
    if (ret != 0)
    {
        ERROR("Failed to initialise GET_DIGEST");
        return ret;
    }
    data_written = 0U;
    while (remaining_data > 0U)
    {
        if (remaining_data > max_size)
        {
            data_written = max_size;
            ret = run_fcs_get_digest_update( uuid, context_id,
                    src_data, max_size, digest_data, digest_size,
                    FCS_UPDATE );
        }
        else
        {
            data_written = remaining_data;
            ret = run_fcs_get_digest_update( uuid, context_id,
                    src_data, remaining_data, digest_data, digest_size,
                    FCS_FINALIZE );
        }
        if (ret == 0)
        {
            remaining_data -= data_written;
            src_data += data_written;
        }
        else
        {
            ERROR("GET_DIGEST failed");
            return ret;
        }
    }
    return ret;
}
static int run_fcs_mac_verify_init(
        char *uuid, uint32_t context_id,
        uint32_t key_id, uint32_t dig_mode)
{
    uint64_t fcs_mac_verify_init_args[5];
    uint32_t session_id = 0U;
    sdm_client_handle fcs_handle;

    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    fcs_mac_verify_init_args[0] = session_id;
    fcs_mac_verify_init_args[1] = context_id;
    fcs_mac_verify_init_args[2] = key_id;
    fcs_mac_verify_init_args[3] = FCS_MAC_PARAM_SIZE;
    fcs_mac_verify_init_args[4] = FCS_SET_DIGEST_SIZE((uint64_t) dig_mode);

    return sip_svc_send(fcs_handle, FCS_MAC_VERIFY_INIT,
            fcs_mac_verify_init_args, sizeof(fcs_mac_verify_init_args), NULL,
            0);
}
static int run_fcs_mac_verify_update(
        char *uuid, uint32_t context_id,
        char *src_addr, uint32_t src_size,
        char *mac_data, uint32_t mac_data_size,
        char *dest_data, uint32_t *dest_size,
        uint8_t final)
{
    (void)mac_data;
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_mac_verify_args[8], mac_verify_smc_resp[2];
    uint32_t session_id = 0U, *mac_verify_mbox_resp;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    mac_verify_mbox_resp = (uint32_t*)pvPortMalloc(
            FCS_MAC_VERIFY_RESP);
    if (mac_verify_mbox_resp == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    *dest_size = 0;
    fcs_mac_verify_args[0] = session_id;
    fcs_mac_verify_args[1] = context_id;
    fcs_mac_verify_args[2] = (uint64_t)src_addr;
    fcs_mac_verify_args[3] = (uint64_t) src_size + (uint64_t) mac_data_size;
    fcs_mac_verify_args[4] = (uint64_t)mac_verify_mbox_resp;
    fcs_mac_verify_args[5] = FCS_MAC_VERIFY_RESP;
    fcs_mac_verify_args[6] = src_size;
    fcs_mac_verify_args[7] = (uint64_t)FCS_SMMU_GET_ADDR(src_addr);

    cache_force_write_back(src_addr, src_size + mac_data_size);
    cache_force_invalidate(mac_verify_mbox_resp, FCS_MAC_VERIFY_RESP);

    if (final == FCS_UPDATE)
    {
        ret = sip_svc_send(fcs_handle, FCS_MAC_VERIFY_UPDATE,
                fcs_mac_verify_args, sizeof(fcs_mac_verify_args),
                mac_verify_smc_resp, sizeof(mac_verify_smc_resp));
    }
    else
    {
        ret = sip_svc_send(fcs_handle, FCS_MAC_VERIFY_FINALIZE,
                fcs_mac_verify_args, sizeof(fcs_mac_verify_args),
                mac_verify_smc_resp, sizeof(mac_verify_smc_resp));
    }
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (mac_verify_smc_resp[FCS_RESP_STATUS] == 0UL))
        {
            cache_flush((void *) mac_verify_mbox_resp,
                    (size_t) mac_verify_smc_resp[FCS_RESP_SIZE]);
            *dest_size = (uint32_t) mac_verify_smc_resp[FCS_RESP_SIZE];
            (void) memcpy((void *) dest_data,
                    (void *) &mac_verify_mbox_resp[FCS_RESP_DATA], *dest_size);
        }
        status = (uint16_t) mac_verify_smc_resp[FCS_RESP_STATUS];
        ret = (int) status;
    }
    vPortFree(mac_verify_mbox_resp);
    return ret;
}
int run_fcs_mac_verify( char *uuid, uint32_t context_id,
        uint32_t key_id, uint32_t dig_size, char *src_data,
        uint32_t src_size, char *dest_data, uint32_t *dest_size,
        uint32_t user_data_size )
{
    uint32_t session_id = 0U, max_size, remaining_data = src_size, data_written,
            mac_size;
    int ret;
    char *mac_data;
    sdm_client_handle fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    if (FCS_GET_COMM_SOURCE(session_id) == FCS_COMM_SOURCE_HPS)
    {
        max_size = FCS_MAX_SIZE_HPS;
    }
    else
    {
        max_size = FCS_MAX_SIZE_FPGA;
    }
    if (key_id == 0U)
    {
        ERROR("Invalid Key ID");
        return -EINVAL;
    }
    if ((((uint64_t)(uintptr_t)src_data % 8UL) != 0UL) ||
            (dest_data == NULL) ||
            (dest_size == NULL))
    {
        ERROR("Invalid address");
        return -EINVAL;
    }
    if ((src_size < 8U) || ((src_size % 8U) != 0U))
    {
        ERROR("Invalid Size");
        return -EINVAL;
    }

    ret = run_fcs_mac_verify_init( uuid, context_id, key_id,
            dig_size );
    if (ret != 0)
    {
        ERROR("Failed to initialise mac_verify");
        return ret;
    }
    mac_data = src_data + user_data_size;
    mac_size = src_size - user_data_size;
    while (remaining_data > 0U)
    {
        if (remaining_data > max_size)
        {
            /* minimum 8 bytes of data for final command*/
            if ((remaining_data - max_size) >= (8 + mac_size))
            {
                data_written = max_size;
                ret = run_fcs_mac_verify_update( uuid, context_id,
                        src_data, data_written, mac_data, 0,
                        dest_data, dest_size, FCS_UPDATE );

            }
            else
            {
                data_written = remaining_data - (8 + mac_size);
                ret = run_fcs_mac_verify_update( uuid, context_id,
                        src_data, data_written, mac_data, 0,
                        dest_data, dest_size, FCS_UPDATE );
            }
        }
        else
        {
            data_written = remaining_data;
            ret = run_fcs_mac_verify_update( uuid, context_id,
                    src_data, remaining_data - mac_size, mac_data,
                    mac_size, dest_data, dest_size,
                    FCS_FINALIZE );
        }
        if (ret == 0)
        {
            remaining_data -= data_written;
            src_data += data_written;
        }
        else
        {
            ERROR("mac_verify failed");
            return ret;
        }
    }
    return ret;
}

int run_fcs_sdos_encrypt( char *uuid, uint32_t context_id,
        char *src_data, uint32_t src_size, char *resp_data,
        uint32_t *resp_size )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t sdos_encrypt_args[10], sdos_smc_resp[2];
    uint32_t session_id;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((src_data == NULL) || (src_size == 0U))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    if ((resp_data == NULL) || (resp_size == NULL))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    sdos_encrypt_args[0] = session_id;
    sdos_encrypt_args[1] = context_id;
    sdos_encrypt_args[2] = FCS_SDOS_ENCRYPT_MODE;
    /* we pass the smmu address instead of regular address */
    sdos_encrypt_args[3] = (uint64_t)(uintptr_t)src_data;
    sdos_encrypt_args[4] = src_size;
    sdos_encrypt_args[5] = (uint64_t)(uintptr_t)resp_data;
    sdos_encrypt_args[6] = FCS_SDOS_ENC_MAX_RESP;
    /* no owner id required */
    sdos_encrypt_args[7] = 0;
    sdos_encrypt_args[8] = FCS_SMMU_GET_ADDR(src_data);
    sdos_encrypt_args[9] = FCS_SMMU_GET_ADDR(resp_data);
    cache_force_write_back(src_data, src_size);
    cache_force_invalidate(resp_data, FCS_SDOS_ENC_MAX_RESP);

    ret = sip_svc_send(fcs_handle, FCS_SDOS_CRYPTION,
            sdos_encrypt_args, sizeof(sdos_encrypt_args),
            sdos_smc_resp, sizeof(sdos_smc_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) && (sdos_smc_resp[FCS_RESP_STATUS] == 0UL))
        {
            *resp_size = (uint32_t) sdos_smc_resp[FCS_RESP_SIZE];
            cache_force_invalidate(resp_data, *resp_size);
        }
        status = (uint16_t) sdos_smc_resp[FCS_RESP_STATUS];
        ret = (int) status;
    }
    return ret;
}

int run_fcs_sdos_decrypt( char *uuid, uint32_t context_id,
        char *src_data, uint32_t src_size, char *resp_data,
        uint32_t *resp_size, uint64_t owner_flag)
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t sdos_decrypt_args[10], sdos_smc_resp[2];
    uint32_t session_id;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((src_data == NULL) || (src_size == 0U))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    if ((resp_data == NULL) || (resp_size == NULL))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }

    sdos_decrypt_args[0] = session_id;
    sdos_decrypt_args[1] = context_id;
    sdos_decrypt_args[2] = FCS_SDOS_DECRYPT_MODE;
    /* we pass the smmu address instead of regular address */
    sdos_decrypt_args[3] = (uint64_t)(uintptr_t)src_data;
    sdos_decrypt_args[4] = src_size;
    sdos_decrypt_args[5] = (uint64_t)(uintptr_t)resp_data;
    sdos_decrypt_args[6] = FCS_SDOS_DEC_MAX_RESP;
    sdos_decrypt_args[7] = owner_flag;
    sdos_decrypt_args[8] = FCS_SMMU_GET_ADDR(src_data);
    sdos_decrypt_args[9] = FCS_SMMU_GET_ADDR(resp_data);
    cache_force_write_back(src_data, src_size);
    cache_force_invalidate(resp_data, FCS_SDOS_DEC_MAX_RESP);

    ret = sip_svc_send(fcs_handle, FCS_SDOS_CRYPTION,
            sdos_decrypt_args, sizeof(sdos_decrypt_args),
            sdos_smc_resp, sizeof(sdos_smc_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) && (sdos_smc_resp[FCS_RESP_STATUS] == 0UL))
        {
            *resp_size = (uint32_t) sdos_smc_resp[FCS_RESP_SIZE];
            cache_force_invalidate(resp_data, *resp_size);
        }
        status = (uint16_t) sdos_smc_resp[FCS_RESP_STATUS];
        ret = (int)status;
    }
    return ret;
}

/* HKDF Input buffer data format
 * 1st input data size in bytes
 * 1st input data padded to 80 bytes
 * 2nd input data size in bytes
 * 2nd input data padded to 80 bytes
 * Output key object
 */
int run_fcs_hkdf_request( char *uuid, uint32_t key_id,
        uint32_t step_type, uint32_t mac_mode, char *input_buffer,
        uint32_t output_key_size, uint32_t *hkdf_status )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t hkdf_args[6], hkdf_resp[2];
    uint32_t session_id = 0U;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if (input_buffer == NULL)
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    if (output_key_size == 0U)
    {
        ERROR("Invalid output key size");
        return -EINVAL;
    }

    hkdf_args[0] = session_id;
    hkdf_args[1] = step_type;
    hkdf_args[2] = mac_mode;
    hkdf_args[3] = (uint64_t)input_buffer;
    hkdf_args[4] = key_id;
    hkdf_args[5] = output_key_size;
    cache_force_write_back(input_buffer, 400);
    ret = sip_svc_send(fcs_handle, FCS_HKDF_REQUEST,
            hkdf_args, sizeof(hkdf_args),
            hkdf_resp, sizeof(hkdf_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) && (hkdf_resp[FCS_RESP_STATUS] == 0UL))
        {
            status = (uint16_t)(hkdf_resp[FCS_RESP_STATUS] & FCS_STATUS_MASK);
            *hkdf_status =
                    (uint32_t)(hkdf_resp[FCS_RESP_KEY_STATUS] &
                    FCS_STATUS_MASK);
            return (int) status;
        }
    }
    return ret;
}

int run_fcs_get_chip_id( uint32_t *chip_low,
        uint32_t *chip_high)
{
    uint64_t get_chip_id_resp[FCS_RESP_DATA];
    int ret;
    uint16_t status;
    bool sem_ret;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }

    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_GET_CHIP_ID,
            NULL, 0, get_chip_id_resp, sizeof(get_chip_id_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (get_chip_id_resp[FCS_RESP_STATUS] == 0UL))
        {
            *chip_low = (uint32_t) get_chip_id_resp[1];
            *chip_high = (uint32_t) get_chip_id_resp[2];
        }
        status =
                (uint16_t)(get_chip_id_resp[FCS_RESP_STATUS] & FCS_STATUS_MASK);
        return (int) status;
    }
    return ret;
}

int run_fcs_attestation_get_certificate( int cert_req, char *cert_data,
        uint32_t *cert_size )
{
    uint64_t get_attestation_cert_args[3], get_attestation_cert_resp[2];
    int ret;
    uint16_t status;
    bool sem_ret;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    if ((cert_data == NULL) || (cert_size == NULL))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    cache_force_invalidate(cert_data, FCS_ATTEST_CERT_MAX_SIZE);

    get_attestation_cert_args[0] = (uint64_t)cert_req;
    get_attestation_cert_args[1] = (uint64_t)cert_data;
    get_attestation_cert_args[2] = *cert_size;
    ret = sip_svc_send(fcs_descriptor->security_handle,
            FCS_GET_ATTESTATION_CERT,
            get_attestation_cert_args, sizeof(get_attestation_cert_args),
            get_attestation_cert_resp, sizeof(get_attestation_cert_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (get_attestation_cert_resp[FCS_RESP_STATUS] == 0UL))
        {
            *cert_size = (uint32_t) get_attestation_cert_resp[FCS_RESP_SIZE];
            cache_force_invalidate(cert_data, *cert_size);
        }
        status =
                (uint16_t)(get_attestation_cert_resp[FCS_RESP_STATUS] &
                FCS_STATUS_MASK);
        return (int) status;
    }
    return ret;
}

int run_fcs_attestation_certificate_reload( int cert_req )
{
    uint64_t cert_on_reload_args[3], cert_reload_err;
    int ret;
    uint16_t status;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }

    cert_on_reload_args[0] = (uint64_t)cert_req;
    ret = sip_svc_send(fcs_descriptor->security_handle,
            FCS_CREATE_CERT_ON_RELOAD,
            cert_on_reload_args, sizeof(cert_on_reload_args),
            &cert_reload_err, sizeof(cert_reload_err));
    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            status = (uint16_t) cert_reload_err;
            return (int)status;
        }
    }

    return ret;
}

int run_fcs_mctp_cmd_send( char *src_data, uint32_t src_size, char *resp_data,
        uint32_t *resp_size )
{
    uint64_t mctp_send_args[3], mctp_send_resp[2];
    int ret;
    uint16_t status;
    bool sem_ret;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    if ((src_data == NULL) || (src_size == 0U) ||
            (resp_data == NULL) || (resp_size == NULL))
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    cache_force_write_back(src_data, src_size);
    cache_force_invalidate(resp_data, FCS_MCTP_MAX_SIZE);

    mctp_send_args[0] = (uint64_t)src_data;
    mctp_send_args[1] = src_size;
    mctp_send_args[2] = (uint64_t)resp_data;
    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_MCTP_SEND_MSG,
            mctp_send_args, sizeof(mctp_send_args),
            mctp_send_resp, sizeof(mctp_send_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) && (mctp_send_resp[FCS_RESP_STATUS] == 0UL))
        {
            *resp_size = (uint32_t) mctp_send_resp[FCS_RESP_SIZE];
            cache_force_invalidate(resp_data, *resp_size);
        }
        status = (uint16_t)(mctp_send_resp[FCS_RESP_STATUS] & FCS_STATUS_MASK);
        return (int) status;
    }
    return ret;
}

int run_fcs_get_jtag_idcode( uint32_t *jtag_id_code)
{
    uint64_t get_idcode_resp[2];
    int ret;
    uint16_t status;
    bool sem_ret;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }

    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_GET_IDCODE,
            NULL, 0, get_idcode_resp, sizeof(get_idcode_resp));
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (get_idcode_resp[FCS_RESP_STATUS] == 0UL))
        {
            *jtag_id_code = (uint32_t) get_idcode_resp[1];
        }
        status = (uint16_t)(get_idcode_resp[FCS_RESP_STATUS] & FCS_STATUS_MASK);
        return (int) status;
    }

    return ret;
}

int run_fcs_get_device_identity( char *dev_identity, uint32_t *dev_id_size )
{
    uint64_t get_device_identity_args[1], get_device_identity_resp[2];
    int ret;
    uint16_t status;
    bool sem_ret;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    cache_force_invalidate(dev_identity, FCS_DEV_IDENTITY_RESP_SIZE);

    get_device_identity_args[0] = (uint64_t)dev_identity;
    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_GET_DEVICE_IDENTITY,
            get_device_identity_args, sizeof(get_device_identity_args),
            get_device_identity_resp, sizeof(get_device_identity_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (get_device_identity_resp[FCS_RESP_STATUS] == 0UL))
        {
            *dev_id_size = (uint32_t) get_device_identity_resp[FCS_RESP_SIZE];
            cache_force_invalidate(dev_identity, *dev_id_size);
        }
        status =
                (uint16_t)(get_device_identity_resp[FCS_RESP_STATUS] &
                FCS_STATUS_MASK);
        return (int) status;
    }
    return ret;
}

static int fcs_aes_set_params( uint32_t block_mode, uint32_t crypt_mode,
        uint32_t iv_src, char *iv_data, uint32_t tag_size, uint32_t aad_size,
        uint32_t *param_data, uint32_t *param_size)
{
    *param_size = FCS_AES_MAX_PARAM_SIZE;
    if ((block_mode != FCS_AES_ECB) && (block_mode != FCS_AES_CBC) &&
            (block_mode != FCS_AES_CTR) && (block_mode != FCS_AES_GCM) &&
            (block_mode != FCS_AES_GCM_GHASH))
    {
        ERROR("Invalid AES mode");
        return -EINVAL;
    }
    param_data[0] = block_mode;
    if (block_mode == FCS_AES_GCM_GHASH)
    {
        crypt_mode = 0;
    }
    param_data[0] |= FCS_SET_CRYPT_MODE(crypt_mode);
    if ((block_mode != FCS_AES_GCM) && (block_mode != FCS_AES_GCM_GHASH))
    {
        /* tags not applicable for non GCM and GCM-GHASH modes */
        /* iv generation not applicable for non GCM and GCM-GHASH modes */
        tag_size = 0x0;
        iv_src = 0x0;
        param_data[1] = 0;
        if (FCS_AES_BLOCK_MODE(block_mode) == FCS_AES_ECB)
        {
            *param_size = FCS_AES_ECB_PARAM_SIZE;
        }
        else
        {
            /* aad_data not applicable */
            param_data[2] = 0;
        }
    }
    else
    {
        if (tag_size > FCS_AES_TAG_128)
        {
            ERROR("Invalid tag length");
            return -EINVAL;
        }
        param_data[0] |= FCS_SET_TAG_LEN(tag_size);
        if (iv_src > FCS_IV_INTERNAL_BASE)
        {
            ERROR("Invalid IV source");
            return -EINVAL;
        }
        param_data[1] = iv_src;
        if (aad_size > FCS_AAD_MAX_SIZE)
        {
            ERROR("Invalid AAD size");
            return -EINVAL;
        }
        param_data[2] = aad_size;

    }
    (void) memcpy((void *) &param_data[3], (void *) iv_data, FCS_AES_IV_SIZE );
    return 0;
}
static int run_fcs_aes_crypt_init( char *uuid, uint32_t context_id,
        uint32_t key_id, uint32_t block_mode, uint32_t crypt_mode,
        uint32_t iv_src, char *iv_data, uint32_t tag_size,
        uint32_t aad_size )
{
    int ret;
    uint64_t fcs_aes_init_args[5];
    uint32_t session_id = 0U, param_size = 0, *param_data;
    sdm_client_handle fcs_handle;

    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    param_data = (uint32_t*)pvPortMalloc(
            FCS_AES_MAX_PARAM_SIZE);
    if (param_data == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    ret = fcs_aes_set_params( block_mode, crypt_mode, iv_src, iv_data,
            tag_size, aad_size, param_data, &param_size );
    if (ret != 0)
    {
        ERROR("Failed to set AES params");
        vPortFree(param_data);
        return -EINVAL;
    }
    cache_force_write_back(param_data, param_size);

    fcs_aes_init_args[0] = session_id;
    fcs_aes_init_args[1] = context_id;
    fcs_aes_init_args[2] = key_id;
    fcs_aes_init_args[3] = (uint64_t)param_data;
    fcs_aes_init_args[4] = param_size;

    ret = sip_svc_send(fcs_handle, FCS_AES_INIT,
            fcs_aes_init_args, sizeof(fcs_aes_init_args), NULL, 0);

    vPortFree(param_data);
    return ret;
}
static int run_fcs_aes_update( char *uuid, uint32_t context_id,
        char *src_addr,uint32_t src_size, char *dest_addr,
        uint32_t dest_size, uint32_t padding_size,
        uint32_t *data_written, uint8_t final)
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_aes_update_args[9], fcs_aes_update_resp[2];
    uint32_t session_id = 0U;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    cache_force_write_back(src_addr, src_size);
    cache_force_invalidate(dest_addr, dest_size);

    fcs_aes_update_args[0] = session_id;
    fcs_aes_update_args[1] = context_id;
    /* temp fix for address check */
    fcs_aes_update_args[2] = (uint64_t)(uintptr_t)src_addr;
    fcs_aes_update_args[3] = src_size;
    fcs_aes_update_args[4] = (uint64_t)(uintptr_t)dest_addr;
    fcs_aes_update_args[5] = dest_size;
    fcs_aes_update_args[6] = padding_size;
    /* passing smmu address */
    fcs_aes_update_args[7] = FCS_SMMU_GET_ADDR(src_addr);
    fcs_aes_update_args[8] = FCS_SMMU_GET_ADDR(dest_addr);
    if (final == FCS_UPDATE)
    {
        ret = sip_svc_send(fcs_handle, FCS_AES_UPDATE,
                fcs_aes_update_args, sizeof(fcs_aes_update_args),
                fcs_aes_update_resp, sizeof(fcs_aes_update_resp));
    }
    else
    {
        ret = sip_svc_send(fcs_handle, FCS_AES_FINALIZE,
                fcs_aes_update_args, sizeof(fcs_aes_update_args),
                fcs_aes_update_resp, sizeof(fcs_aes_update_resp));
    }
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (fcs_aes_update_resp[FCS_RESP_STATUS] == 0U))
        {
            *data_written = (uint32_t) fcs_aes_update_resp[FCS_RESP_SIZE];
            cache_force_invalidate(dest_addr, *data_written);
        }
        status =
                (uint16_t)(fcs_aes_update_resp[FCS_RESP_STATUS] &
                FCS_STATUS_MASK);
        return (int) status;
    }

    return ret;
}

int run_fcs_aes_cryption( char *uuid, uint32_t key_id,
        uint32_t context_id, uint32_t crypt_mode, uint32_t block_mode,
        uint32_t iv_src, char *iv_data, uint32_t tag_size,
        uint32_t aad_size, char *aad_data, char *input_data,
        uint32_t input_size, char *output_data, uint32_t output_size)
{
    int ret;
    int finalized = 0;
    uint32_t session_id = 0U, data_rem, max_size, data_written = 0U, padding1,
            padding2, aes_input_size, aes_output_size, input_tag = 0U;
    char *aes_input_data, *temp;
    sdm_client_handle fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    padding1 = 0;
    padding2 = 0;
    if ((block_mode == FCS_AES_GCM) || (block_mode == FCS_AES_GCM_GHASH))
    {
        /* pad to make it multiple of 8 bytes */
        if ((aad_size % FCS_GCM_BLOCK_SIZE) != 0U)
        {
            padding1 = FCS_GCM_BLOCK_SIZE - (aad_size % FCS_GCM_BLOCK_SIZE);
        }
    }
    aes_input_size = aad_size + padding1 + input_size;
    if ((aes_input_size % FCS_GCM_BLOCK_SIZE) != 0U)
    {
        padding2 = FCS_GCM_BLOCK_SIZE - (aes_input_size % FCS_GCM_BLOCK_SIZE);
    }
    aes_input_size += padding2;
    aes_input_data = (char *)pvPortMalloc(aes_input_size + FCS_GCM_TAG_SIZE);
    if (aes_input_data == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    temp = aes_input_data;
    (void) memset(aes_input_data, 0, aes_input_size + FCS_GCM_TAG_SIZE);
    (void) memcpy(aes_input_data, aad_data, aad_size);
    (void) memcpy(aes_input_data + aad_size + padding1, input_data, input_size);
    if (((crypt_mode != FCS_AES_ENCRYPT_MODE) &&
            (block_mode == FCS_AES_GCM)) ||
            (block_mode == FCS_AES_GCM_GHASH))
    {
        (void) memcpy(aes_input_data + aes_input_size, input_data + input_size,
                FCS_GCM_TAG_SIZE);
        aes_input_size += FCS_GCM_TAG_SIZE;
        input_tag = FCS_GCM_TAG_SIZE;
    }
    if (((input_size % 32U) != 0U) || (input_size < 32U))
    {
        if (((input_size % 16U) == 0U) || (input_size == 0U))
        {
            if ((block_mode != FCS_AES_GCM) &&
                    (block_mode != FCS_AES_GCM_GHASH))
            {
                ERROR("Invalid Source Size");
                vPortFree(aes_input_data);
                return -EINVAL;
            }
        }
        else
        {
            ERROR("Invalid Source Size");
            vPortFree(aes_input_data);
            return -EINVAL;
        }
    }
    if ((block_mode == FCS_AES_GCM_GHASH))
    {
        output_size = 0;
    }
    if (((output_size % 32U) != 0U) || (output_size < 32U))
    {
        if (((output_size % 16U) == 0U) || (output_size == 16U))
        {
            if ((block_mode != FCS_AES_GCM) &&
                    (block_mode != FCS_AES_GCM_GHASH))
            {
                ERROR("Invalid Destination Size");
                vPortFree(aes_input_data);
                return -EINVAL;
            }
        }
        else
        {
            ERROR("Invalid Destination Size");
            vPortFree(aes_input_data);
            return -EINVAL;
        }
    }

    ret = run_fcs_aes_crypt_init( uuid, context_id, key_id,
            block_mode, crypt_mode, iv_src, iv_data, tag_size,
            aad_size );
    if (ret != 0)
    {
        ERROR("Failed to initialise AES sequence");
        vPortFree(aes_input_data);
        return ret;
    }

    data_rem = aes_input_size;
    if ((block_mode == FCS_AES_GCM) ||
            (block_mode == FCS_AES_GCM_GHASH))
    {
        max_size = FCS_AES_GCM_MAX_SIZE;
    }
    else
    {
        if (FCS_GET_COMM_SOURCE(session_id) == FCS_COMM_SOURCE_HPS)
        {
            max_size = FCS_MAX_SIZE_HPS;
        }
        else
        {
            max_size = FCS_MAX_SIZE_FPGA;
        }
    }
    while (data_rem > 0U)
    {
        if (data_rem > max_size)
        {
            aes_input_size = max_size;
            /* doesnt affect non GCM modes */
            if (block_mode == FCS_AES_GCM_GHASH)
            {
                aes_output_size = 0;
            }
            else
            {
                aes_output_size = aes_input_size - (aad_size + padding1);
            }
            ret = run_fcs_aes_update( uuid, context_id,
                    aes_input_data, aes_input_size,
                    output_data, aes_output_size,
                    0, &data_written, FCS_UPDATE );
            /* aad data and padding written to SDM if available */
            aad_size = 0;
            padding1 = 0;
        }
        else
        {
            aes_input_size = data_rem;/* doesnt affect non GCM modes */
            if (block_mode == FCS_AES_GCM_GHASH)
            {
                aes_output_size = 0;
            }
            else
            {
                aes_output_size = aes_input_size -
                        (aad_size + padding1 + input_tag);
            }
            if ((block_mode == FCS_AES_GCM) && (input_tag == 0U))
            {
                aes_output_size += FCS_GCM_TAG_SIZE;
            }
            finalized = 1;
            ret = run_fcs_aes_update( uuid, context_id,
                    aes_input_data, aes_input_size,
                    output_data, aes_output_size,
                    padding2, &data_written, FCS_FINALIZE );
        }
        if (ret == 0)
        {
            data_rem -= data_written;
            aes_output_size += data_written;
            if (finalized != 1)
            {
                aes_input_data += data_written;
                output_data += data_written;
            }
        }
        else
        {
            ERROR("AES_CRYPTION failed");
            break;
        }
    }
    aes_input_data = temp;
    vPortFree(aes_input_data);
    return ret;
}

int run_fcs_ecdsa_hash_sign(
        char *uuid, uint32_t context_id, uint32_t key_id,
        uint32_t ecc_algo, char *hash_data, uint32_t hash_data_size,
        char *signed_data, uint32_t *signed_data_size )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_hash_sign_args[6], hash_sign_smc_resp[2];
    uint32_t session_id = 0U, *hash_sign_mbox_resp;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((ecc_algo != FCS_ECC_NISTP_256) &&
            (ecc_algo != FCS_ECC_NISTP_384) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_256) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_384))
    {
        ERROR("Invalid ECC Algorithm");
        return -EINVAL;
    }
    fcs_hash_sign_args[0] = session_id;
    fcs_hash_sign_args[1] = context_id;
    fcs_hash_sign_args[2] = key_id;
    fcs_hash_sign_args[3] = FCS_ECDSA_PARAM_SIZE;
    fcs_hash_sign_args[4] = ecc_algo;
    ret = sip_svc_send(fcs_handle, FCS_ECDSA_HASH_SIGN_INIT,
            fcs_hash_sign_args, sizeof(fcs_hash_sign_args),
            NULL, 0);

    if (ret != 0)
    {
        return ret;
    }
    hash_sign_mbox_resp = (uint32_t*)pvPortMalloc(
            FCS_ECDSA_HASH_SIGN_MAX_RESP);
    if (hash_sign_mbox_resp == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    cache_force_write_back(hash_data, hash_data_size);
    cache_force_invalidate(hash_sign_mbox_resp,
            FCS_ECDSA_HASH_SIGN_MAX_RESP);

    fcs_hash_sign_args[0] = session_id;
    fcs_hash_sign_args[1] = context_id;
    fcs_hash_sign_args[2] = (uint64_t)hash_data;
    fcs_hash_sign_args[3] = hash_data_size;
    fcs_hash_sign_args[4] = (uint64_t)hash_sign_mbox_resp;
    fcs_hash_sign_args[5] = FCS_ECDSA_HASH_SIGN_MAX_RESP;

    ret = sip_svc_send(fcs_handle, FCS_ECDSA_HASH_SIGN_FINALIZE,
            fcs_hash_sign_args, sizeof(fcs_hash_sign_args),
            hash_sign_smc_resp, sizeof(hash_sign_smc_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (hash_sign_smc_resp[FCS_RESP_STATUS] == 0U))
        {
            cache_force_invalidate((void *) hash_sign_mbox_resp,
                    (size_t) hash_sign_smc_resp[FCS_RESP_SIZE]);
            *signed_data_size = (uint32_t) hash_sign_smc_resp[FCS_RESP_SIZE] -
                    FCS_RESP_HEADER_SIZE;
            (void) memcpy((void *) signed_data,
                    (void *) &hash_sign_mbox_resp[FCS_RESP_DATA],
                    *signed_data_size);
        }
        status = (uint16_t) hash_sign_smc_resp[FCS_RESP_STATUS];
        ret = (int)status;
    }
    vPortFree(hash_sign_mbox_resp);
    return ret;
}
int run_fcs_ecdsa_hash_verify( char *uuid, uint32_t context_id,
        uint32_t key_id, uint32_t ecc_algo, char *hash_data,
        uint32_t hash_data_size, char *sig_data, uint32_t sig_size,
        char *pub_key_data, uint32_t pub_key_size, char *dest_data,
        uint32_t *dest_size )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_hash_sign_verify_args[6], hash_sign_verify_smc_resp[2];
    uint32_t session_id = 0U, *hash_sign_verify_mbox_resp, mbox_arg_size;
    char *hash_sign_verify_mbox_args;

    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((ecc_algo != FCS_ECC_NISTP_256) &&
            (ecc_algo != FCS_ECC_NISTP_384) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_256) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_384))
    {
        ERROR("Invalid ECC Algorithm");
        return -EINVAL;
    }

    fcs_hash_sign_verify_args[0] = session_id;
    fcs_hash_sign_verify_args[1] = context_id;
    fcs_hash_sign_verify_args[2] = key_id;
    fcs_hash_sign_verify_args[3] = FCS_ECDSA_PARAM_SIZE;
    fcs_hash_sign_verify_args[4] = ecc_algo;
    ret = sip_svc_send(fcs_handle, FCS_ECDSA_HASH_SIGN_VERIFY_INIT,
            fcs_hash_sign_verify_args, sizeof(fcs_hash_sign_verify_args),
            NULL, 0);

    if (ret != 0)
    {
        return ret;
    }

    hash_sign_verify_mbox_resp = (uint32_t*)pvPortMalloc(
            FCS_ECDSA_HASH_VERIFY_RESP);
    if (hash_sign_verify_mbox_resp == NULL)
    {
        ERROR("Failed to allocate mempry");
        return -ENOMEM;
    }
    hash_sign_verify_mbox_args = (char*)pvPortMalloc(
            hash_data_size + sig_size + pub_key_size);
    if (hash_sign_verify_mbox_args == NULL)
    {
        ERROR("Failed to allocate mempry");
        return -ENOMEM;
    }
    (void) memcpy(hash_sign_verify_mbox_args, hash_data, hash_data_size);
    (void) memcpy(hash_sign_verify_mbox_args + hash_data_size, sig_data,
            sig_size);
    mbox_arg_size = hash_data_size + sig_size;
    if ((pub_key_data != NULL) && (pub_key_size != 0U))
    {
        (void) memcpy(hash_sign_verify_mbox_args + hash_data_size + sig_size,
                pub_key_data, pub_key_size);
        mbox_arg_size += pub_key_size;
    }
    cache_force_write_back(hash_sign_verify_mbox_args, mbox_arg_size);
    cache_force_invalidate(hash_sign_verify_mbox_resp,
            FCS_ECDSA_HASH_VERIFY_RESP);

    fcs_hash_sign_verify_args[0] = session_id;
    fcs_hash_sign_verify_args[1] = context_id;
    fcs_hash_sign_verify_args[2] = (uint64_t)hash_sign_verify_mbox_args;
    fcs_hash_sign_verify_args[3] = mbox_arg_size;
    fcs_hash_sign_verify_args[4] = (uint64_t)hash_sign_verify_mbox_resp;
    fcs_hash_sign_verify_args[5] = FCS_ECDSA_HASH_VERIFY_RESP;

    ret = sip_svc_send(fcs_handle, FCS_ECDSA_HASH_SIGN_VERIFY_FINALIZE,
            fcs_hash_sign_verify_args, sizeof(fcs_hash_sign_verify_args),
            hash_sign_verify_smc_resp, sizeof(hash_sign_verify_smc_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (hash_sign_verify_smc_resp[FCS_RESP_STATUS] == 0UL))
        {
            cache_force_invalidate((void *) hash_sign_verify_mbox_resp,
                    (size_t) hash_sign_verify_smc_resp[FCS_RESP_SIZE]);
            *dest_size = (uint32_t) hash_sign_verify_smc_resp[FCS_RESP_SIZE] -
                    FCS_RESP_HEADER_SIZE;
            (void) memcpy((void *) dest_data,
                    (void *) &hash_sign_verify_mbox_resp[FCS_RESP_DATA],
                    *dest_size);
        }
        status = (uint16_t) hash_sign_verify_smc_resp[FCS_RESP_STATUS];
        ret = (int)status;
    }
    vPortFree(hash_sign_verify_mbox_resp);
    return ret;
}

static int run_fcs_ecdsa_sha2_data_sign_init( char *uuid,
        uint32_t context_id, uint32_t key_id, uint32_t ecc_algo )
{
    int ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_ecdsa_sha2_init_args[5];
    uint32_t session_id = 0U;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((ecc_algo != FCS_ECC_NISTP_256) &&
            (ecc_algo != FCS_ECC_NISTP_384) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_256) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_384))
    {
        ERROR("Invalid ECC Algorithm");
        return -EINVAL;
    }

    fcs_ecdsa_sha2_init_args[0] = session_id;
    fcs_ecdsa_sha2_init_args[1] = context_id;
    fcs_ecdsa_sha2_init_args[2] = key_id;
    fcs_ecdsa_sha2_init_args[3] = FCS_ECDSA_PARAM_SIZE;
    fcs_ecdsa_sha2_init_args[4] = ecc_algo;
    ret = sip_svc_send(fcs_handle, FCS_ECDSA_SHA2_SIGN_INIT,
            fcs_ecdsa_sha2_init_args, sizeof(fcs_ecdsa_sha2_init_args),
            NULL, 0);

    return ret;
}

static int run_fcs_ecdsa_sha2_data_sign_update( char *uuid,
        uint32_t context_id, char *src_addr, uint32_t src_size,
        char *dest_data, uint32_t *dest_size, uint8_t final )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_sha2_sign_args[8], sha2_sign_smc_resp[2];
    uint32_t session_id = 0U, *sha2_sign_mbox_resp;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    sha2_sign_mbox_resp = (uint32_t*)pvPortMalloc(
            FCS_ECDSA_HASH_SHA2_SIGN_MAX_RESP);
    if (sha2_sign_mbox_resp == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    cache_force_invalidate(sha2_sign_mbox_resp,
            FCS_ECDSA_HASH_SHA2_SIGN_MAX_RESP);
    cache_force_write_back(src_addr, src_size);

    fcs_sha2_sign_args[0] = session_id;
    fcs_sha2_sign_args[1] = context_id;
    fcs_sha2_sign_args[2] = (uint64_t)src_addr;
    fcs_sha2_sign_args[3] = src_size;
    fcs_sha2_sign_args[4] = (uint64_t)sha2_sign_mbox_resp;
    fcs_sha2_sign_args[5] = FCS_ECDSA_HASH_SHA2_SIGN_MAX_RESP;
    fcs_sha2_sign_args[6] = FCS_SMMU_GET_ADDR(src_addr);

    if (final == FCS_UPDATE)
    {
        ret = sip_svc_send(fcs_handle, FCS_ECDSA_SHA2_SIGN_UPDATE,
                fcs_sha2_sign_args, sizeof(fcs_sha2_sign_args),
                sha2_sign_smc_resp, sizeof(sha2_sign_smc_resp));
    }
    else
    {
        ret = sip_svc_send(fcs_handle, FCS_ECDSA_SHA2_SIGN_FINALIZE,
                fcs_sha2_sign_args, sizeof(fcs_sha2_sign_args),
                sha2_sign_smc_resp, sizeof(sha2_sign_smc_resp));
    }
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (sha2_sign_smc_resp[FCS_RESP_STATUS] == 0UL) &&
                (final == FCS_FINALIZE))
        {
            cache_force_invalidate((void *) sha2_sign_mbox_resp,
                    (size_t) sha2_sign_smc_resp[FCS_RESP_SIZE]);
            *dest_size = (uint32_t) sha2_sign_smc_resp[FCS_RESP_SIZE] -
                    FCS_RESP_HEADER_SIZE;
            (void) memcpy((void *) dest_data,
                    (void *) &sha2_sign_mbox_resp[FCS_RESP_DATA], *dest_size);
        }
        status = (uint16_t) sha2_sign_smc_resp[FCS_RESP_STATUS];
        ret = (int)status;
    }
    vPortFree(sha2_sign_mbox_resp);
    return ret;
}
int run_fcs_ecdsa_sha2_data_sign( char *uuid, uint32_t context_id,
        uint32_t key_id, uint32_t ecc_algo, char  *src_data,
        uint32_t src_size, char *dest_data, uint32_t *dest_size )
{
    uint32_t session_id = 0U, max_size, remaining_data = src_size, data_written;
    int ret;
    sdm_client_handle fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    if (FCS_GET_COMM_SOURCE(session_id) == FCS_COMM_SOURCE_HPS)
    {
        max_size = FCS_MAX_SIZE_HPS;
    }
    else
    {
        max_size = FCS_MAX_SIZE_FPGA;
    }
    if (key_id == 0U)
    {
        ERROR("Invalid Key ID");
        return -EINVAL;
    }
    if ((((uint64_t)(uintptr_t)src_data % 8UL) != 0UL) ||
            (dest_data == NULL) ||
            (dest_size == NULL))
    {
        ERROR("Invalid address");
        return -EINVAL;
    }
    if ((src_size < 8U) || ((src_size % 8U) != 0U))
    {
        ERROR("Invalid Size");
        return -EINVAL;
    }

    ret = run_fcs_ecdsa_sha2_data_sign_init( uuid, context_id, key_id,
            ecc_algo );
    if (ret != 0)
    {
        ERROR("Failed to initialise sha2_data_sign");
        return ret;
    }
    while (remaining_data > 0U)
    {
        if (remaining_data > max_size)
        {
            data_written = max_size;
            ret = run_fcs_ecdsa_sha2_data_sign_update( uuid, context_id,
                    src_data, max_size, dest_data, dest_size, FCS_UPDATE );
        }
        else
        {
            data_written = remaining_data;
            ret = run_fcs_ecdsa_sha2_data_sign_update( uuid, context_id,
                    src_data, remaining_data, dest_data, dest_size,
                    FCS_FINALIZE );
        }
        if (ret == 0)
        {
            remaining_data -= data_written;
            src_data += data_written;
        }
        else
        {
            return ret;
        }
    }
    return ret;
}
static int run_fcs_ecdsa_sha2_data_sign_verify_init( char *uuid,
        uint32_t context_id, uint32_t key_id, uint32_t ecc_algo )
{
    sdm_client_handle fcs_handle;
    uint64_t fcs_ecdsa_sha2_init_args[5];
    uint32_t session_id = 0U;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((ecc_algo != FCS_ECC_NISTP_256) &&
            (ecc_algo != FCS_ECC_NISTP_384) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_256) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_384))
    {
        ERROR("Invalid ECC Algorithm");
        return -EINVAL;
    }

    fcs_ecdsa_sha2_init_args[0] = session_id;
    fcs_ecdsa_sha2_init_args[1] = context_id;
    fcs_ecdsa_sha2_init_args[2] = key_id;
    fcs_ecdsa_sha2_init_args[3] = FCS_ECDSA_PARAM_SIZE;
    fcs_ecdsa_sha2_init_args[4] = ecc_algo;
    return sip_svc_send(fcs_handle, FCS_ECDSA_SHA2_SIGN_VERIFY_INIT,
            fcs_ecdsa_sha2_init_args, sizeof(fcs_ecdsa_sha2_init_args),
            NULL, 0);
}
/*
 * Signature data should be stored after the input data
 * If pubkey is provided, ensure its after the signature to be verified
 */
static int run_fcs_ecdsa_sha2_data_sign_verify_update( char *uuid,
        uint32_t context_id, char *src_addr, uint32_t src_size,
        char *signed_data, uint32_t sig_size, char *pub_key_data,
        uint32_t pub_key_size, char *dest_data,
        uint32_t *dest_size, uint8_t final )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_sha2_sign_verify_args[8], sha2_sign_verify_smc_resp[2];
    uint32_t session_id = 0U, *sha2_sign_verify_mbox_resp, payload_size;
    char *sha2_sign_verify_mbox_args;

    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    sha2_sign_verify_mbox_resp = (uint32_t*)pvPortMalloc(
            FCS_ECDSA_HASH_SHA2_VERIFY_RESP);
    if (sha2_sign_verify_mbox_resp == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    sha2_sign_verify_mbox_args = (char*)pvPortMalloc(
            src_size + sig_size + pub_key_size);

    if (sha2_sign_verify_mbox_args == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }

    (void) memcpy(sha2_sign_verify_mbox_args, (char*)(uintptr_t)src_addr,
            src_size);
    (void) memcpy(sha2_sign_verify_mbox_args + src_size, signed_data, sig_size);
    payload_size = src_size + sig_size;
    if (pub_key_data != NULL)
    {
        (void) memcpy(sha2_sign_verify_mbox_args + payload_size, pub_key_data,
                pub_key_size);
        payload_size += pub_key_size;
    }
    cache_force_invalidate(sha2_sign_verify_mbox_resp,
            FCS_ECDSA_HASH_SHA2_VERIFY_RESP);
    cache_force_write_back(sha2_sign_verify_mbox_args, payload_size);

    fcs_sha2_sign_verify_args[0] = session_id;
    fcs_sha2_sign_verify_args[1] = context_id;
    fcs_sha2_sign_verify_args[2] = (uint64_t)sha2_sign_verify_mbox_args;
    fcs_sha2_sign_verify_args[3] = payload_size;
    fcs_sha2_sign_verify_args[4] = (uint64_t)sha2_sign_verify_mbox_resp;
    fcs_sha2_sign_verify_args[5] = FCS_ECDSA_HASH_SHA2_VERIFY_RESP;
    fcs_sha2_sign_verify_args[6] = src_size;
    fcs_sha2_sign_verify_args[7] =
            FCS_SMMU_GET_ADDR(sha2_sign_verify_mbox_args);

    if (final == FCS_UPDATE)
    {
        ret = sip_svc_send(fcs_handle, FCS_ECDSA_SHA2_SIGN_VERIFY_UPDATE,
                fcs_sha2_sign_verify_args, sizeof(fcs_sha2_sign_verify_args),
                sha2_sign_verify_smc_resp, sizeof(sha2_sign_verify_smc_resp));
    }
    else
    {
        ret = sip_svc_send(fcs_handle, FCS_ECDSA_SHA2_SIGN_VERIFY_FINALIZE,
                fcs_sha2_sign_verify_args, sizeof(fcs_sha2_sign_verify_args),
                sha2_sign_verify_smc_resp, sizeof(sha2_sign_verify_smc_resp));
    }
    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (sha2_sign_verify_smc_resp[FCS_RESP_STATUS] == 0UL))
        {
            cache_flush((void *) sha2_sign_verify_mbox_resp,
                    (size_t) sha2_sign_verify_smc_resp[FCS_RESP_SIZE]);
            *dest_size = (uint32_t) sha2_sign_verify_smc_resp[FCS_RESP_SIZE] -
                    FCS_RESP_HEADER_SIZE;
            (void) memcpy((void *) dest_data,
                    (void *) &sha2_sign_verify_mbox_resp[FCS_RESP_DATA],
                    *dest_size);
        }
        status = (uint16_t) sha2_sign_verify_smc_resp[FCS_RESP_STATUS];
        ret = (int)status;
    }
    vPortFree(sha2_sign_verify_mbox_resp);
    return ret;
}
int run_fcs_ecdsa_sha2_data_sign_verify( char *uuid,
        uint32_t context_id, uint32_t key_id, uint32_t ecc_algo,
        char *src_data, uint32_t src_size, char *signed_data,
        uint32_t sig_size, char *pub_key_data, uint32_t pub_key_size,
        char *dest_data, uint32_t *dest_size )
{
    uint32_t session_id = 0U, max_size,
            remaining_data = src_size + sig_size + pub_key_size, data_written;
    int ret;
    sdm_client_handle fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }

    if (FCS_GET_COMM_SOURCE(session_id) == FCS_COMM_SOURCE_HPS)
    {
        max_size = FCS_MAX_SIZE_HPS;
    }
    else
    {
        max_size = FCS_MAX_SIZE_FPGA;
    }
    if ((key_id == 0U) && (pub_key_data == NULL))
    {
        ERROR("Invalid Key ID");
        return -EINVAL;
    }
    if ((((uint64_t)(uintptr_t)src_data % 8UL) != 0UL) ||
            (dest_data == NULL) ||
            (dest_size == NULL))
    {
        ERROR("Invalid address");
        return -EINVAL;
    }
    if ((src_size < 8U) || ((src_size % 8U) != 0U))
    {
        ERROR("Invalid Size");
        return -EINVAL;
    }

    ret = run_fcs_ecdsa_sha2_data_sign_verify_init( uuid, context_id,
            key_id,
            ecc_algo );
    if (ret != 0)
    {
        ERROR("Failed to initialise sha_data_sign_verify");
        return ret;
    }
    while (remaining_data > 0U)
    {
        if (remaining_data > max_size)
        {
            if ((remaining_data - max_size) >= (8 + sig_size + pub_key_size))
            {
                data_written = max_size;
                ret = run_fcs_ecdsa_sha2_data_sign_verify_update( uuid,
                        context_id,
                        src_data, data_written, signed_data,
                        0, pub_key_data, 0,
                        dest_data, dest_size, FCS_UPDATE );
            }
            else
            {
                data_written = remaining_data - (8 + sig_size + pub_key_size);
                ret = run_fcs_ecdsa_sha2_data_sign_verify_update( uuid,
                        context_id,
                        src_data, data_written, signed_data,
                        0, pub_key_data, 0,
                        dest_data, dest_size, FCS_UPDATE );
            }
        }
        else
        {
            data_written = remaining_data;
            ret = run_fcs_ecdsa_sha2_data_sign_verify_update( uuid,
                    context_id,
                    src_data, src_size, signed_data,
                    sig_size,
                    pub_key_data,
                    pub_key_size, dest_data, dest_size, FCS_FINALIZE );
        }
        if (ret == 0)
        {
            remaining_data -= data_written;
            src_data += data_written;
        }
        else
        {
            return ret;
        }
    }
    return ret;
}

int run_fcs_ecdsa_get_public_key(  char *uuid, uint32_t context_id,
        uint32_t key_id, uint32_t ecc_algo, char *pub_key_data,
        uint32_t *pub_key_size )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_get_pubkey_args[6], get_pubkey_smc_resp[2];
    uint32_t session_id = 0U, *get_pubkey_mbox_resp;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((ecc_algo != FCS_ECC_NISTP_256) &&
            (ecc_algo != FCS_ECC_NISTP_384) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_256) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_384))
    {
        ERROR("Invalid ECC Algorithm");
        return -EINVAL;
    }

    fcs_get_pubkey_args[0] = session_id;
    fcs_get_pubkey_args[1] = context_id;
    fcs_get_pubkey_args[2] = key_id;
    fcs_get_pubkey_args[3] = FCS_ECDSA_PARAM_SIZE;
    fcs_get_pubkey_args[4] = ecc_algo;
    ret = sip_svc_send(fcs_handle, FCS_GET_PUBKEY_INIT,
            fcs_get_pubkey_args, sizeof(fcs_get_pubkey_args),
            NULL, 0);

    if (ret != 0)
    {
        return ret;
    }
    get_pubkey_mbox_resp = (uint32_t*)pvPortMalloc(
            FCS_GET_PUBKEY_RESP);
    if (get_pubkey_mbox_resp == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    cache_force_invalidate(get_pubkey_mbox_resp, FCS_GET_PUBKEY_RESP);

    fcs_get_pubkey_args[0] = session_id;
    fcs_get_pubkey_args[1] = context_id;
    fcs_get_pubkey_args[2] = (uint64_t)get_pubkey_mbox_resp;
    fcs_get_pubkey_args[3] = FCS_GET_PUBKEY_RESP;
    ret = sip_svc_send(fcs_handle, FCS_GET_PUBKEY_FINALIZE,
            fcs_get_pubkey_args, sizeof(fcs_get_pubkey_args),
            get_pubkey_smc_resp, sizeof(get_pubkey_smc_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) &&
                (get_pubkey_smc_resp[FCS_RESP_STATUS] == 0UL))
        {
            *pub_key_size = (uint32_t) get_pubkey_smc_resp[FCS_RESP_SIZE] -
                    FCS_RESP_HEADER_SIZE;
            (void) memcpy((void *) pub_key_data,
                    (void *) &get_pubkey_mbox_resp[FCS_RESP_DATA],
                    *pub_key_size);
        }
        status = (uint16_t)get_pubkey_smc_resp[FCS_RESP_STATUS];
        ret = (int)status;
    }
    vPortFree(get_pubkey_mbox_resp);
    return ret;
}

int run_fcs_ecdh_request(  char *uuid, uint32_t key_id,
        uint32_t context_id, uint32_t ecc_algo, char *pub_key_data,
        uint32_t pub_key_size, char *shared_sec_data,
        uint32_t *shared_sec_size )
{
    int ret;
    uint16_t status;
    bool sem_ret;
    sdm_client_handle fcs_handle;
    uint64_t fcs_ecdh_args[6], ecdh_smc_resp[2];
    uint32_t session_id = 0U, *ecdh_mbox_resp;
    fcs_handle = get_client_handle(uuid, &session_id);
    if (fcs_handle == NULL)
    {
        ERROR("Failed to locate client");
        return -EIO;
    }
    if ((ecc_algo != FCS_ECC_NISTP_256) &&
            (ecc_algo != FCS_ECC_NISTP_384) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_256) &&
            (ecc_algo != FCS_ECC_BRAINPOOL_384))
    {
        ERROR("Invalid ECC Algorithm");
        return -EINVAL;
    }
    if ((pub_key_data == NULL) || (pub_key_size == 0U))
    {
        ERROR("Invalid pubkey details");
        return -EINVAL;
    }
    fcs_ecdh_args[0] = session_id;
    fcs_ecdh_args[1] = context_id;
    fcs_ecdh_args[2] = key_id;
    fcs_ecdh_args[3] = FCS_ECDSA_PARAM_SIZE;
    fcs_ecdh_args[4] = ecc_algo;
    ret = sip_svc_send(fcs_handle, FCS_ECDH_INIT,
            fcs_ecdh_args, sizeof(fcs_ecdh_args),
            NULL, 0);

    if (ret != 0)
    {
        return ret;
    }

    ecdh_mbox_resp = (uint32_t*)pvPortMalloc(FCS_ECDH_MAX_RESP);
    if (ecdh_mbox_resp == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    cache_force_invalidate(ecdh_mbox_resp, FCS_ECDH_MAX_RESP);
    cache_force_write_back(pub_key_data, pub_key_size);

    *shared_sec_size = FCS_ECDH_MAX_RESP;
    fcs_ecdh_args[0] = session_id;
    fcs_ecdh_args[1] = context_id;
    fcs_ecdh_args[2] = (uint64_t)pub_key_data;
    fcs_ecdh_args[3] = pub_key_size;
    fcs_ecdh_args[4] = (uint64_t)ecdh_mbox_resp;
    fcs_ecdh_args[5] = FCS_ECDH_MAX_RESP;

    ret = sip_svc_send(fcs_handle, FCS_ECDH_FINALIZE,
            fcs_ecdh_args, sizeof(fcs_ecdh_args),
            ecdh_smc_resp, sizeof(ecdh_smc_resp));

    if (ret == 0)
    {
        sem_ret = osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER);
        if ((sem_ret == true) && (ecdh_smc_resp[FCS_RESP_STATUS] == 0UL))
        {
            cache_force_invalidate((void*) ecdh_mbox_resp,
                    (size_t) ecdh_smc_resp[FCS_RESP_SIZE]);
            *shared_sec_size = (uint32_t) ecdh_smc_resp[FCS_RESP_SIZE] -
                    FCS_RESP_HEADER_SIZE;
            (void) memcpy((void *) shared_sec_data,
                    (void *) &ecdh_mbox_resp[FCS_RESP_DATA],
                    *shared_sec_size);
        }
        status = (uint16_t)ecdh_smc_resp[FCS_RESP_STATUS];
        ret = (int)status;
    }
    vPortFree(ecdh_mbox_resp);
    return ret;
}
int run_fcs_qspi_open(void)
{
    int ret;
    uint16_t status;
    uint64_t qspi_open_err;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_QSPI_OPEN,
            NULL, 0, &qspi_open_err, sizeof(qspi_open_err));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            status = (uint16_t)qspi_open_err;
            ret = (int)status;
        }
    }
    return ret;
}
int run_fcs_qspi_close(void)
{
    int ret;
    uint16_t status;
    uint64_t qspi_close_err;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_QSPI_CLOSE,
            NULL, 0, &qspi_close_err, sizeof(qspi_close_err));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            status = (uint16_t)(qspi_close_err);
            ret = (int)status;
        }
    }
    return ret;
}
int run_fcs_qspi_set_cs(uint32_t chip_sel_info)
{
    int ret;
    uint16_t status;
    uint64_t qspi_chip_sel_args[3], qspi_chip_sel_err;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    qspi_chip_sel_args[0] = GET_QSPI_CHIP_SEL((uint64_t) chip_sel_info);
    qspi_chip_sel_args[1] = GET_QSPI_CA((uint64_t) chip_sel_info);
    qspi_chip_sel_args[2] = GET_QSPI_MODE((uint64_t) chip_sel_info);
    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_QSPI_CHIP_SELECT,
            qspi_chip_sel_args, sizeof(qspi_chip_sel_args),
            &qspi_chip_sel_err, sizeof(qspi_chip_sel_err));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            status = (uint16_t) qspi_chip_sel_err;
            ret = (int)status;
        }
    }
    return ret;
}
int run_fcs_qspi_read(uint32_t qspi_addr, uint32_t data_len, char *buffer)
{
    int ret;
    uint16_t status;
    uint64_t qspi_read_args[3], qspi_read_resp[2];
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    if (data_len > 1024U)
    {
        ERROR("Exceeding maximum size");
        return -EINVAL;
    }
    if (data_len == 0U)
    {
        ERROR("No size specified");
        return -EINVAL;
    }
    if (buffer == NULL)
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }

    qspi_read_args[0] = qspi_addr;
    qspi_read_args[1] = (uint64_t)buffer;
    qspi_read_args[2] = (uint64_t) data_len * MBOX_WORD_SIZE;
    cache_force_invalidate(buffer, data_len * MBOX_WORD_SIZE);

    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_QSPI_READ,
            qspi_read_args, sizeof(qspi_read_args),
            qspi_read_resp, sizeof(qspi_read_resp));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            status = (uint16_t) qspi_read_resp[FCS_RESP_STATUS];
            ret = (int)status;
            if (ret == 0)
            {
                INFO("Read %ld bytes", qspi_read_resp[FCS_RESP_SIZE]);
            }
        }
    }
    return ret;
}
int run_fcs_qspi_write(uint32_t qspi_addr, uint32_t data_len, char *buffer)
{
    int ret;
    uint16_t status;
    uint64_t qspi_write_args[2], qspi_write_err;
    uint32_t *qspi_payload;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("FCS is not initialised");
        return -EIO;
    }
    if (data_len > 1024U)
    {
        ERROR("Exceeding maximum size");
        return -EINVAL;
    }
    if (data_len == 0U)
    {
        ERROR("No size specified");
        return -EINVAL;
    }
    if (buffer == NULL)
    {
        ERROR("No buffer provided");
        return -EINVAL;
    }
    /*
     * Allocate memory to store the address, number of words and the words
     * to be written
     */
    qspi_payload = (uint32_t*)pvPortMalloc(
            sizeof(qspi_addr) + sizeof(data_len) + (data_len * MBOX_WORD_SIZE));
    if (qspi_payload == NULL)
    {
        ERROR("Failed to allocate memory");
        return -ENOMEM;
    }
    /* Formatting the payload */
    qspi_payload[0] = qspi_addr;
    qspi_payload[1] = data_len;
    (void) memcpy((void *) &qspi_payload[2], (void *)buffer,
            data_len * MBOX_WORD_SIZE );

    qspi_write_args[0] = (uint64_t)qspi_payload;
    /* Adding two to account for the size of the qspi_addr and data_len */
    qspi_write_args[1] = MBOX_WORD_SIZE * ((uint64_t) data_len + 2UL);
    cache_force_write_back(qspi_payload, data_len * MBOX_WORD_SIZE);

    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_QSPI_WRITE,
            qspi_write_args, sizeof(qspi_write_args),
            &qspi_write_err, sizeof(qspi_write_err));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == true)
        {
            status = (uint16_t)qspi_write_err;
            ret = (int)status;
        }
    }
    return ret;
}
int run_fcs_qspi_erase(uint32_t qspi_addr, uint32_t data_len)
{
    int ret;
    uint16_t status;
    uint64_t qspi_erase_args[2], qspi_erase_err;
    if (fcs_descriptor->security_handle == NULL)
    {
        ERROR("Security driver not initialised");
        return -EIO;
    }
    if ((data_len % 1024U) != 0U)
    {
        ERROR("Length in words not a multiple of 1024");
        return -EINVAL;
    }
    if (data_len == 0U)
    {
        ERROR("No size specified");
        return -EINVAL;
    }

    qspi_erase_args[0] = qspi_addr;
    qspi_erase_args[1] = data_len;
    ret = sip_svc_send(fcs_descriptor->security_handle, FCS_QSPI_ERASE,
            qspi_erase_args, sizeof(qspi_erase_args),
            &qspi_erase_err, sizeof(qspi_erase_err));

    if (ret == 0)
    {
        if (osal_semaphore_wait(fcs_descriptor->fcs_sem,
                OSAL_TIMEOUT_WAIT_FOREVER) == false)
        {
            status = (uint16_t)qspi_erase_err;
            ret = (int)status;
        }
    }
    return ret;
}

void fcs_callback( uint64_t *resp_values )
{
    (void)resp_values;
    (void) osal_semaphore_post(fcs_descriptor->fcs_sem);
}
