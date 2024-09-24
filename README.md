# OTA Updater Project  
This project implements an Over-The-Air (OTA) update mechanism for ESP32 using the ESP-IDF framework. The main requirement is to ensure that the application binaries remain independent and unaware of the update system. This includes all the essential functionalities such as Wi-Fi initialization, NVS (Non-Volatile Storage) management, OTA updates, and secure communication mtls with a server.


## Attention Points and Warnings
To accommodate different flash sizes, the partition scheme can be easily adjusted by updating the ota_1 partition (the application partition) accordingly(currently it is configured for 8MB). Remember to check if the size of the application binary uploaded in the server will fit in the ota_1 partition.

As already discussed before in the chat, for the project to work properly the application bin cannot call the esp_ota_mark_app_valid_cancel_rollback() function.

Another consideration is modifying the NVS "mtls_auth" namespace from the uploaded application may result in improper functionality of the code.

Please before running or testing the code make sure to change the wifi credentials to yours and also change the deviceId to a newly generated one from the server.



## Main Functionalities
- Wi-Fi Initialization: Establishes a connection to a specified Wi-Fi network.
- NVS Management: Facilitates the storage and retrieval of certificates, keys, and firmware versions.
- OTA Updates: Downloads and installs firmware updates from a server.
- Secure Communication: Ensures secure communication with the server using mtls HTTPS.

## Project Structure
```
.devcontainer/
 devcontainer.json
 Dockerfile
.gitignore
.vscode/
 c_cpp_properties.json
 launch.json
 settings.json
 tasks.json
build/
 ...
CMakeLists.txt
main/
 CMakeLists.txt
 main.c
 lib/
 helpers.h
 ota.h
 wifi.h
 ...
partitions.csv
README.md
sdkconfig
sdkconfig.old
```

### Key Files
- `main/main.c`: Serves as the entry point of the application, containing the `app_main` function responsible for orchestrating the OTA update process.
- `main/lib/wifi.h`: Handles Wi-Fi initialization and connection.
- `main/lib/ota.h`: Manages the OTA update process and partition boot selection.
- `main/lib/helpers.h`: Provides utility functions for error handling and logging.
- `main/lib/https.h`: Manages secure communication with the server.
- `main/lib/nvs.h`: Manages the NVS, including loading and saving certificates, private keys, and version numbers.

### Basic Flow of the Program
- The program starts by setting up all the necessary boilerplate for Wi-Fi, NVS, and other components.
- It checks if the certificate and key are already stored in the NVS, indicating that it is not the first boot.
- If they are not found, the program proceeds to generate a CSR (Certificate Signing Request) and a private key.
- The CSR is then sent to the server, which responds with the client certificate.
- This certificate is then stored in the NVS.
- Once the client certificate and private key are obtained, the program checks the version to determine if an update is needed.
- It tries to retrieve the current version from the NVS partition.
- If the version is not found (first boot) or if the current version is older than the server version, it proceeds to update the OTA (Over-The-Air) firmware.
- The latest version is then saved in the NVS.
- Regardless of whether an update was performed or not, the program always sets the next boot partition to be the application data partition (ota_1).
- The system is then restarted.
- On the next boot, the ESP32 will boot into the application partition.
- Due to the enabled rollback option, it will boot into the update partition on the subsequent restart, and the cycle continues.



## How to Set Up
### Prerequisites
- Ensure that ESP-IDF is installed on your system. Follow the ESP-IDF setup guide for installation instructions.

### Configuration
- Wi-Fi Credentials: Set up your Wi-Fi credentials in the `main/lib/wifi.h` file.
- URLs: Configure the server URLs in the `main/lib/https.h` file.
- DeviceId: Configure the deviceId in the `main/lib/https.h` file.

- Menuconfig: Access `idf.py menuconfig` and ensure that the "Enable rollback" option is already enabled in the bootloader options.

### Build and Flash
1. Open a terminal and navigate to the project directory.
2. Execute the following commands to build and flash the project:
```
idf.py idf.py set-target <yourtargethere> //for example: esp32s3
idf.py build
idf.py flash monitor //(requires the ESP32 to be connected via USB to the computer)
```

### Error Handling
The project incorporates robust error-handling mechanisms to ensure system stability. In the event of a critical error, the system will automatically restart to attempt recovery. These error-handling mechanisms can be easily customized as most functionalities are abstracted into separate files.

### Additional Notes
- The Wi-Fi keys, URLs, and deviceId are currently hard-coded for development purposes. If necessary, these can be stored in the NVS.
- If you have any questions or suggestions for alternative approaches for specific points, please don't hesitate to reach out.