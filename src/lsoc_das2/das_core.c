/**
 *
 *  Filename:      das_core.c
 *
 *  Description:   DAS policy engine
 *
 *  Created by strawmanbobi 2022-01-10
 *
 *  Copyright (c) 2022 Aliyun-IoT
 *
 **/

#include <stdlib.h>

#include "proto/pb_encode.h"
#include "proto/pb_decode.h"
#include "proto/lsoc_pb.h"
#include "proto/lsoc_defs.h"

#include "das_platform.h"
#include "das_service.h"
#include "das_hardware.h"
#include "das_sha1.h"

#include "das.h"

static const char DAS_VERSION[] = "das:2.0.1";
static const char DAS_PROFILE[] = "profile:rtos|lv";
static const char DAS_FIRMWARE[] = "firmware:";

static const char DAS_PLATFORM[] = "os:rtos,arch:"DAS_ARCH;

static const char DAS_FIL_TAG[] = "file:";
static const char DAS_KEY_DEVICE_ID[] = "uid:";
static const char DAS_KEY_SEPARATOR = ',';

#define DEVICE_ID_MAX_LEN 96
#define DEFAULT_FIRMWARE_VERSION    "0.0.0"

// protocol : auth_code and ID in session of Message
#define MES_SESS_KEY_AUTH_CODE "auth_code:"
#define MES_SESS_KEY_ID     "id:"
#define ID2_AUTH_CODE_MAX   (256)

// protocol : auth_code and ID in session of AtsDev
#define ATS_DEV_KEY_CHALLENGE "challenge:"
#define ATS_DEV_KEY_AUTH_CODE "auth_code:"
#define ATS_DEV_KEY_ID2_ID    "id:"
#define ID2_CHALLANGE_AUTH_CODE_MAX   (256)

#define UPT_CMD_KEY_TOKEN   "token:"
#define UPT_CMD_KEY_OTP     "otp:"


char g_fw_ver[DEVICE_ID_MAX_LEN] = {0};
char g_dev_id[DEVICE_ID_MAX_LEN] = {0};

#ifdef DAS_DEBUG

#define DAS_LOG_ATS_CMD(ss, m)                                          \
    DAS_LOG("ats_cmd  - domain: %d\n", m.domain);                \
    DAS_LOG("ats_cmd  - code: %d\n", m.code);                    \
    DAS_LOG("ats_cmd  - time_ms: 0x%lx\n", (long)m.timestamp);   \
    DAS_LOG("ats_cmd  - which_data: %d\n", m.which_data);        \
    DAS_LOG("ats_cmd  - type: %d\n", m.data.atsCmdData.type);        \
    DAS_LOG("ats_cmd  - patternUrl: %s\n", ss->pbb.ats_cmd_ptn_url);        \
    DAS_LOG("ats_cmd  - pattern: %s\n", ss->pbb.ats_cmd_ptn);

#define DAS_LOG_MES_CMD(ss, m)                                              \
    DAS_LOG("mes_cmd  - domain: %d\n", m.domain);                \
    DAS_LOG("mes_cmd  - code: %d\n", m.code);                    \
    DAS_LOG("mes_cmd  - time_ms: 0x%lx\n", (long)m.timestamp);   \
    DAS_LOG("mes_cmd  - which_data: %d\n", m.which_data);        \
    DAS_LOG("mes_cmd  - type: %d\n", m.data.mesCmdData.type);

#define DAS_LOG_UPT_CMD(ss, m)                                              \
    DAS_LOG("upt_cmd  - domain: %d\n", m.domain);                \
    DAS_LOG("upt_cmd  - code: %d\n", m.code);                    \
    DAS_LOG("upt_cmd  - time_ms: 0x%lx\n", (long)m.timestamp);   \
    DAS_LOG("upt_cmd  - which_data: %d\n", m.which_data);        \
    DAS_LOG("upt_cmd  - type: %d\n", m.data.uptCmdData.type);        \
    DAS_LOG("upt_cmd  - url: %s\n", ss->pbb.upt_cmd_url);        \
    DAS_LOG("upt_cmd  - object: %s\n", ss->pbb.upt_cmd_obj);

#define DAS_LOG_INFO(ss, m)                                               \
    DAS_LOG("dev_inf  - domain: %d\n", m->domain);                \
    DAS_LOG("dev_inf  - code: %d\n", m->code);                    \
    DAS_LOG("dev_inf  - time_ms: 0x%lx\n", (long)m->timestamp);   \
    DAS_LOG("dev_inf  - which_data: %d\n", m->which_data);        \
    DAS_LOG("dev_inf  - version: %s\n", ss->pbb.inf_ver); \
    DAS_LOG("dev_inf  - device: %s\n", ss->pbb.inf_dev);   \
    DAS_LOG("dev_inf  - state: %s\n", ss->pbb.inf_state);     \
    DAS_LOG("dev_inf  - session : %s\n", ss->pbb.msg_session);

#define DAS_LOG_ATS_FIL(ss, m)                                                \
    DAS_LOG("ats_fil  - domain: %d\n", m->domain);                    \
    DAS_LOG("ats_fil  - code: %d\n", m->code);                        \
    DAS_LOG("ats_fil  - time_ms: 0x%lx\n", (long)m->timestamp);       \
    DAS_LOG("ats_fil  - which_data: %d\n", m->which_data);            \
    DAS_LOG("ats_fil  - cmd: 0x%lx\n", (long)m->data.atsFilData.cmd); \
    DAS_LOG("ats_fil  - path: %s\n", ss->pbb.ats_fil_path);        \
    DAS_LOG("ats_fil  - sum: %s\n", ss->pbb.ats_fil_sig);    \
    DAS_LOG("ats_fil  - attribute: %s\n", ss->pbb.ats_fil_attr); \
    DAS_LOG("ats_fil  - session : %s\n", ss->pbb.msg_session);

#define DAS_LOG_ATS_DEV(ss, m)                                                \
    DAS_LOG("ats_dev  - domain: %d\n", m->domain);                    \
    DAS_LOG("ats_dev  - code: %d\n", m->code);                        \
    DAS_LOG("ats_dev  - time_ms: 0x%lx\n", (long)m->timestamp);       \
    DAS_LOG("ats_dev  - which_data: %d\n", m->which_data);            \
    DAS_LOG("ats_dev  - cmd: 0x%lx\n", (long)m->data.atsDevData.cmd); \
    DAS_LOG("ats_dev  - object: %s\n", ss->pbb.ats_dev_obj);    \
    DAS_LOG("ats_dev  - session : %s\n", ss->pbb.msg_session);

