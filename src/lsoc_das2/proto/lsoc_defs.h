/* Configurable proto buffer size, 20201222 Maqiu */

#ifndef LSOC_DEFS_H
#define LSOC_DEFS_H

/* SOC command buffer length */
#define DAS_CMD_BUFFER_LENGTH          (256)

// must bigger than all char[] in protocol messages
//#define MAX_COMMON_READ_BUFFER_SIZE    (1024)

#define MAX_INF_VERSION_SIZE           (96)
#define MAX_INF_DEVICE_SIZE            (96)

#ifdef DAS_PLATFORM_LINUX
#define MAX_INF_STATE_SIZE             (1024)
#else
#define MAX_INF_STATE_SIZE             (96)
#endif

#define MAX_ATS_CMD_PTN_URL_SIZE       (1)
#define MAX_ATS_CMD_PTN_SIZE           (256)

#ifdef DAS_PLATFORM_LINUX
#define MAX_ATS_FIL_PATH_SIZE          (1024)
#else
#define MAX_ATS_FIL_PATH_SIZE          (96)
#endif

#define MAX_ATS_FIL_ATTR_SIZE          (96)
#define MAX_ATS_FIL_SIG_SIZE           (96)

#define MAX_ATS_DEV_OBJ_SIZE           (512)

#define MAX_ATS_END_SUM_SIZE           (96)

#define MAX_MES_FIL_SUM_SIZE           (96)
#define MAX_MES_FIL_MAC_SIZE           (96)

//#define MAX_MES_NET_SUM_SIZE           (96)
//#define MAX_MES_NET_MAC_SIZE           (96)

#define MAX_UPT_CMD_URL_SIZE           (96)
#define MAX_UPT_CMD_OBJ_SIZE           (256)

#define MAX_MSG_SESS_SIZE              (512)


#endif /* LSOC_DEFS_H */


