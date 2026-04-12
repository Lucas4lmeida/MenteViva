#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include "pico/stdlib.h"
#include "pico/cyw43_arch.h"
#include "hardware/sync.h"
#include "lwip/pbuf.h"
#include "lwip/tcp.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/ip_addr.h"
#include "menteviva.h"

#ifndef WIFI_SSID
#define WIFI_SSID "SEU_SSID"
#endif

#ifndef WIFI_PASSWORD
#define WIFI_PASSWORD "SUA_SENHA"
#endif

#define WIFI_TENTATIVAS  3
#define WIFI_TIMEOUT_MS  10000
#define HTTP_PORT        80

typedef struct {
    bool stack_ok;
    bool conectado;
    bool servidor_ok;
    int  simon_ult;
    int  reflexo_ult;
    char trend_simon[8];
    char trend_reflexo[8];
    char ip[20];
    struct tcp_pcb *server_pcb;
} wifi_estado_t;

static wifi_estado_t wifi_estado = {
    .stack_ok = false,
    .conectado = false,
    .servidor_ok = false,
    .simon_ult = -1,
    .reflexo_ult = -1,
    .trend_simon = "N/D",
    .trend_reflexo = "N/D",
    .ip = "0.0.0.0",
    .server_pcb = NULL
};

static char http_body[1200];
static char http_resp[1400];

static void wifi_snapshot(int *simon_ult, char *trend_simon,
                          int *reflexo_ult, char *trend_reflexo, char *ip) {
    uint32_t ints = save_and_disable_interrupts();

    *simon_ult = wifi_estado.simon_ult;
    *reflexo_ult = wifi_estado.reflexo_ult;
    strcpy(trend_simon, wifi_estado.trend_simon);
    strcpy(trend_reflexo, wifi_estado.trend_reflexo);
    strcpy(ip, wifi_estado.ip);

    restore_interrupts(ints);
}

static void wifi_montar_html(void) {
    int simon_ult, reflexo_ult;
    char trend_simon[8];
    char trend_reflexo[8];
    char ip[20];
    char simon_txt[24];
    char reflexo_txt[24];

    wifi_snapshot(&simon_ult, trend_simon, &reflexo_ult, trend_reflexo, ip);

    if (simon_ult >= 0)
        snprintf(simon_txt, sizeof(simon_txt), "Nivel %d", simon_ult);
    else
        snprintf(simon_txt, sizeof(simon_txt), "-");

    if (reflexo_ult >= 0)
        snprintf(reflexo_txt, sizeof(reflexo_txt), "%d ms", reflexo_ult);
    else
        snprintf(reflexo_txt, sizeof(reflexo_txt), "-");

    snprintf(
        http_body, sizeof(http_body),
        "<!doctype html>"
        "<html><head>"
        "<meta charset='utf-8'>"
        "<meta name='viewport' content='width=device-width,initial-scale=1'>"
        "<meta http-equiv='refresh' content='3'>"
        "<title>MenteViva</title>"
        "<style>"
        "body{font-family:Arial,sans-serif;background:#0f172a;color:#e5e7eb;margin:0;padding:16px;}"
        ".wrap{max-width:760px;margin:0 auto;}"
        ".top,.card{background:#111827;border-radius:16px;padding:16px;box-shadow:0 6px 24px rgba(0,0,0,.25);}"
        ".top{margin-bottom:16px;}"
        ".grid{display:grid;grid-template-columns:repeat(auto-fit,minmax(220px,1fr));gap:16px;}"
        ".v{font-size:28px;font-weight:700;margin:8px 0;}"
        ".muted{color:#94a3b8;font-size:14px;}"
        "h1,h2,p{margin:0;}"
        "h2{margin-bottom:8px;}"
        "</style></head><body>"
        "<div class='wrap'>"
        "<div class='top'>"
        "<h1>MenteViva</h1>"
        "<p>Dashboard local do Pico W</p>"
        "<p class='muted'>IP: %s</p>"
        "</div>"
        "<div class='grid'>"
        "<div class='card'>"
        "<h2>Simon</h2>"
        "<div class='v'>%s</div>"
        "<div class='muted'>Tendencia: %s</div>"
        "</div>"
        "<div class='card'>"
        "<h2>Reflexo</h2>"
        "<div class='v'>%s</div>"
        "<div class='muted'>Tendencia: %s</div>"
        "</div>"
        "</div>"
        "<p class='muted' style='margin-top:16px'>Atualizacao automatica a cada 3 s.</p>"
        "</div></body></html>",
        ip,
        simon_txt, trend_simon,
        reflexo_txt, trend_reflexo
    );
}

