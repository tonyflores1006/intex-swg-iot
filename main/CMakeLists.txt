idf_component_register(SRCS "IntexSWG.cpp" 
                            "TM1650.cpp" 
                            "utils.cpp" 
                            "RestServer.cpp"
                            "cloud_aws.c"
                            "aws_custom_utils.c"
                            "ota.c"
                    INCLUDE_DIRS ".")

set(COMPONENT_REQUIRES aws_iot mcommon mconfig mwifi mlink)

target_add_binary_data(${COMPONENT_TARGET} "cloud_cfg/server.cert" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "cloud_cfg/device.cert" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "cloud_cfg/device.key" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "cloud_cfg/deviceid.txt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "cloud_cfg/endpoint.txt" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "cloud_cfg/github_server.cert" TEXT)
