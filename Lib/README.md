# AMBI ESP32
The firmware for ESP32 Chip of Ambi device

## Build

### Build Manually
1. Spin up the docker container

    ```bash
    # under ambi_fw_esp32/
    docker build -t=ambilabs/ambi_host2_base .
    docker run -it ambilabs/ambi_host2_base /bin/bash
    ```

2. In the container's shell, run the following

    ```bash
    git clone https://github.com/AmbiLabs/ambi_fw_esp32.git
    cd ambi_fw_esp32
    ./setup.sh 
    ./build.sh
    ```

3. Collect built firmware at `ambi_fw_esp32/firmware`. 
### Build with CircleCI