static err_t http_recv_cb(void *arg, struct tcp_pcb *tpcb, struct pbuf *p, err_t err) {
    (void)arg;
    (void)err;

    if (!p) {
        err_t close_err = tcp_close(tpcb);
        if (close_err != ERR_OK) {
            tcp_abort(tpcb);
            return ERR_ABRT;
        }
        return ERR_OK;
    }

    tcp_recved(tpcb, p->tot_len);
    pbuf_free(p);

    wifi_montar_html();

    int body_len = (int)strlen(http_body);
    int resp_len = snprintf(
        http_resp, sizeof(http_resp),
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/html; charset=utf-8\r\n"
        "Cache-Control: no-store\r\n"
        "Connection: close\r\n"
        "Content-Length: %d\r\n\r\n"
        "%s",
        body_len, http_body
    );

    if (resp_len <= 0 || resp_len >= (int)sizeof(http_resp)) {
        tcp_abort(tpcb);
        return ERR_ABRT;
    }

    if (tcp_write(tpcb, http_resp, (u16_t)resp_len, TCP_WRITE_FLAG_COPY) != ERR_OK) {
        tcp_abort(tpcb);
        return ERR_ABRT;
    }

    if (tcp_output(tpcb) != ERR_OK) {
        tcp_abort(tpcb);
        return ERR_ABRT;
    }

    err_t close_err = tcp_close(tpcb);
    if (close_err != ERR_OK) {
        tcp_abort(tpcb);
        return ERR_ABRT;
    }

    return ERR_OK;
}

static void http_err_cb(void *arg, err_t err) {
    (void)arg;
    (void)err;
}

static err_t http_accept_cb(void *arg, struct tcp_pcb *client, err_t err) {
    (void)arg;

    if (err != ERR_OK || client == NULL)
        return ERR_VAL;

    tcp_recv(client, http_recv_cb);
    tcp_err(client, http_err_cb);
    return ERR_OK;
}

static bool http_server_iniciar(void) {
    bool ok = false;

    cyw43_arch_lwip_begin();

    struct tcp_pcb *pcb = tcp_new_ip_type(IPADDR_TYPE_ANY);
    if (pcb) {
        err_t bind_err = tcp_bind(pcb, IP_ANY_TYPE, HTTP_PORT);
        if (bind_err == ERR_OK) {
            pcb = tcp_listen_with_backlog(pcb, 1);
            if (pcb) {
                tcp_accept(pcb, http_accept_cb);
                wifi_estado.server_pcb = pcb;
                ok = true;
            }
        }

        if (!ok && pcb) {
            tcp_abort(pcb);
        }
    }

    cyw43_arch_lwip_end();
    return ok;
}

void wifi_atualizar_dashboard(int simon_ult, const char *trend_simon,
                              int reflexo_ult, const char *trend_reflexo) {
    uint32_t ints = save_and_disable_interrupts();

    wifi_estado.simon_ult = simon_ult;
    wifi_estado.reflexo_ult = reflexo_ult;

    strncpy(wifi_estado.trend_simon, trend_simon ? trend_simon : "N/D",
            sizeof(wifi_estado.trend_simon) - 1);
    wifi_estado.trend_simon[sizeof(wifi_estado.trend_simon) - 1] = '\0';

    strncpy(wifi_estado.trend_reflexo, trend_reflexo ? trend_reflexo : "N/D",
            sizeof(wifi_estado.trend_reflexo) - 1);
    wifi_estado.trend_reflexo[sizeof(wifi_estado.trend_reflexo) - 1] = '\0';

    restore_interrupts(ints);
}

bool wifi_ativo(void) {
    return wifi_estado.conectado && wifi_estado.servidor_ok;
}

const char *wifi_ip_str(void) {
    return wifi_estado.ip;
}

void wifi_init(void) {
    if (wifi_estado.stack_ok)
        return;

    if (cyw43_arch_init()) {
        printf("[wifi] falha ao iniciar stack\n");
        return;
    }

    wifi_estado.stack_ok = true;
    cyw43_arch_enable_sta_mode();

    printf("[wifi] conectando em %s\n", WIFI_SSID);

    for (int tentativa = 1; tentativa <= WIFI_TENTATIVAS; tentativa++) {
        int rc = cyw43_arch_wifi_connect_timeout_ms(
            WIFI_SSID,
            WIFI_PASSWORD,
            CYW43_AUTH_WPA2_AES_PSK,
            WIFI_TIMEOUT_MS
        );

        if (rc == 0) {
            wifi_estado.conectado = true;
            break;
        }

        printf("[wifi] tentativa %d falhou\n", tentativa);
        sleep_ms(1000);
    }

    if (!wifi_estado.conectado) {
        printf("[wifi] sem conexao, firmware segue normal\n");
        return;
    }

    if (netif_default && netif_is_up(netif_default)) {
        const char *ip = ip4addr_ntoa(netif_ip4_addr(netif_default));
        if (ip) {
            strncpy(wifi_estado.ip, ip, sizeof(wifi_estado.ip) - 1);
            wifi_estado.ip[sizeof(wifi_estado.ip) - 1] = '\0';
        }
    }

    wifi_estado.servidor_ok = http_server_iniciar();

    if (wifi_estado.servidor_ok) {
        printf("[wifi] ok em http://%s/\n", wifi_estado.ip);
    } else {
        printf("[wifi] conectado, mas falhou ao subir http\n");
    }
}

void wifi_poll(void) {
    if (wifi_estado.stack_ok) {
        cyw43_arch_poll();
    }
}
