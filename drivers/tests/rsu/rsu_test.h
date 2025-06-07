#ifndef _RSU_TEST_H_
#define _RSU_TEST_H_

#include <libRSU.h>

#define iot_rsu_init                      librsu_init
#define iot_rsu_slot_count                rsu_slot_count
#define iot_rsu_slot_erase                rsu_slot_erase
#define iot_rsu_status_log                rsu_status_log
#define iot_rsu_slot_get_info             rsu_slot_get_info
#define iot_rsu_slot_program_file         rsu_slot_program_file
#define iot_rsu_slot_verify_file          rsu_slot_verify_file
#define iot_rsu_slot_load_after_reboot    rsu_slot_load_after_reboot
#define iot_rsu_max_retry                 rsu_max_retry
#define iot_rsu_slot_priority             rsu_slot_priority
#define iot_rsu_dcmf_status               rsu_dcmf_status
#define iot_rsu_dcmf_status               rsu_dcmf_status
#define iot_rsu_dcmf_version              rsu_dcmf_version
#define iot_rsu_exit                      librsu_exit

#endif
