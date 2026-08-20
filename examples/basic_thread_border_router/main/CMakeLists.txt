idf_component_register(SRCS "esp_ot_br.c"
                       "led_indicator.c"
                       INCLUDE_DIRS "."
                       EMBED_TXTFILES ${project_dir}/server_certs/ca_cert.pem
                       REQUIRES driver led_strip
                       )
