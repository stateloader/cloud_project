#ifndef SECRETS__H
#define SECRETS__H

#include <pgmspace.h>

#define WIFI_SSID "SSID PHRASE"
#define WIFI_PASS "PASSWORD"

static const char *AWS_CRT PROGMEM = R"EOF(
-----BEGIN CERTIFICATE-----
<AWS CERTIFICATE>
-----END CERTIFICATE-----
)EOF";

static const char *DEV_CRT PROGMEM = R"KEY(
-----BEGIN CERTIFICATE-----
<DEVICE CERTIFICATE>
-----END CERTIFICATE-----
)KEY";

static const char *DEV_KEY PROGMEM = R"KEY(
-----BEGIN RSA PRIVATE KEY-----
<DEVICE PRIVATE KEY>
-----END RSA PRIVATE KEY-----
)KEY";

#endif
