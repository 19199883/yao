#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <dlfcn.h>
#include <string>
#include <thread>         // std::this_thread::sleep_for
#include <chrono>         // std::chrono::seconds

#include <signal.h>     /* signal */
#include "vrt_value_obj.h"
#include "tunn_rpt_producer.h"
#include "uni_consumer.h"
#include "pos_calcu.h"
#include "ctp_data_formater.h"
#include "efh_lev2_producer.h"
#include "l1md_producer.h"
#include "dce_md_receiver.h"
#include "zce_md_receiver.h"

/* Note that the parameter for queue size is a power of 2. */
#define  QUEUE_SIZE  4096

UniConsumer *uniConsumer = NULL;
EfhLev2Producer* efhLev2Producer = NULL;
L1MDProducer* l1MDProducer = NULL; 
TunnRptProducer *tunnRptProducer = NULL;
DceQuote *dceQuote = NULL;
ZceQuote *zceQuote = NULL;

static void
SIG_handler(int s)
{
	uniConsumer->Stop();
}

int main(/*int argc, const char **argv*/)
{
	struct sigaction SIGINT_act;
	SIGINT_act.sa_handler = SIG_handler;
	sigemptyset(&SIGINT_act.sa_mask);
	SIGINT_act.sa_flags = 0;
	sigaction(SIGUSR2, &SIGINT_act, NULL);

	// clog setting		   CLOG_LEVEL_WARNING
	clog_set_minimum_level(CLOG_LEVEL_INFO);
	FILE *fp;/*文件指针*/
	fp=fopen("./x-trader.log","w+");

	Log::fp = fp;

	struct clog_handler *clog_handler = 
		clog_stream_handler_new_fp(fp, true, "%l %m");
	clog_handler_push_process(clog_handler);

	clog_warning("test..."); 
#ifdef LATENCY_MEASURE
	clog_warning("latency measure on"); 
#else
	clog_warning("latency measure off"); 
#endif

#ifdef COMPLIANCE_CHECK
	clog_warning("COMPLIANCE_CHECK on"); 
#else
	clog_warning("COMPLIANCE_CHECK off"); 
#endif

#ifdef PERSISTENCE_ENABLED
	clog_warning("PERSISTENCE_ENABLED on"); 
#else
	clog_warning("PERSISTENCE_ENABLEDon off"); 
#endif
	// version
	clog_warning("version:ytrader_2020-07-29_r"); 
	
	struct vrt_queue  *queue;
	int64_t  result;

	rip_check(queue = vrt_queue_new("x-trader queue", 
					vrt_hybrid_value_type(), 
					QUEUE_SIZE));

	dceQuote = new DceQuote(queue);
	zceQuote = new ZceQuote(queue);
	efhLev2Producer  = new EfhLev2Producer(queue);
	l1MDProducer  = new L1MDProducer(queue); 
	tunnRptProducer = new TunnRptProducer(queue);

#ifdef PERSISTENCE_ENABLED 
	// 如果进行行情落地，则是行情程序，不进行交易
#else
	while(!tunnRptProducer->IsReady())
	{
		std::this_thread::sleep_for (std::chrono::seconds(1));
	}
#endif

	uniConsumer = new UniConsumer (queue, 
				l1MDProducer,
				efhLev2Producer, 
				dceQuote,
				zceQuote,
				tunnRptProducer);
	uniConsumer->Start();

	clog_warning("main exit."); 
	
	fflush (fp);

  // free vrt_queue
	vrt_queue_free(queue);

	delete tunnRptProducer; 
	delete shfe_full_producer, 
	delete shfe_l1_md_producer ,
	delete dceQuote;
	delete zceQuote;
	delete uniConsumer;

// clog: free resources
	//pos_calc::destroy_instance();
	clog_handler_free(clog_handler);

	return 0;
}