#define DAS_LOG_ATS_END(ss, m)                                                \
    DAS_LOG("ats_end  - domain: %d\n", m->domain);                    \
    DAS_LOG("ats_end  - code: %d\n", m->code);                        \
    DAS_LOG("ats_end  - time_ms: 0x%lx\n", (long)m->timestamp);       \
    DAS_LOG("ats_end  - which_data: %d\n", m->which_data);            \
    DAS_LOG("ats_end  - cmd: 0x%lx\n", (long)m->data.atsEndData.cmd); \
    DAS_LOG("ats_end  - total: %d\n", m->data.atsEndData.total);      \
    DAS_LOG("ats_end  - sum: %s\n", ss->pbb.ats_end_sum);          \
    DAS_LOG("ats_end  - session : %s\n", ss->pbb.msg_session);

#define DAS_LOG_MES_FIL(ss, m)                                                \
    DAS_LOG("mes_fil  - domain: %d\n", m->domain);                    \
    DAS_LOG("mes_fil  - code: %d\n", m->code);                        \
    DAS_LOG("mes_fil  - time_ms: 0x%lx\n", (long)m->timestamp);       \
    DAS_LOG("mes_fil  - which_data: %d\n", m->which_data);            \
    DAS_LOG("mes_fil  - cmd: 0x%lx\n", (long)m->data.mesFilData.cmd); \
    DAS_LOG("mes_fil  - sum: %s\n", ss->pbb.mes_fil_sum);          \
    DAS_LOG("mes_fil  - mac: %s\n", ss->pbb.mes_fil_mac);          \
    DAS_LOG("mes_fil  - session : %s\n", ss->pbb.msg_session);

#define DAS_LOG_MES_END(ss, m)                                                \
    DAS_LOG("mes_end  - domain: %d\n", m->domain);                    \
    DAS_LOG("mes_end  - code: %d\n", m->code);                        \
    DAS_LOG("mes_end  - time_ms: 0x%lx\n", (long)m->timestamp);       \
    DAS_LOG("mes_end  - which_data: %d\n", m->which_data);            \
    DAS_LOG("mes_end  - cmd: 0x%lx\n", (long)m->data.mesEndData.cmd); \
    DAS_LOG("mes_end  - total: %d\n", m->data.mesEndData.total);      \
    DAS_LOG("mes_end  - session : %s\n", ss->pbb.msg_session);

#define DAS_LOG_STEPPING(ss, m)                                          \
    DAS_LOG("stepping - step: %d\n", m->step);                   \
    DAS_LOG("stepping - service_index: %d\n", m->service_index);

#define DAS_CORE_ASSERT(_x)                             \
    do {                                                \
        if (!(_x)) {                                    \
            DAS_LOG("ASSERT FAILURE:\n");                \
            DAS_LOG("%s (%d): %s\n",                     \
                    __FILE__, __LINE__, __FUNCTION__);  \
            while (1) /* loop */;                       \
        }                                               \
    } while (0)

#else

#define DAS_LOG_INFO(ss, m)
#define DAS_LOG_ATS_CMD(ss, m)
#define DAS_LOG_MES_CMD(ss, m)
#define DAS_LOG_UPT_CMD(ss, m)
#define DAS_LOG_ATS_FIL(ss, m)
#define DAS_LOG_ATS_DEV(ss, m)
#define DAS_LOG_ATS_END(ss, m)
#define DAS_LOG_MES_FIL(ss, m)
#define DAS_LOG_MES_END(ss, m)
#define DAS_LOG_STEPPING(ss, m)

#define DAS_CORE_ASSERT(_x)

#endif

#define DAS_SESSION_MAGIC     0x73736164

#define DAS_PUB_TOPIC(s,p,d)  { \
        char *_o=s->pub_topic;  \
        int _l=strlen("/sys/"); \
        memcpy(_o,"/sys/",_l);  \
        _o+=_l;_l=strlen(p);    \
        memcpy(_o,p,_l);        \
        _o+=_l;*_o='/';         \
        _o++;_l=strlen(d);      \
        memcpy(_o,d,_l);_o+=_l; \
        _l=strlen("/security/upstream"); \
        memcpy(_o,"/security/upstream",_l); \
    }
#define DAS_SUB_TOPIC(s,p,d) { \
        char *_o=s->sub_topic;  \
        int _l=strlen("/sys/"); \
        memcpy(_o,"/sys/",_l);  \
        _o+=_l;_l=strlen(p);    \
        memcpy(_o,p,_l);        \
        _o+=_l;*_o='/';         \
        _o++;_l=strlen(d);      \
        memcpy(_o,d,_l);_o+=_l; \
        _l=strlen("/security/downstream"); \
        memcpy(_o,"/security/downstream",_l); \
    }

typedef enum {
    DAS_CMD_NONE = 0,
    DAS_CMD_ATTEST_FULL = 1,
    DAS_CMD_ATTEST_DEV = 2, // simple command : response immediately
    DAS_CMD_MEASURE = 3,
    DAS_CMD_UPDATE = 4, // simple command : do immediately
} das_cmd_t;

typedef enum {
    DAS_STEP_IDLE = 0,
    DAS_STEP_INFO = 1,
    DAS_STEP_ATTEST_FULL = 2,
    DAS_STEP_MEASURE = 3,
} das_step_t;

typedef struct _das_proto_buffer {
    char ats_cmd_ptn_url[MAX_ATS_CMD_PTN_URL_SIZE];
    char ats_cmd_ptn[MAX_ATS_CMD_PTN_SIZE];

    char upt_cmd_url[MAX_UPT_CMD_URL_SIZE];
    char upt_cmd_obj[MAX_UPT_CMD_OBJ_SIZE];

    char inf_ver[MAX_INF_VERSION_SIZE];
    char inf_dev[MAX_INF_DEVICE_SIZE];
    char inf_state[MAX_INF_STATE_SIZE];

    char ats_fil_path[MAX_ATS_FIL_PATH_SIZE];
    char ats_fil_attr[MAX_ATS_FIL_ATTR_SIZE];
    char ats_fil_sig[MAX_ATS_FIL_SIG_SIZE];

    char ats_dev_obj[MAX_ATS_DEV_OBJ_SIZE];

    char ats_end_sum[MAX_ATS_END_SUM_SIZE];

    char mes_fil_sum[MAX_MES_FIL_SUM_SIZE];
    char mes_fil_mac[MAX_MES_FIL_MAC_SIZE];

    char msg_session[MAX_MSG_SESS_SIZE];
} das_proto_buffer_t;

