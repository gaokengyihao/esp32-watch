#ifndef __OTA_H__
#define __OTA_H__

#include <stdint.h>
#include <stddef.h>

void ota_handle_frame(const uint8_t *data, size_t len);
void ota_mark_valid(void);

#endif