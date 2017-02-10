#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/sched.h>
#include <linux/types.h>
#include <linux/list.h>
#include <linux/kthread.h>
#include <linux/err.h>
#include <linux/delay.h>

#include "diting_ktask.h"
#include "diting_util.h"
#include "diting_nolockqueue.h"

static int volatile diting_ktask_run_t;
static struct diting_ktask_loop lo[DITING_KTASK_LOOP_NUMBER];

/*check audit message*/
static int diting_ktask_loop_chkqueue(void *arg)
{
	while(diting_ktask_run_t){
		struct diting_common_msgnode *item = NULL;
		struct diting_procrun_msgnode *procrun_item = NULL;

		diting_nolockqueue_module.dequeue(diting_nolockqueue_module.getque(), (void **)&item);
		if(!item || IS_ERR(item)){
			msleep(1000);
			continue;
		}

		switch(item->type){
			case DITING_PROCRUN:
				procrun_item = (struct diting_procrun_msgnode *)item;
				printk("-------uid:%d  username: %s---proc:%s\n", procrun_item->uid, procrun_item->username, procrun_item->proc);
				break;
			case DITING_PROCACCESS:
				break;
			case DITING_KILLER:
				break;
			default:
				break;	
		}

		kfree(item);
	}
	return 0;
}

static int
diting_ktask_module_init(void)
{
	int i = 0;

	for(; i < DITING_KTASK_LOOP_NUMBER; i++){
		lo[i].lo_thread = NULL;
		lo[i].lo_status = KTASK_INIT;
		lo[i].lo_number = i;
	}

	diting_ktask_run_t = 0;

	return 0;
}

static int diting_ktask_module_create(void)
{
	lo[0].lo_thread = kthread_create(diting_ktask_loop_chkqueue, 
			&(lo[0]), "loop%d"	,lo[0].lo_number);
	if(IS_ERR(lo[0].lo_thread))
		return -1;

	lo[0].lo_status = KTASK_READY;

	diting_ktask_run_t = 1;
	return 0;
}


static int diting_ktask_module_run(void)
{
	wake_up_process(lo[0].lo_thread);
	lo[0].lo_status = KTASK_RUNNING;

	return 0;
}


static int diting_ktask_module_destroy(void)
{
	diting_ktask_run_t = 0;
	if(!IS_ERR(lo[0].lo_thread))
		kthread_stop(lo[0].lo_thread);

	return 0;
}


struct diting_ktask_module diting_ktask_module = {
	.init		= diting_ktask_module_init,
	.create		= diting_ktask_module_create,
	.run		= diting_ktask_module_run,
	.destroy	= diting_ktask_module_destroy
};