idf_component_register(
    SRC_DIRS "." "controller-state" "timer" "bluetooth"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio bt nvs_flash wpa_supplicant esp_wifi
)