typedef struct _das_session {
    uint32_t magic;         /* magic number */

    char pub_topic[DAS_TOPIC_LENGTH];
    char sub_topic[DAS_TOPIC_LENGTH];

    void *channel;          /* communication channel handle */
    bool connected;
    publish_handle_t publish_handle;

    das_cmd_t cmd;
    uint64_t cmd_id;
    char cmd_buffer[DAS_CMD_BUFFER_LENGTH];

    uint64_t info_time;

    Message report;
    uint32_t report_number;
    // this buffer is used as an adjustable buffer for 'Message'
    das_proto_buffer_t pbb;

    das_step_t step;
    uint64_t stepping_now;
    das_service_t *stepping_service;

    das_sha1_context_t stepping_sum_context;
    das_sha1_context_t stepping_mac_context;

    // upload and download may in different threads/tasks
    uint8_t stepping_buffer[DAS_BUFFER_LENGTH];

    uint32_t service_index;
    das_service_state_t service_state;

    bool is_id2_prov;
} das_session_t;

static das_session_t _g_session = {0};
// For first info stage, we only report heartbeat
static bool _g_is_first_info = true;

void *das_init(const char *product_name, const char *device_name) {
    das_session_t *ss = &_g_session;
    das_service_t *service;
    int i;

    if (product_name == NULL || device_name == NULL) {
        DAS_LOG("invalid input args\n");
        return NULL;
    }

#if(DAS_SERVICE_ID2_ID)
    if (IROT_SUCCESS != lite_id2_client_init()) {
        DAS_LOG("id2 init error\n");
        return NULL;
    }
#endif

    ss->magic = DAS_SESSION_MAGIC;

    DAS_PUB_TOPIC(ss, product_name, device_name);
    DAS_SUB_TOPIC(ss, product_name, device_name);

    for (i = 0; das_service_table[i] != NULL; i++) {
        service = das_service_table[i];

        if (!service->name) {
            DAS_LOG("invalid service %d\n", i);
            return NULL;
        }
        if (service->init) {
            service->init();
        }
    }

    ss->channel = NULL;
    ss->publish_handle = NULL;

    ss->cmd = DAS_CMD_NONE;
    ss->info_time = 0;
    ss->step = DAS_STEP_IDLE;

    return ss;
}

void das_final(void *session) {
    das_service_t *service;
    int i;
    /* session is static, do nothing*/
    DAS_LOG("das session finalized.\n");

    for (i = 0; das_service_table[i] != NULL; i++) {
        service = das_service_table[i];

        if (!service->name) {
            DAS_LOG("invalid service %d\n", i);
            return;
        }

        if (service->deinit) {
            service->deinit();
        }
    }
}

static const char *_get_sub_topic(das_session_t *session) {
    return session->sub_topic;
}

const char *das_sub_topic(void *session, const char *topic) {
    uint32_t topic_len;
    das_session_t *ss = (das_session_t *)session;

    if (ss == NULL) {
        DAS_LOG("invalid input args\n");
        return NULL;
    }

    if (topic == NULL) {
        return _get_sub_topic(ss);
    }

    topic_len = strlen(topic);
    if (topic_len > DAS_TOPIC_LENGTH - 1) {
        return NULL;
    }

    memcpy(ss->sub_topic, topic, topic_len);
    ss->sub_topic[topic_len] = '\0';

    return ss->sub_topic;
}

static const char *_get_pub_topic(das_session_t *session) {
    return session->pub_topic;
}

const char *das_pub_topic(void *session, const char *topic) {
    uint32_t topic_len;
    das_session_t *ss = (das_session_t *)session;

    if (ss == NULL) {
        DAS_LOG("invalid input args\n");
        return NULL;
    }

    if (topic == NULL) {
        return _get_pub_topic(ss);
    }

    topic_len = strlen(topic);
    if (topic_len > DAS_TOPIC_LENGTH - 1) {
        DAS_LOG("short buffer, %d\n", topic_len);
        return NULL;
    }

    memcpy(ss->pub_topic, topic, topic_len);
    ss->pub_topic[topic_len] = '\0';

    return ss->pub_topic;
}

void das_connection(void *session, publish_handle_t publish_handle, void *channel) {
    das_session_t *ss = (das_session_t *)session;

    if (session == NULL || publish_handle == NULL || channel == NULL) {
        DAS_LOG("invalid input args\n");
        DAS_CORE_ASSERT(0);
        return;
    }

    if (ss->magic != DAS_SESSION_MAGIC) {
        DAS_LOG("invalid session magic, 0x%x\n", ss->magic);
        DAS_CORE_ASSERT(0);
        return;
    }

    ss->channel = channel;
    ss->publish_handle = publish_handle;
    ss->connected = false;

    return;
}

void das_on_connected(void *session) {
    das_session_t *ss = (das_session_t *)session;

    DAS_CORE_ASSERT(session != NULL && ss->magic == DAS_SESSION_MAGIC);

    ss->connected = true;

    return;
}

void das_on_disconnected(void *session) {
    das_session_t *ss = (das_session_t *)session;

    DAS_CORE_ASSERT(session != NULL && ss->magic == DAS_SESSION_MAGIC);

    ss->connected = false;

    return;
}

