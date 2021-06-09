#pragma once

#ifndef app_priv_h
#define app_priv_h

#ifdef __cplusplus
extern "C" {
#endif


#include "stdbool.h"
#include "esp_err.h"

#define FW_VERSION 1

int cloud_start(void);
esp_err_t do_firmware_upgrade(const char *url);

#ifdef __cplusplus
}
#endif

#endif // app_priv_h