idf_component_register(
    SRCS "main.cpp"
         "SaberController.cpp"
         "samples/smoothswing/SmoothSwingSample.cpp"
         "effects/BladeIgnite.cpp"
         "effects/BladeSpark.cpp"
         "effects/BlasterImpact.cpp"
         "effects/BladeClash.cpp"
         "effects/BladeDrag.cpp"
    INCLUDE_DIRS "." 
                 "samples" 
                 "samples/smoothswing"
                 "effects"
    REQUIRES sd_card audio_engine mpu6050 gpio_button smart_led rgb_led
)

target_compile_features(${COMPONENT_LIB} PRIVATE cxx_std_20)