#if defined DAS2_FEATURE_ATTESTATION
das_result_t _do_update_cmd(const char * cmd_buffer,
                            char * out_buffer, int out_buffer_size,
                            bool * is_id2_prov) {
    das_result_t res = DAS_SUCCESS;
    lite_irot_result_t id2_res = IROT_SUCCESS;
    char delim_comma[] = ",";
    //    char delim_colon[] = ":";
    char ** result_pair = NULL;
    size_t i, nb_pair;
    uint8_t auth_code[ID2_AUTH_CODE_MAX];
    uint32_t auth_code_len = sizeof(auth_code);
    uint32_t base64_auth_code_len = 0;

    if (!cmd_buffer || !out_buffer || !is_id2_prov) {
        return DAS_ERROR_BAD_PARAMETERS;
    }

    out_buffer[0] = '\0';

    result_pair = das_strsplit(cmd_buffer, delim_comma, &nb_pair);
    if (result_pair) {
        for (i = 0; i < nb_pair; i++) {
            DAS_LOG("update data : %s\n", result_pair[i]);

            if (strncmp(result_pair[i], UPT_CMD_KEY_TOKEN, strlen(UPT_CMD_KEY_TOKEN)) == 0) {
                char * ptr = result_pair[i] + strlen(UPT_CMD_KEY_TOKEN);

                if (out_buffer_size < strlen(MES_SESS_KEY_AUTH_CODE) + 4 * ((ID2_AUTH_CODE_MAX + 2) / 3) + 1) {
                    DAS_LOG("input buffer to small, buffer size = %d\n", out_buffer_size);
                    res = DAS_ERROR_SHORT_BUFFER;
                    break;
                }

                id2_res = lite_id2_client_get_otp_auth_code((uint8_t*)ptr, strlen(ptr),
                          auth_code, &auth_code_len);

                if (id2_res != IROT_SUCCESS) {
                    DAS_LOG("lite_id2_client_get_otp_auth_code = %d\n", id2_res);
                    out_buffer[0] = '\0';
                    res = DAS_ERROR_ID2_INTERNAL;
                } else {
                    base64_auth_code_len = out_buffer_size - strlen(MES_SESS_KEY_AUTH_CODE);
                    if (id2_plat_base64_encode(auth_code, auth_code_len,
                                               (uint8_t*)out_buffer + strlen(MES_SESS_KEY_AUTH_CODE), &base64_auth_code_len) == 0) {
                        memcpy(out_buffer, MES_SESS_KEY_AUTH_CODE, strlen(MES_SESS_KEY_AUTH_CODE));
                        DAS_LOG("encoded auth code : %s\n", out_buffer);
                        *is_id2_prov = true;
                    } else {
                        DAS_LOG("id2_plat_base64_encode error\n");
                        out_buffer[0] = '\0';
                        res = DAS_ERROR_ID2_INTERNAL;
                    }
                }
            } else if (strncmp(result_pair[i], UPT_CMD_KEY_OTP, strlen(UPT_CMD_KEY_OTP)) == 0) {
                uint8_t * raw_otp = (uint8_t*) result_pair[i] + strlen(UPT_CMD_KEY_OTP);
                uint32_t raw_otp_len = strlen(result_pair[i] + strlen(UPT_CMD_KEY_OTP));
                uint32_t otp_data_len = 0;
                uint8_t * otp_data = (uint8_t*) malloc(sizeof(uint8_t) * raw_otp_len);

                if (otp_data) {
                    DAS_LOG("encoded otp data : %s\n", raw_otp);
                    if (id2_plat_base64_decode(raw_otp, raw_otp_len, otp_data, &otp_data_len) == 0) {
                        DAS_LOG("decoded otp data len : %d\n", otp_data_len);
                        id2_res = lite_id2_client_load_otp_data(otp_data, otp_data_len);
                        if (id2_res != IROT_SUCCESS) {
                            DAS_LOG("lite_id2_client_load_otp_data = %d\n", id2_res);
                            res = DAS_ERROR_ID2_INTERNAL;
                        }
                    } else {
                        DAS_LOG("id2_plat_base64_decode error\n");
                        res = DAS_ERROR_ID2_INTERNAL;
                    }
                    free(otp_data);
                } else {
                    DAS_LOG("out-of-memory");
                    res = DAS_ERROR_OUT_OF_MEMORY;
                }
            } else {
                DAS_LOG("unknown update data error\n");
                res = DAS_ERROR_NOT_SUPPORTED;
            }

        }
        free(result_pair);
    } else {
        DAS_LOG("out-of-mem error");
        res = DAS_ERROR_OUT_OF_MEMORY;
    }

    return res;
}
#endif

bool _proto_write_string(das_pb_ostream_t *stream, const das_pb_field_t *field, void * const *arg) {
    char *str = *arg;

    if (!das_pb_encode_tag_for_field(stream, field))
        return false;

    return das_pb_encode_string(stream, (uint8_t*)str, strlen(str));
}

bool _proto_read_string(das_pb_istream_t *stream, const das_pb_field_t *field, void **arg) {
    //    uint8_t buffer[MAX_COMMON_READ_BUFFER_SIZE];
    int len = stream->bytes_left;
    uint8_t *buffer = *arg;

    //    if (len > sizeof(buffer) - 1) {
    //        DAS_LOG("MAX_COMMON_READ_BUFFER_SIZE too small\n");
    //        return false;
    //    }

    //    buffer[len] = '\0';

    if (!das_pb_read(stream, buffer, len))
        return false;

    /* Print the string, in format comparable with protoc --decode.
     * Format comes from the arg defined in main().
     */
    //    strncpy(tmp, buffer, MAX_COMMON_READ_BUFFER_SIZE);

    return true;
}

/*
 * https://github.com/nanopb/nanopb/blob/master/tests/oneof_callback/decode_oneof.c
 */
/* The callback below is a message-level callback which is called before each
 * submessage is encoded. It is used to set the das_pb_callback_t callbacks inside
 * the submessage. The reason we need this is that different submessages share
 * storage inside oneof union, and before we know the message type we can't set
 * the callbacks without overwriting each other.
 */

bool _proto_decode_message_data(das_pb_istream_t *stream, const das_pb_field_t *field, void **arg) {
    /* Print the prefix field before the submessages.
     * This also demonstrates how to access the top level message fields
     * from callbacks.
     */
    //    Message *top_msg = field->message;
    //    DAS_LOG("prefix: %d\n", (int)topmsg->prefix);

    das_session_t *ss = *arg;

    if (field->tag == Message_atsCmdData_tag) {

        AtsCmdData *ats_cmd_data = field->pData;
        DAS_LOG("parse ats cmd with type = %d\n", (int) ats_cmd_data->type);

        ats_cmd_data->patternUrl.funcs.decode = _proto_read_string;
        ats_cmd_data->patternUrl.arg = ss->pbb.ats_cmd_ptn_url;
        ats_cmd_data->pattern.funcs.decode = _proto_read_string;
        ats_cmd_data->pattern.arg = ss->pbb.ats_cmd_ptn;

    } else if (field->tag == Message_mesCmdData_tag) {

        MesCmdData *mes_cmd_data = field->pData;
        DAS_LOG("parse mes cmd with type = %d\n", (int) mes_cmd_data->type);

    } else if (field->tag == Message_uptCmdData_tag) {

        UptCmdData *upt_cmd_data = field->pData;
        DAS_LOG("parse upt cmd with type = %d\n", (int) upt_cmd_data->type);

        upt_cmd_data->url.funcs.decode = _proto_read_string;
        upt_cmd_data->url.arg = ss->pbb.upt_cmd_url;
        upt_cmd_data->object.funcs.decode = _proto_read_string;
        upt_cmd_data->object.arg = ss->pbb.upt_cmd_obj;

    } else {
        DAS_LOG("cannot parse the submessage error, tag = %d\n", (int)field->tag);
        return false;
    }

    /* Once we return true, das_pb_dec_submessage() will go on to decode the
     * submessage contents. But if we want, we can also decode it ourselves
     * above and leave stream->bytes_left at 0 value, inhibiting automatic
     * decoding.
     */
    return true;
}

