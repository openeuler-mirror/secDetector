#include <stdio.h>
#include "../ebpf_types.h"
#include "../fentry.h"
#include "../../../include/secDetector_topic.h"

static int handle_event(void *ctx, void *data, size_t data_sz)
{
	const struct ebpf_event *e = data;
	printf("timestamp:%llu event_name:%s exe:%s pid:%u tgid:%u uid:%u gid:%u comm:%s"
		" sid:%u ppid:%u pgid:%u pcomm:%s nodename:%s pns:%u root_pns:%u",
		e->timestamp, e->event_name, e->exe, e->pid, e->tgid, e->uid, e->gid, e->comm, e->sid, e->ppid, e->pgid,e->pcomm, e->nodename, e->pns, e->root_pns);
    	if (e->type & (CREATFILE | DELFILE | SETFILEATTR | WRITEFILE | READFILE))
		printf(" filename:%s", e->file_info.filename);
	if (e->type & SETFILEATTR)
		printf(" name:%s", e->file_info.name);
	printf(" exit_code: %u\n", e->process_info.exit_code);
	return 0;
}

static int libbpf_print_fn(enum libbpf_print_level level, const char *format, va_list args)
{
	return vfprintf(stderr, format, args);
}

int main()
{
	/* Set up libbpf errors and debug info callback */
	libbpf_set_print(libbpf_print_fn);
	StartFileBPFProg(handle_event);
}
