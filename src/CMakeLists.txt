idf_component_register(
    SRC_DIRS "." "controller-state" "timer"
    INCLUDE_DIRS "."
    PRIV_REQUIRES esp_driver_gpio
    PRIV_REQUIRES esp_driver_gpio bt
)