#if defined DAS2_FEATURE_ATTESTATION
static das_result_t _do_attest_dev(const char * cmd_buffer,
                                   char * out_buffer,
                                   int out_buffer_size) {
    das_result_t res = DAS_SUCCESS;
    lite_irot_result_t id2_res = IROT_SUCCESS;
    uint32_t auth_code_len, id_len;
    int offset = 0;
    char * challenge = NULL;
    char delim_colon[] = ":";

    if (!cmd_buffer || !out_buffer) {
        return DAS_ERROR_BAD_PARAMETERS;
    }

    // buffer size check -> auth_code:xxx,id:xxx
    if ((out_buffer_size - 1) < (strlen(ATS_DEV_KEY_AUTH_CODE) +
                                 ID2_CHALLANGE_AUTH_CODE_MAX + 1 + strlen(ATS_DEV_KEY_ID2_ID) + ID2_ID_LEN)) {
        DAS_LOG("buffer not enough");
        return DAS_ERROR_SHORT_BUFFER;
    }

    DAS_LOG("command : %s\n", cmd_buffer);

    challenge = strstr(cmd_buffer, delim_colon);
    if (!challenge) {
        DAS_LOG("challenge parse error");
        return DAS_ERROR_SOC_PROTO_ERROR;
    }

    challenge += 1;
    DAS_LOG("challenge : %s\n", challenge);

    offset = strlen(ATS_DEV_KEY_AUTH_CODE);
    memcpy(out_buffer, ATS_DEV_KEY_AUTH_CODE, offset);

    auth_code_len = out_buffer_size - offset - 1;// buffer length when used as input
    id2_res = lite_id2_client_get_challenge_auth_code(challenge,
              NULL, 0,
              (uint8_t*)(out_buffer + offset),
              &auth_code_len);

    if (id2_res != IROT_SUCCESS) {
        DAS_LOG("lite_id2_client_get_challenge_auth_code = %d\n", id2_res);
        out_buffer[0] = '\0';
        res = DAS_ERROR_ID2_INTERNAL;
    } else {
        offset += auth_code_len;

        out_buffer[offset] = DAS_KEY_SEPARATOR;
        offset++;

        memcpy(out_buffer + offset, ATS_DEV_KEY_ID2_ID, strlen(ATS_DEV_KEY_ID2_ID));
        offset += strlen(ATS_DEV_KEY_ID2_ID);

        id_len = out_buffer_size - offset - 1;// buffer length when used as input
        if (IROT_SUCCESS == lite_id2_client_get_id((uint8_t*)(out_buffer + offset), &id_len)) {
            offset += id_len;
            out_buffer[offset] = '\0';
        } else {
            DAS_LOG("cannot get id2 id");
            out_buffer[offset] = '\0';
            res = DAS_ERROR_ID2_INTERNAL;
        }
    }

    return res;
}
#endif

static void _reset_protocol_buffer(das_session_t *session) {
    session->pbb.ats_cmd_ptn_url[0] = '\0';
    session->pbb.ats_cmd_ptn[0] = '\0';

    session->pbb.upt_cmd_url[0] = '\0';
    session->pbb.upt_cmd_obj[0] = '\0';

    session->pbb.inf_ver[0] = '\0';
    session->pbb.inf_dev[0] = '\0';
    session->pbb.inf_state[0] = '\0';

    session->pbb.ats_fil_path[0] = '\0';
    session->pbb.ats_fil_attr[0] = '\0';
    session->pbb.ats_fil_sig[0] = '\0';

    session->pbb.ats_dev_obj[0] = '\0';

    session->pbb.ats_end_sum[0] = '\0';

    session->pbb.mes_fil_sum[0] = '\0';
    session->pbb.mes_fil_mac[0] = '\0';

    session->pbb.msg_session[0] = '\0';
}

static int _decode_message(das_session_t *session, const uint8_t *message, size_t size) {
    bool status;
    das_pb_istream_t stream;
    Message cmd = Message_init_zero;

    if (session == NULL || message == NULL || size == 0) {
        DAS_LOG("invalid input args\n");
        return DAS_ERROR_BAD_PARAMETERS;
    }

    /* reset protocol buffer before decoding message */
    _reset_protocol_buffer(session);

    cmd.cb_data.funcs.decode = _proto_decode_message_data;
    cmd.cb_data.arg = session;
    stream = das_pb_istream_from_buffer(message, size);

    status = das_pb_decode(&stream, Message_fields, &cmd);
    if (!status) {
        DAS_LOG("decode failed: %s\n", PB_GET_ERROR(&stream));
        return DAS_ERROR_GENERIC;
    }

    if (cmd.which_data == Message_atsCmdData_tag) {
        if (AtsCmdData_Type_FULL == cmd.data.atsCmdData.type) {
            DAS_LOG_ATS_CMD(session, cmd);

            session->cmd = DAS_CMD_ATTEST_FULL;
            session->cmd_id = cmd.timestamp;

        } else if (AtsCmdData_Type_DEV == cmd.data.atsCmdData.type) {
            DAS_LOG_ATS_CMD(session, cmd);

            session->cmd = DAS_CMD_ATTEST_DEV;
            session->cmd_id = cmd.timestamp;

            // put update data to stepping buffer
            strncpy((char*)session->cmd_buffer,
                    session->pbb.ats_cmd_ptn,
                    DAS_CMD_BUFFER_LENGTH - 1);
            ((char*)session->cmd_buffer)[DAS_CMD_BUFFER_LENGTH - 1] = '\0';

        } else {
            DAS_LOG("not support ats cmd type: %d\n", cmd.data.atsCmdData.type);
            return DAS_ERROR_NOT_SUPPORTED;
        }

    } else if (cmd.which_data == Message_mesCmdData_tag) {
        if (MesCmdData_Type_FULL != cmd.data.mesCmdData.type) {
            DAS_LOG("not support mes cmd type: %d\n", cmd.data.mesCmdData.type);
            return DAS_ERROR_NOT_SUPPORTED;
        }

        DAS_LOG_MES_CMD(session, cmd);

        session->cmd = DAS_CMD_MEASURE;
        session->cmd_id = cmd.timestamp;

    } else if (cmd.which_data == Message_uptCmdData_tag) {
        if (UptCmdData_Type_SESSION != cmd.data.uptCmdData.type) {
            DAS_LOG("not support upt cmd type: %d\n", cmd.data.uptCmdData.type);
            return DAS_ERROR_NOT_SUPPORTED;
        }

        DAS_LOG_UPT_CMD(session, cmd);

        session->cmd = DAS_CMD_UPDATE;
        session->cmd_id = cmd.timestamp;

        // put update data to stepping buffer
        strncpy((char*)session->cmd_buffer,
                session->pbb.upt_cmd_obj,
                DAS_CMD_BUFFER_LENGTH - 1);
        ((char*)session->cmd_buffer)[DAS_CMD_BUFFER_LENGTH - 1] = '\0';

    } else {
        DAS_LOG("unknown command: %d\n", cmd.which_data);
    }

    return DAS_SUCCESS;
}

