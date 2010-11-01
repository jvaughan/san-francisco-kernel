
#include <linux/module.h>
#include <linux/init.h>
#include <linux/debugfs.h>
#include <linux/err.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/proc_fs.h>
#include <linux/bootmem.h>
#include <linux/zte_memlog.h>

static spinlock_t logbuf_lock = SPIN_LOCK_UNLOCKED;

static char *ptr_trace_buffer = NULL;
static char *ptr_printk_buffer = NULL;
static int dump_in_progress = 0;

void mem_printk(const char *s)
{
	int i = 0,idx;
	unsigned long flags;
	arm11log_type *alog_tag = (arm11log_type *)A11LOG_TAG_V_BASE;
		
	if ((ptr_printk_buffer == NULL) || (dump_in_progress == 1))
		return;
	
	spin_lock_irqsave(&logbuf_lock, flags);
	idx = alog_tag->trace_buffer_idx;
	while(s[i] && (i<1024)){
		*(ptr_printk_buffer+idx) = s[i];
		if (idx == A11LOG_SIZE -1)
			alog_tag->trace_buffer_len = A11LOG_SIZE;
		
		idx = (idx+1)%A11LOG_SIZE;
		i++;
	}
	alog_tag->trace_buffer_idx = idx;
	spin_unlock_irqrestore(&logbuf_lock, flags);
	return;
}

void mem_trace(const char *s)
{
	int i = 0,idx;
	unsigned long flags;
	volatile arm11log_type *alog_tag = (arm11log_type *)(A11LOG_TAG_V_BASE+16);
		
	if (ptr_trace_buffer == NULL)
		return;
	if (alog_tag->trace_buffer_idx >= A11TRACE_LENGTH) {
		ptr_trace_buffer = NULL;
		return;
	}
	spin_lock_irqsave(&logbuf_lock, flags);
	idx = alog_tag->trace_buffer_idx;
	while(s[i] && (idx<A11TRACE_LENGTH) && (i<1024)){
		*(ptr_trace_buffer+idx) = s[i];
		i++;
		idx++;	
	}
	alog_tag->trace_buffer_idx = idx;
	spin_unlock_irqrestore(&logbuf_lock, flags);
	return;
}

void init_trace_buffer(void)
{
	arm11log_type *alog_tag = (arm11log_type *)(A11LOG_TAG_V_BASE + 16);
 
	ptr_trace_buffer = phys_to_virt(0x20900000);
		
	if ((alog_tag->trace_buffer_magic == MAGIC_TRACE) &&
		(alog_tag->trace_buffer_idx < A11TRACE_LENGTH)){
		return;
	} else {
		alog_tag->trace_buffer_magic = MAGIC_TRACE;
		alog_tag->trace_buffer_idx = 0;
	}
}


void init_printk_buffer(void)
{
	arm11log_type *alog_tag = (arm11log_type *)A11LOG_TAG_V_BASE;
	ptr_printk_buffer = (char *)A11LOG_V_BASE;
	if ((alog_tag->trace_buffer_magic == MAGIC_PRINTK) &&
		(alog_tag->trace_buffer_idx < A11LOG_SIZE))
		return;
	else {
		alog_tag->trace_buffer_magic = MAGIC_PRINTK;
		alog_tag->trace_buffer_idx = 0;
		alog_tag->trace_buffer_len = 0;
		memset(ptr_printk_buffer, 0, A11LOG_SIZE);
	}
}


static int printk_data_read_proc(char *buf, char **start, off_t off,
				  int count, int *eof, void *data)
{
	int len;
	int remains;
	arm11log_type *alog_tag = (arm11log_type *)(A11LOG_TAG_V_BASE);
	
	*start = buf;
	dump_in_progress = 1;
	
	if (alog_tag->trace_buffer_len == 0)
  		remains = alog_tag->trace_buffer_idx - off;
	else
		remains = A11LOG_SIZE - off;
		
	if (remains < 0) {
		dump_in_progress = 0;
		*eof = 1;
		return 0;
	}	
	if (count > remains) {
		len = remains;
		dump_in_progress = 0;
		*eof = 1;
	} else {
		len = count;
		*eof = 0;
	}
	if (!buf || (count <= 0))
		goto out;
	if (len > A11LOG_SIZE)
		len = A11LOG_SIZE;
	if (alog_tag->trace_buffer_len == 0){ /* not wrapped */
		memcpy(buf, ptr_printk_buffer + off , len);
	} else if ((off + len)<=(A11LOG_SIZE - alog_tag->trace_buffer_idx)){ 
			memcpy((char *) buf, ptr_printk_buffer + alog_tag->trace_buffer_idx + off, len);
	} else if (off >= (A11LOG_SIZE - alog_tag->trace_buffer_idx)){
			memcpy((char *) buf, ptr_printk_buffer + off - (A11LOG_SIZE - alog_tag->trace_buffer_idx), len);
	} else {
			int len1, len2;
			len1 = A11LOG_SIZE - off - alog_tag->trace_buffer_idx;
			len2 = len - len1;
			if ((len1>0) && (len2>0)) {
				memcpy((char *) buf, ptr_printk_buffer + off + alog_tag->trace_buffer_idx, len1);
				memcpy((char *) buf + len1, ptr_printk_buffer, len2);
			}	
	}

	return len;
out:
	printk("error happen: -1\n");
	return -1;
}

static int trace_data_read_proc(char *buf, char **start, off_t off,
			       int count, int *eof, void *data)
{
	char *ptr = (char *) ptr_trace_buffer;
	int len = 0;
	int remains;
	unsigned int to_read;
	arm11log_type *alog_tag = (arm11log_type *)(A11LOG_TAG_V_BASE + 16);
	
    to_read = alog_tag->trace_buffer_idx;

	if (to_read > A11TRACE_LENGTH)
		to_read = 0;
	
	*start = buf;
	remains = to_read - off;
	if (remains < 0) {
		alog_tag->trace_buffer_idx = 0;
		return 0;
	}

	if (count > remains) 
	{
		len = remains;
		
		*eof = 1;
	} 
	else
	{
		len = count;
		*eof = 0;
	}

	if (!buf || (count <= 0))
		goto out;

	if (len > A11TRACE_LENGTH)	/* now 128kb */
		len = A11TRACE_LENGTH;

	memcpy((char *) buf, (char *) ptr + off, len);

	if (*eof == 1)
		alog_tag->trace_buffer_idx = 0;

	return len;
out:
	printk("error happen\n");
	return -1;

}

void log_init(void)
{
	create_proc_read_entry("lastboot", 0, (void *)0, printk_data_read_proc,(void *)0);
	create_proc_read_entry("lasttrace", 0, (void *)0, trace_data_read_proc,(void *)0);
}

