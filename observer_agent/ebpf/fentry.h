#ifndef __SECDETECTOR_EBPF_H
#define __SECDETECTOR_EBPF_H
#ifdef __cplusplus
extern "C"
{
#endif
#include <bpf/libbpf.h>

void StopProcesseBPFProg();
int StartProcesseBPFProg(ring_buffer_sample_fn cb);

#ifdef __cplusplus
}
#endif
#endif /* __SECDETECTOR_EBPF_H */