void das_on_message(void *session, const uint8_t *message, size_t size) {
    das_session_t *ss = (das_session_t *)session;

    if (session == NULL || ss->magic != DAS_SESSION_MAGIC) {
        DAS_LOG("Invalid session 0x%lx\n", (long)ss);
        return;
    }

    if (message == NULL || size == 0) {
        DAS_LOG("Invalid message 0x%lx\n", (long)message);
        return;
    }

    if (ss->cmd != DAS_CMD_NONE) {
        DAS_LOG("DAS command (%d) is in processing, so a incoming command dropped.\n", ss->cmd);
        return;
    }

    _decode_message(ss, message, size);
}

static size_t _encode_message(das_session_t *session, Message *message) {
    bool status;
    size_t size;
    das_pb_ostream_t stream;

    status = das_pb_get_encoded_size(&size, Message_fields, message);
    if (!status) {
        DAS_LOG("get encoded size failed\n");
        return 0;
    }

    /* get buf size only */
    if (message == NULL) {
        return size;
    }

    if (size > DAS_BUFFER_LENGTH) {
        DAS_LOG("short buffer, %lx\n", (long)size);
        return 0;
    }

    stream = das_pb_ostream_from_buffer(session->stepping_buffer, size);

    status = das_pb_encode(&stream, Message_fields, message);
    if (!status) {
        DAS_LOG("encoding failed: %s\n", PB_GET_ERROR(&stream));
        return 0;
    }

    return size;
}

static void _publish_message(das_session_t *session) {
    size_t size;

    if (session->connected) {
        size = _encode_message(session, &session->report);
        if (size > 0) {
            session->publish_handle(session->pub_topic,
                                    session->stepping_buffer, size, session->channel);
        }
    }

    /* reset protocol buffer after publish message */
    _reset_protocol_buffer(session);
}

static void _fill_session(char * buffer, int size) {

#if (DAS_SERVICE_ID2_ID)
    uint32_t id_buf_len = 0;
    int id_tag_len = strlen(MES_SESS_KEY_ID);

    if (!buffer) {
        DAS_LOG("buffer null\n");
        return;
    }

    if (size < id_tag_len + ID2_ID_LEN + 1) {
        DAS_LOG("buffer too small\n");
        return;
    }

    // if we cannot get id2 id, let server know
    id_buf_len = size - id_tag_len - 1;
    if (IROT_SUCCESS == lite_id2_client_get_id((uint8_t*)(buffer + id_tag_len), &id_buf_len)) {
        memcpy(buffer, MES_SESS_KEY_ID, id_tag_len);
        //memcpy(buffer + id_tag_len, id, id_buf_len);
        buffer[id_tag_len + id_buf_len] = '\0';
    } else {
        DAS_LOG("cannot get id2 id\n");
        buffer[0] = '\0';
    }
#endif
}

static void _publish_info(das_session_t *session, bool final) {
    Message *report = &(session->report);
    InfData *info = &(report->data.infData);
    char *offset;
    int length;
    size_t device_id_len = 0;
    char *fversion = NULL;

    if (true == final) {
        return;
    }

    /********************************************
     * Setup encode buffer
     ********************************************/
    report->session.funcs.encode = _proto_write_string;
    report->session.arg = session->pbb.msg_session;

    info->version.funcs.encode = _proto_write_string;
    info->version.arg = session->pbb.inf_ver;
    info->device.funcs.encode = _proto_write_string;
    info->device.arg = session->pbb.inf_dev;
    info->state.funcs.encode = _proto_write_string;
    info->state.arg = session->pbb.inf_state;
    /********************************************/

    report->domain = Domain_SYS;
    report->code = Code_INF;
    report->timestamp = session->stepping_now;
    report->which_data = Message_infData_tag;

    if (!session->is_id2_prov) {
        _fill_session(session->pbb.msg_session, sizeof(session->pbb.msg_session));
    }

    /* das version */
    offset = session->pbb.inf_ver;
    length = strlen(DAS_VERSION);
    memcpy(offset, DAS_VERSION, length);
    offset += length;
    *offset = DAS_KEY_SEPARATOR;
    offset++;

    /* profile */
    length = strlen(DAS_PROFILE);
    memcpy(offset, DAS_PROFILE, length);
    offset += length;
    *offset = DAS_KEY_SEPARATOR;
    offset++;

    /* firmware version */
    length = strlen(DAS_FIRMWARE);
    memcpy(offset, DAS_FIRMWARE, length);
    offset += length;

    length = sizeof(session->pbb.inf_ver) - (offset - session->pbb.inf_ver);
    fversion = strlen(g_fw_ver) > 0 ? g_fw_ver : DEFAULT_FIRMWARE_VERSION;
    length = length > strlen(fversion) ? strlen(fversion) : length - 1;
    memcpy(offset, fversion, length);
    /* truncate overflow */
    offset += length;
    *offset = '\0';

    /* device & device.arch */
    offset = session->pbb.inf_dev;
    length = strlen(DAS_PLATFORM);
    memcpy(offset, DAS_PLATFORM, length);
    offset += length;

    /* device.id */
    length = 0;
    device_id_len = strlen(g_dev_id);
    if (device_id_len) {
        *offset = DAS_KEY_SEPARATOR;
        offset++;

        length = strlen(DAS_KEY_DEVICE_ID);
        memcpy(offset, DAS_KEY_DEVICE_ID, length);
        offset += length;

        length = sizeof(session->pbb.inf_dev) - (offset - session->pbb.inf_dev);
        length = length > device_id_len ? device_id_len : length - 1;
        memcpy(offset, g_dev_id, length);
    }

    /* truncate overflow */
    offset += length;
    *offset = '\0';

    DAS_LOG_INFO(session, report);

    _publish_message(session);
}

