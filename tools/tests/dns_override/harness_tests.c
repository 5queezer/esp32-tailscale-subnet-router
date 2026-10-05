/* Assertions for the production functions inserted by test_dns_override.py. */
static uint32_t addr(const char *s)
{
    ip4_addr_t parsed;
    assert(ip4addr_aton(s, &parsed));
    return parsed.addr;
}

static void reset(const char *override)
{
    memset(mock_slots, 0, sizeof(mock_slots));
    memset(mock_set_calls, 0, sizeof(mock_set_calls));
    mock_clear_calls = 0;
    mock_fail_set_type = -1;
    mock_callback_result = ERR_OK;
    mock_network_present = true;
    memset(&mock_network, 0, sizeof(mock_network));
    if (override) {
        snprintf(mock_network.dns, sizeof(mock_network.dns), "%s", override);
    }
}

static void set_slot(int type, const char *value)
{
    mock_slots[type].ip.type = ESP_IPADDR_TYPE_V4;
    mock_slots[type].ip.u_addr.ip4.addr = addr(value);
}

int main(void)
{
    esp_netif_t sta;
    const char *override = "192.0.2.53";

    reset(override);
    set_slot(ESP_NETIF_DNS_MAIN, "198.51.100.53");
    set_slot(ESP_NETIF_DNS_BACKUP, "203.0.113.53");
    set_slot(ESP_NETIF_DNS_FALLBACK, "198.51.100.54");
    assert(wifi_apply_dns_override(&sta));
    assert(mock_slots[0].ip.u_addr.ip4.addr == addr(override));
    assert(mock_slots[1].ip.u_addr.ip4.addr == addr(override));
    assert(mock_slots[2].ip.type == ESP_IPADDR_TYPE_ANY);
    assert(mock_set_calls[0] == 1 && mock_set_calls[1] == 1);
    assert(mock_clear_calls == 1);
    puts("PASS initial apply");

    set_slot(ESP_NETIF_DNS_MAIN, "198.51.100.53");
    set_slot(ESP_NETIF_DNS_BACKUP, "198.51.100.54");
    assert(wifi_apply_dns_override(&sta));
    assert(mock_slots[0].ip.u_addr.ip4.addr == addr(override));
    assert(mock_slots[1].ip.u_addr.ip4.addr == addr(override));
    assert(mock_set_calls[0] == 2 && mock_set_calls[1] == 2);
    assert(mock_clear_calls == 1);
    puts("PASS renewal overwrite repair");

    mock_network.dns[0] = '\0';
    set_slot(ESP_NETIF_DNS_MAIN, "198.51.100.53");
    set_slot(ESP_NETIF_DNS_BACKUP, "203.0.113.53");
    assert(!wifi_apply_dns_override(&sta));
    assert(mock_slots[0].ip.u_addr.ip4.addr == addr("198.51.100.53"));
    assert(mock_slots[1].ip.u_addr.ip4.addr == addr("203.0.113.53"));
    assert(mock_slots[2].ip.type == ESP_IPADDR_TYPE_ANY);
    assert(mock_clear_calls == 1);
    puts("PASS explicit-to-inherited transition");

    reset("");
    set_slot(ESP_NETIF_DNS_MAIN, "198.51.100.53");
    set_slot(ESP_NETIF_DNS_BACKUP, "203.0.113.53");
    assert(!wifi_apply_dns_override(&sta));
    assert(mock_slots[0].ip.u_addr.ip4.addr == addr("198.51.100.53"));
    assert(mock_slots[1].ip.u_addr.ip4.addr == addr("203.0.113.53"));
    assert(mock_set_calls[0] == 0 && mock_set_calls[1] == 0);
    assert(mock_clear_calls == 0);
    puts("PASS no override");

    reset(override);
    set_slot(ESP_NETIF_DNS_MAIN, override);
    set_slot(ESP_NETIF_DNS_BACKUP, override);
    assert(!wifi_apply_dns_override(&sta));
    assert(mock_set_calls[0] == 0 && mock_set_calls[1] == 0);
    assert(mock_clear_calls == 0);
    puts("PASS no-op");

    reset(override);
    set_slot(ESP_NETIF_DNS_MAIN, "198.51.100.53");
    set_slot(ESP_NETIF_DNS_BACKUP, "203.0.113.53");
    mock_fail_set_type = ESP_NETIF_DNS_BACKUP;
    assert(wifi_apply_dns_override(&sta));
    assert(mock_slots[0].ip.u_addr.ip4.addr == addr(override));
    assert(mock_slots[1].ip.u_addr.ip4.addr == addr("203.0.113.53"));
    assert(mock_set_calls[0] == 1 && mock_set_calls[1] == 1);
    puts("PASS partial set failure");

    reset(override);
    set_slot(ESP_NETIF_DNS_MAIN, override);
    set_slot(ESP_NETIF_DNS_BACKUP, override);
    set_slot(ESP_NETIF_DNS_FALLBACK, "198.51.100.54");
    mock_callback_result = -2;
    assert(!wifi_apply_dns_override(&sta));
    assert(mock_slots[2].ip.u_addr.ip4.addr == addr("198.51.100.54"));
    assert(mock_clear_calls == 0);
    puts("PASS fallback clear failure");

    return 0;
}
