set(INC "."
        "camera"
        "DHT11"
        "wifi"
        "Led"
        "OV2640"
        "IRSend"
        "IRReceive")
set(SRC "."
        "main.c"
        "DHT11/dht11.c"
        "wifi/app_wifi.c"
        "Led/led.c"
        "OV2640/stream_server.c"
        "OV2640/rtsp_server.cpp"
        "OV2640/CRtspSession.cpp"
        "OV2640/CStreamer.cpp"
        "OV2640/OV2640.cpp"
        "OV2640/OV2640Streamer.cpp"
        # "IRSend/IRSend.c"
        "IRSend/IRSendRaw.c"
        "IRReceive/IRReceive.c")

idf_component_register( SRCS ${SRC}
                        INCLUDE_DIRS ${INC}
                        PRIV_INCLUDE_DIRS .
                        PRIV_REQUIRES esp32-camera nvs_flash esp_http_server camera_example_common)
target_compile_options(${COMPONENT_LIB} PRIVATE "-Wno-format")