#if defined DAS2_FEATURE_ATTESTATION
static void _publish_attest(das_session_t *session, bool final) {
    uint8_t hash[DAS_SHA1_LENGTH];
    Message *report = &(session->report);
    AtsFilData *fdata = &(report->data.atsFilData);
    AtsEndData *edata = &(report->data.atsEndData);
    das_sum_context_t *sum_ctx = (das_sum_context_t *)session->stepping_buffer;

    /********************************************
     * Setup encode buffer
     ********************************************/
    report->session.funcs.encode = _proto_write_string;
    report->session.arg = session->pbb.msg_session;
    /********************************************/

    report->domain = Domain_SYS;
    report->code = Code_ATS;
    report->timestamp = session->stepping_now;

    _fill_session(session->pbb.msg_session, sizeof(session->pbb.msg_session));

    /* recycle sum context if final */
    das_sum_finish(sum_ctx, hash);

    if (false == final) {
        report->which_data = Message_atsFilData_tag;
        fdata->cmd = session->cmd_id;

        /********************************************
         * Setup encode buffer for Ats Fil
         ********************************************/
        fdata->path.funcs.encode = _proto_write_string;
        fdata->path.arg = session->pbb.ats_fil_path;
        fdata->attribute.funcs.encode = _proto_write_string;
        fdata->attribute.arg = session->pbb.ats_fil_attr;
        fdata->signature.funcs.encode = _proto_write_string;
        fdata->signature.arg = session->pbb.ats_fil_sig;

        das_hex2string(session->pbb.ats_fil_sig, sizeof(session->pbb.ats_fil_sig), hash, DAS_SHA1_LENGTH);

        das_sum_update(&session->stepping_sum_context, hash, DAS_SHA1_LENGTH);
        session->report_number++;

        DAS_LOG_ATS_FIL(session, report);

        _publish_message(session);

        /* re-start sum context */
        das_sum_start(sum_ctx);
    } else {
        report->which_data = Message_atsEndData_tag;
        edata->cmd = session->cmd_id;
        edata->total = session->report_number;

        /********************************************
         * Setup encode buffer for Ats End
         ********************************************/
        edata->sum.funcs.encode = _proto_write_string;
        edata->sum.arg = session->pbb.ats_end_sum;

        das_sum_finish(&session->stepping_sum_context, hash);

        memcpy(session->pbb.ats_end_sum, DAS_FIL_TAG, strlen(DAS_FIL_TAG));
        das_hex2string((char*)session->pbb.ats_end_sum + strlen(DAS_FIL_TAG),
                       sizeof(session->pbb.ats_end_sum) - strlen(DAS_FIL_TAG), hash, DAS_SHA1_LENGTH);

        DAS_LOG_ATS_END(session, report);

        _publish_message(session);

        session->cmd = DAS_CMD_NONE;
    }
}

static void _publish_measure(das_session_t *session, bool final) {
    uint8_t hash[DAS_SHA1_LENGTH];
    Message *report = &(session->report);
    MesFilData *fdata = &(report->data.mesFilData);
    MesEndData *edata = &(report->data.mesEndData);
    das_sum_context_t *sum_ctx = (das_sum_context_t *)session->stepping_buffer;

    /********************************************
     * Setup encode buffer
     ********************************************/
    report->session.funcs.encode = _proto_write_string;
    report->session.arg = session->pbb.msg_session;
    /********************************************/

    report->domain = Domain_SYS;
    report->code = Code_MES;
    report->timestamp = session->stepping_now;

    _fill_session(session->pbb.msg_session, sizeof(session->pbb.msg_session));

    /* recycle sum context if final */
    das_sum_finish(sum_ctx, hash);

    if (false == final) {
        das_sum_update(&session->stepping_sum_context, hash, DAS_SHA1_LENGTH);

        /* re-start sum context */
        das_sum_start(sum_ctx);
    } else {
        report->which_data = Message_mesFilData_tag;
        fdata->cmd = session->cmd_id;

        /********************************************
         * Setup encode buffer for Mes Fil
         ********************************************/
        fdata->sum.funcs.encode = _proto_write_string;
        fdata->sum.arg = session->pbb.mes_fil_sum;
        fdata->mac.funcs.encode = _proto_write_string;
        fdata->mac.arg = session->pbb.mes_fil_mac;

        das_sum_finish(&session->stepping_sum_context, hash);

        das_hex2string(session->pbb.mes_fil_sum, sizeof(session->pbb.mes_fil_sum), hash, DAS_SHA1_LENGTH);

        das_mac_finish(&session->stepping_mac_context, hash);

        das_hex2string(session->pbb.mes_fil_mac, sizeof(session->pbb.mes_fil_mac), hash, DAS_SHA1_LENGTH);

        DAS_LOG_MES_FIL(session, report);
        _publish_message(session);

        report->which_data = Message_mesEndData_tag;
        edata->cmd = session->cmd_id;
        edata->total = 1;

        DAS_LOG_MES_END(session, report);

        _publish_message(session);

        session->cmd = DAS_CMD_NONE;
    }
}

static void _publish_ats_dev(das_session_t *session, bool final) {
    Message *report = &(session->report);
    AtsDevData *dev = &(report->data.atsDevData);

    /********************************************
     * Setup encode buffer
     ********************************************/
    report->session.funcs.encode = _proto_write_string;
    report->session.arg = session->pbb.msg_session;

    dev->object.funcs.encode = _proto_write_string;
    dev->object.arg = session->pbb.ats_dev_obj;
    /********************************************/

    report->domain = Domain_SYS;
    report->code = Code_ATS;
    report->timestamp = session->stepping_now;
    report->which_data = Message_atsDevData_tag;

    _fill_session(session->pbb.msg_session, sizeof(session->pbb.msg_session));

    /* cmd timestamp */
    dev->cmd = session->cmd_id;

    DAS_LOG_ATS_DEV(session, report);

    _publish_message(session);

    session->cmd = DAS_CMD_NONE;
}

#if (DAS_AUTO_MEASURE)
static bool _das_measured = false;
void _das_auto_measure(das_session_t *session, uint64_t now) {
    if (session == NULL || now == 0) {
        return;
    }

    if (_das_measured == false) {
        session->cmd = DAS_CMD_MEASURE;
        session->cmd_id = now;
        _das_measured = true;
    }
}
#endif

static das_result_t _process_update_cmd(das_session_t *ss) {
    das_result_t ret = DAS_SUCCESS;

    if (!ss) {
        DAS_LOG("null param");
        ret = DAS_ERROR_GENERIC;
    } else {
        ss->is_id2_prov = false;

        ret = _do_update_cmd(ss->cmd_buffer, // paramters for update
                             (char*)(ss->pbb.msg_session),
                             sizeof(ss->pbb.msg_session),
                             &ss->is_id2_prov);

        if (ret != DAS_SUCCESS) {
            DAS_LOG("service update error, err code: %d\n", ret);
        } else {
            // TODO: put state assignment to _publish_info because we want it to be determined
            // after which_data is set.
            ss->pbb.inf_state[0] = '\0';
            _publish_info(ss, false);
        }

        ss->is_id2_prov = false;
    }

    ss->cmd = DAS_CMD_NONE;

    return ret;
}

static das_result_t _process_attest_dev(das_session_t *ss) {
    das_result_t ret = DAS_SUCCESS;

    ret = _do_attest_dev(ss->cmd_buffer, // paramters for ats dev
                         (char*)(ss->pbb.ats_dev_obj),
                         sizeof(ss->pbb.ats_dev_obj));
    if (ret != DAS_SUCCESS) {
        DAS_LOG("service attest dev error, err code: %d\n", ret);
    } else {
        _publish_ats_dev(ss, false);
    }

    ss->cmd = DAS_CMD_NONE;

    return ret;
}

#endif /* DAS2_FEATURE_ATTESTATION */

/*
 * A exception case to drop message if we must
 */
