#ifndef __SECDETECTOR_EBPF_TYPES_H
#define __SECDETECTOR_EBPF_TYPES_H
#ifdef __cplusplus
extern "C"
{
#endif

#define TASK_COMM_SIZE 16
#define EVENT_NAME_SIZE 32
#define MAX_FILENAME_SIZE 256
#define MAX_TEXT_SIZE 4096

struct process_info
{
    unsigned exit_code;
};

struct file_info
{
    char filename[MAX_TEXT_SIZE];
};

struct ebpf_event
{
    int type;
    char event_name[EVENT_NAME_SIZE];
    // common info
    unsigned long long timestamp;
    unsigned uid;
    unsigned gid;
    char exe[MAX_FILENAME_SIZE];
    unsigned pid;
    char comm[TASK_COMM_SIZE];
    unsigned tgid;
    unsigned pgid;
    unsigned ppid;
    char pcomm[TASK_COMM_SIZE];
    unsigned sid;
    char nodename[MAX_FILENAME_SIZE];
    unsigned pns;
    unsigned root_pns;
    union {
        struct process_info process_info;
        struct file_info info;
    };
};

#ifdef __cplusplus
}
#endif
#endif
