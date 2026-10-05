/* Host mocks for test_dns_override.py.
 *
 * The production DNS functions are extracted from main/main.c and inserted
 * below this prelude before compilation. Keep behavior out of this fixture:
 * it supplies only the ESP-IDF/lwIP types and controlled mock boundaries.
 */
#include <assert.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

typedef int esp_err_t;
typedef int err_t;
typedef int esp_netif_dns_type_t;
typedef struct { uint32_t addr; } ip4_addr_t;
typedef struct { uint32_t addr; } esp_ip4_addr_t;
typedef struct {
    unsigned type;
    union { esp_ip4_addr_t ip4; } u_addr;
} esp_ip_addr_t;
typedef struct { esp_ip_addr_t ip; } esp_netif_dns_info_t;
typedef struct { int unused; } esp_netif_t;
typedef struct { char dns[16]; } wifi_network_t;

#define ESP_OK 0
#define ESP_FAIL -1
#define ERR_OK 0
#define ESP_IPADDR_TYPE_V4 0
#define ESP_IPADDR_TYPE_ANY 46
#define ESP_NETIF_DNS_MAIN 0
#define ESP_NETIF_DNS_BACKUP 1
#define ESP_NETIF_DNS_FALLBACK 2
#define ESP_IP_IS_ANY(ip) ((ip).u_addr.ip4.addr == 0)
#define ESP_LOGW(...) ((void)0)
#define ESP_LOGI(...) ((void)0)
#define TAG_STA "test"
#define esp_err_to_name(err) "mock"

static int s_net_current;
static wifi_network_t mock_network;
static bool mock_network_present;
static esp_netif_dns_info_t mock_slots[3];
static int mock_set_calls[3];
static int mock_clear_calls;
static int mock_fail_set_type = -1;
static err_t mock_callback_result = ERR_OK;

static bool wifi_networks_get(int idx, wifi_network_t *out)
{
    if (!mock_network_present || idx != s_net_current) return false;
    *out = mock_network;
    return true;
}

static bool ip4addr_aton(const char *s, ip4_addr_t *out)
{
    unsigned a, b, c, d;
    if (sscanf(s, "%u.%u.%u.%u", &a, &b, &c, &d) != 4
        || a > 255 || b > 255 || c > 255 || d > 255) return false;
    out->addr = a | (b << 8) | (c << 16) | (d << 24);
    return true;
}

static esp_err_t esp_netif_get_dns_info(esp_netif_t *sta,
                                        esp_netif_dns_type_t type,
                                        esp_netif_dns_info_t *out)
{
    (void)sta;
    *out = mock_slots[type];
    return ESP_OK;
}

static esp_err_t esp_netif_set_dns_info(esp_netif_t *sta,
                                        esp_netif_dns_type_t type,
                                        esp_netif_dns_info_t *dns)
{
    (void)sta;
    mock_set_calls[type]++;
    if (type == mock_fail_set_type) return ESP_FAIL;
    mock_slots[type] = *dns;
    return ESP_OK;
}

static void dns_setserver(unsigned type, const void *dns)
{
    assert(type == ESP_NETIF_DNS_FALLBACK);
    assert(dns == NULL);
    mock_slots[type].ip.type = ESP_IPADDR_TYPE_ANY;
    mock_slots[type].ip.u_addr.ip4.addr = 0;
    mock_clear_calls++;
}

static err_t tcpip_callback_wait(void (*fn)(void *), void *arg)
{
    if (mock_callback_result != ERR_OK) return mock_callback_result;
    fn(arg);
    return ERR_OK;
}