static bool dropping_policy(const das_session_t * ss) {
    if (!ss) {
        return true;
    }

    /* Rule#1 : For first info stage, we only report heartbeat */
    if (_g_is_first_info) {
        if (ss->stepping_service &&
                ss->stepping_service->name &&
                strncmp(ss->stepping_service->name, SERVICE_NAME_SYS, strlen(SERVICE_NAME_SYS) + 1) != 0) {
            DAS_LOG("Now is the first info stage, drop %s info\n", ss->stepping_service->name);
            return true;
        }
    }

    return false;
}

das_result_t das_stepping(void *session, uint64_t now) {
    das_result_t ret = DAS_SUCCESS;
    bool is_stepping = false;
    das_session_t *ss = (das_session_t *)session;
    uint8_t hash[DAS_SHA1_LENGTH] = {0};
    das_sum_context_t *sum_ctx = (das_sum_context_t *)ss->stepping_buffer;

    if (ss == NULL || ss->magic != DAS_SESSION_MAGIC) {
        DAS_LOG("invalid session, 0x%lx\n", (long)ss);
        return DAS_ERROR_BAD_PARAMETERS;
    }

    ss->stepping_now = now;

    if (ss->step == DAS_STEP_IDLE) {
        if (ss->cmd != DAS_CMD_NONE) {
            ss->report_number = 0;
#if defined DAS2_FEATURE_ATTESTATION
            if (ss->cmd == DAS_CMD_ATTEST_FULL) {
                das_sum_start(sum_ctx);
                das_sum_start(&ss->stepping_sum_context);
                ss->step = DAS_STEP_ATTEST_FULL;
            } else if (ss->cmd == DAS_CMD_ATTEST_DEV) {
                _process_attest_dev(ss);
            } else if (ss->cmd == DAS_CMD_MEASURE) {
                das_sum_start(sum_ctx);
                das_sum_start(&ss->stepping_sum_context);

                das_mac_start(&ss->stepping_mac_context);
                das_mac_update(&ss->stepping_mac_context, (uint8_t *)&ss->cmd_id, sizeof(uint64_t));
                ss->step = DAS_STEP_MEASURE;
            } else if (ss->cmd == DAS_CMD_UPDATE) {
                _process_update_cmd(ss);
            } else {
                DAS_LOG("unknown cmd, keep idle");
            }
#else
            DAS_LOG("unknown cmd, keep idle");
#endif
        } else {
            _g_is_first_info = (ss->info_time == 0) ? true : false;
            if (ss->info_time == 0 || now - ss->info_time > DAS_HEARTBEAT_PERIOD) {
                ss->info_time = now;
                ss->step = DAS_STEP_INFO;
            } else {
#if (DAS_AUTO_MEASURE)
                _das_auto_measure(ss, now);
#endif
            }
        }

        ss->service_index = 0;
        ss->stepping_service = das_service_table[0];
        ss->service_state.status = DAS_SRV_STATUS_START;
    }

    if (ss->step != DAS_STEP_IDLE) {
        das_service_state_t *service_state = &ss->service_state;
        void (*publish_handle)(das_session_t *, bool) = NULL;

        memset(&ss->report, 0, sizeof(Message));

        switch (ss->step) {
            case DAS_STEP_INFO: {
                publish_handle = _publish_info;
                if (ss->stepping_service->info) {
                    ret = ss->stepping_service->info(
                              (char*) & (ss->pbb.inf_state),
                              sizeof(ss->pbb.inf_state), service_state);

                    if (ret == DAS_ERROR_GENERIC && strlen((char*) & (ss->pbb.inf_state)) == 0) {
                        ret = DAS_SUCCESS;
                        is_stepping = true;
                    }

                    if (ret != DAS_SUCCESS) {
                        DAS_LOG("service %s info error, err code: %d\n", ss->stepping_service->name, ret);
                        goto _out;
                    }
                } else {
                    is_stepping = true;
                }
                break;
            }
#if defined DAS2_FEATURE_ATTESTATION
            case DAS_STEP_ATTEST_FULL: {
                publish_handle = _publish_attest;
                if (ss->stepping_service->attest) {
                    ret = ss->stepping_service->attest(
                              (char*) & (ss->pbb.ats_fil_path),
                              sizeof(ss->pbb.ats_fil_path),
                              (char*) & (ss->pbb.ats_fil_attr),
                              sizeof(ss->pbb.ats_fil_attr),
                              sum_ctx,
                              service_state
                          );
                    if (ret != DAS_SUCCESS) {
                        DAS_LOG("service %s attest error, err code: %d\n", ss->stepping_service->name, ret);
                        goto _out;
                    }
                } else {
                    is_stepping = true;
                }
                break;
            }
            case DAS_STEP_MEASURE: {
                publish_handle = _publish_measure;
                if (ss->stepping_service->measure) {
                    ret = ss->stepping_service->measure(sum_ctx, &ss->stepping_mac_context, service_state);
                    if (ret != DAS_SUCCESS) {
                        DAS_LOG("service %s measure error, err code: %d\n", ss->stepping_service->name, ret);
                        goto _out;
                    }
                } else {
                    is_stepping = true;
                }
                break;
            }
#endif
            default:
                DAS_LOG("invalid das step status, %d\n", ss->step);
                ret = DAS_ERROR_GENERIC;
                goto _out;
        }

        if (is_stepping) {
            service_state->status = DAS_SRV_STATUS_START;
            ss->service_index++;
        } else {
            if (service_state->status == DAS_SRV_STATUS_PUBLISH) {
                if (!dropping_policy(ss)) {
                    publish_handle(ss, false);
                }
            } else if (service_state->status == DAS_SRV_STATUS_FINISH) {
                if (!dropping_policy(ss)) {
                    publish_handle(ss, false);
                }
                service_state->status = DAS_SRV_STATUS_START;
                ss->service_index++;
            }
        }

        ss->stepping_service = das_service_table[ss->service_index];
        if (ss->stepping_service == NULL) {
            if (!dropping_policy(ss)) {
                publish_handle(ss, true);
            }
            ss->step = DAS_STEP_IDLE;
        }
    }

_out:
    // TODO : not all failure goes here
    if (ret != DAS_SUCCESS) {
        das_sum_finish(sum_ctx, hash);
        das_sum_finish(&ss->stepping_sum_context, hash);

        if (ss->step == DAS_STEP_MEASURE) {
            das_mac_finish(&ss->stepping_mac_context, hash);
        }

        ss->step = DAS_STEP_IDLE;
    }

    return ret;
}

int das_set_firmware_version(char *ver) {
    int ret = -1;
    size_t len = 0;

    if (ver == NULL || (len = strlen(ver)) == 0) {
        return ret;
    }

    len = len < DEVICE_ID_MAX_LEN ? len : DEVICE_ID_MAX_LEN - 1;
    memcpy(g_fw_ver, ver, len);
    g_fw_ver[len] = '\0';

    ret = 0;
    return ret;
}
