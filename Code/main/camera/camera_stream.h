#ifndef CAMERA_STREAM_H
#define CAMERA_STREAM_H

#include "freertos/queue.h"

    esp_err_t start_stream_server(const QueueHandle_t frame_i, const bool return_fb);
    void capture_picture();
#endif