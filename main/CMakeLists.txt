idf_component_register(
    SRCS "main.cpp"
         "samples/smoothswing/SmoothSwingSample.cpp"
         "effects/BladeIgnite.cpp"
    INCLUDE_DIRS "." 
                 "samples" 
                 "samples/smoothswing"
                 "effects"
    REQUIRES sd_card audio_engine mpu6050 gpio_button smart_led
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
