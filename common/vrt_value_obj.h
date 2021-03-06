// done
//
#ifndef __VRT_VALUE_OBJ_H_
#define __VRT_VALUE_OBJ_H_
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
//#include "quote_datatype_shfe_deep.h"

#define gettid() syscall(__NR_gettid)

/*
 *  如下定义使用TCP还是UDP发送大连行情。
 *  网络好，可以使用UDP
 *  网络不好，使用UDP
 */
 #define DCE_UDP_SEND_DATA
//#define DCE_TCP_SEND_DATA


// 如果要支持INE的行情，需要定义INE_ENABLE宏
#define INE_ENABLE

/*
 * 如下宏根据情况进行定义
*/
// 行情持久化开关
//  #define PERSISTENCE_ENABLED
//#define LATENCY_MEASURE


// software license
#define SERVER_NAME  "zjtest1"

// 合规检查开关
#define COMPLIANCE_CHECK

#define POSITION_FILE "total.pos"

#define CLOG_CHANNEL  "x-trader"
// 通过合约查找订阅该合约行情的方法:

// 1: unordered_multimap  
// 2: two-dimensional array
// 3: strcmp
#define FIND_STRATEGIES 3

// 一个trader支持最多信号数 
#define COUNTER_UPPER_LIMIT 30000
#define INVALID_PENDING_SIGNAL 999999
// 满足一天足够的下单量，以空间换时间
#define RPT_BUFFER_SIZE 30000


#ifdef PERSISTENCE_ENABLED 
	#define MAX_DOMINANT_CONTRACT_COUNT 5120
#else
	#define MAX_DOMINANT_CONTRACT_COUNT 1024
#endif


#define UDP_RCV_BUF_SIZE		212992

// 单个策略最大支持的合约数
#define MAX_CONTRACT_COUNT_FOR_STRATEGY 200


#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */
	#include <inttypes.h>		 
	#include <bowsprit.h>
	#include <libcork/core.h>
	#include <libcork/ds.h>
	#include <libcork/helpers/errors.h>
	#include <clogger.h>
	#include <vrt.h>

#ifdef __cplusplus
	};
#endif /* __cplusplus */

	/* --------------------------------------------------------------
	 * x-trader varon-t value and type
	 */


class Log
{
	public:
		static FILE *fp;/*文件指针*/
};

	enum HybridData {
		DCE_YAO_DATA = 0, 
		ZCE_YAO_DATA,
		EFH_LEV2,
		SHFE_L1_MD,
		INE_L1_MD,
		ZCE_L1_MD,
		ZCE_L2_MD, 
		TUNN_RPT,
	};

	struct vrt_hybrid_value {
		struct vrt_value  parent;
		enum HybridData data;
		int32_t index;
	};

	static struct vrt_value *
	vrt_hybrid_value_new(struct vrt_value_type *type)
	{
		struct vrt_hybrid_value *self = (struct vrt_hybrid_value*)cork_new(struct vrt_hybrid_value);
		return &self->parent;
	}

	static void
	vrt_hybrid_value_free(struct vrt_value_type *type, struct vrt_value *vself)
	{
		struct vrt_hybrid_value *iself =
			cork_container_of(vself, struct vrt_hybrid_value, parent);
		free(iself);
	}

	static struct vrt_value_type  _vrt_hybrid_value_type = {
		vrt_hybrid_value_new,
		vrt_hybrid_value_free
	};

	static struct vrt_value_type *
	vrt_hybrid_value_type(void)
	{
		return &_vrt_hybrid_value_type;
	}



#endif
