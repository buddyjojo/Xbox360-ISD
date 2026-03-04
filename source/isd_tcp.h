#ifdef __cplusplus
extern "C" {
#endif

void isd_tcp_init();
void isd_tcp_send(void const *buf, int len);

extern int isd_tcp_recv_len;
extern unsigned char isd_tcp_recv_buf[512];

#ifdef __cplusplus
};
#endif
