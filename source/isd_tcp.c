#include "isd_tcp.h"

#include <lwip/tcp.h>

#define BUFSIZE	65536
#define PORT_ISD 902

typedef struct {
    int len;
    char buf[BUFSIZE];
} ISD_TXST;

static ISD_TXST isd_st;

static struct tcp_pcb *isd_pcb;

static err_t isd_tcp_accept(void *arg, struct tcp_pcb *pcb, err_t err);
static void isd_tcp_close(struct tcp_pcb *pcb);

void isd_tcp_init(void) {
    err_t err;

    isd_pcb = tcp_new();

    if (isd_pcb != NULL) {
        err = tcp_bind(isd_pcb, IP_ADDR_ANY, PORT_ISD);

        if (err == ERR_OK) {
            isd_pcb = tcp_listen(isd_pcb);
            tcp_accept(isd_pcb, isd_tcp_accept);
            printf(" * port: %d\n", PORT_ISD);
        }
    }
}

void isd_tcp_send(void const *buf, int len) {
    if (isd_pcb == NULL || buf == NULL || len == 0)
        return;

    tcp_write(isd_pcb, buf, len, 1);
    tcp_output(isd_pcb);
}

static void isd_tcp_close(struct tcp_pcb *pcb) {
    tcp_arg(pcb, NULL);
    tcp_err(pcb, NULL);
    tcp_sent(pcb, NULL);
    tcp_recv(pcb, NULL);
    tcp_close(pcb);
}

int isd_tcp_recv_len;
unsigned char isd_tcp_recv_buf[512];

static err_t isd_tcp_recv(void *arg, struct tcp_pcb *pcb, struct pbuf *p, err_t err) {
    if (err == ERR_OK && p != NULL) {
        unsigned char * payload = (unsigned char*) p->payload;

        tcp_recved(pcb, p->tot_len);

        memcpy(isd_tcp_recv_buf + isd_tcp_recv_len, payload, p->tot_len);
        isd_tcp_recv_len += p->tot_len;
    }

    if (p != NULL) {
        pbuf_free(p);
    }

    if (err == ERR_OK && p == NULL) {
        isd_tcp_close(pcb);
    }

    return ERR_OK;
}

static void isd_tcp_err(void *arg, err_t err) {
    printf("isd_tcp_err : %08x\r\n",err);
}

static err_t isd_tcp_accept(void *arg, struct tcp_pcb *pcb, err_t err) {
    isd_pcb = pcb;
    isd_st.len = 0;
    tcp_arg(pcb, &isd_st);
    tcp_err(pcb, isd_tcp_err);
    tcp_recv(pcb, isd_tcp_recv);

    return ERR_OK;
}
