#include "CRtspSession.h"
#include "OV2640Streamer.h"
#include "esp_timer.h"
#include "driver/ledc.h"
#include "esp_log.h"
// #include "esp_http_client.h"

//Run `idf.py menuconfig` and set various other options ing "ESP32CAM Configuration"

//common camera settings, see below for other camera options
#define CAM_FRAMESIZE FRAMESIZE_VGA //enough resolution for a simple security cam
#define CAM_QUALITY 12 //0 to 63, with higher being lower-quality (and less bandwidth), 12 seems reasonable

OV2640 cam;

//lifted from Arduino framework
unsigned long millis()
{
    return (unsigned long) (esp_timer_get_time() / 1000ULL);
}

void delay(uint32_t ms)
{
    vTaskDelay(ms / portTICK_PERIOD_MS);
}

void client_worker(void * client)
{
    OV2640Streamer * streamer = new OV2640Streamer((SOCKET)client, cam);
    CRtspSession * session = new CRtspSession((SOCKET)client, streamer);

    unsigned long lastFrameTime = 0;
    const unsigned long msecPerFrame = (1000 / CONFIG_CAM_FRAMERATE);

    while (session->m_stopped == false)
    {
        session->handleRequests(0);

        unsigned long now = millis();
        if ((now > (lastFrameTime + msecPerFrame)) || (now < lastFrameTime))
        {
            session->broadcastCurrentFrame(now);
            lastFrameTime = now;
        }
        else
        {
            //let the system do something else for a bit
            vTaskDelay(1);
        }
    }

    //shut ourselves down
    delete streamer;
    delete session;
    vTaskDelete(NULL);
}

extern "C" void rtsp_server(void);
void rtsp_server(void)
{
    SOCKET server_socket;
    SOCKET client_socket;
    sockaddr_in server_addr;
    sockaddr_in client_addr;
    socklen_t client_addr_len = sizeof(client_addr);
    TaskHandle_t xHandle = NULL;

    camera_config_t config = esp32cam_aithinker_config;
    config.frame_size = CAM_FRAMESIZE;
    config.jpeg_quality = CAM_QUALITY; 
    config.xclk_freq_hz = 16500000; //seems to increase stability compared to the full 20000000
    cam.init(config);

    //setup other camera options
    sensor_t * s = esp_camera_sensor_get();
    //s->set_brightness(s, 0);     // -2 to 2
    s->set_contrast(s, 1);       // -2 to 2
    //s->set_saturation(s, 0);     // -2 to 2
    //s->set_special_effect(s, 2); // 0 to 6 (0 - No Effect, 1 - Negative, 2 - Grayscale, 3 - Red Tint, 4 - Green Tint, 5 - Blue Tint, 6 - Sepia)
    //s->set_whitebal(s, 0);       // 0 = disable , 1 = enable
    //s->set_awb_gain(s, 1);       // 0 = disable , 1 = enable
    s->set_wb_mode(s, 2);        // 0 to 4 - if awb_gain enabled (0 - Auto, 1 - Sunny, 2 - Cloudy, 3 - Office, 4 - Home)
    //s->set_exposure_ctrl(s, 1);  // 0 = disable , 1 = enable
    //s->set_aec2(s, 0);           // 0 = disable , 1 = enable
    //s->set_ae_level(s, 0);       // -2 to 2
    //s->set_aec_value(s, 300);    // 0 to 1200
    //s->set_gain_ctrl(s, 1);      // 0 = disable , 1 = enable
    //s->set_agc_gain(s, 0);       // 0 to 30
    //s->set_gainceiling(s, (gainceiling_t)0);  // 0 to 6
    //s->set_bpc(s, 0);            // 0 = disable , 1 = enable
    //s->set_wpc(s, 1);            // 0 = disable , 1 = enable
    //s->set_raw_gma(s, 1);        // 0 = disable , 1 = enable
    //s->set_lenc(s, 1);           // 0 = disable , 1 = enable
    s->set_hmirror(s, CONFIG_CAM_HORIZONTAL_MIRROR); // 0 = disable , 1 = enable
    s->set_vflip(s, CONFIG_CAM_VERTICAL_FLIP);       // 0 = disable , 1 = enable
    //s->set_dcw(s, 1);            // 0 = disable , 1 = enable
    //s->set_colorbar(s, 0);       // 0 = disable , 1 = enable

    server_addr.sin_family      = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port        = htons(8554); // listen on RTSP port 8554
    server_socket               = socket(AF_INET, SOCK_STREAM, 0);

    int enable = 1;
    if (setsockopt(server_socket, SOL_SOCKET, SO_REUSEADDR, &enable, sizeof(int)) < 0)
    {
        printf("setsockopt(SO_REUSEADDR) failed! errno=%d\n", errno);
    }

    // bind our server socket to the RTSP port and listen for a client connection
    if (bind(server_socket, (sockaddr*)&server_addr, sizeof(server_addr)) != 0)
    {
        printf("bind failed! errno=%d\n", errno);
    }

    if (listen(server_socket, 5) != 0)
    {
        printf("listen failed! errno=%d\n", errno);
    }

    printf("\n\nrtsp://%s.localdomain:8554/mjpeg/1\n\n", CONFIG_LWIP_LOCAL_HOSTNAME);

    // loop forever to accept client connections
    while (true)
    {
        printf("Minimum free heap size: %d bytes\n", esp_get_minimum_free_heap_size());
        client_socket = accept(server_socket, (struct sockaddr*)&client_addr, &client_addr_len);

        printf("Client connected from: %s\n", inet_ntoa(client_addr.sin_addr));

        xTaskCreate(client_worker, "client_worker", 3584, (void*)client_socket, tskIDLE_PRIORITY, &xHandle);
    }

    close(server_socket);
}

// //quick and dirty hack to convert a raw buffer into a C-style null-terminated string
// void null_terminate_string(char * data, int data_len, char * normal_string, int normal_string_len)
// {
//     memset(normal_string, 0, normal_string_len);

//     if (data_len > (normal_string_len - 1))
//     {
//         data_len = normal_string_len - 1;
//     }

//     memcpy(normal_string, data, data_len);
// }
