idf_component_register(SRCS "OTAServer.cpp" "IntexSWG.cpp" 
                            "TM1650.cpp" 
                            "utils.cpp" 
                            "RestServer.cpp" 
                    INCLUDE_DIRS ".")

target_add_binary_data(${COMPONENT_TARGET} "WebsiteFiles/indexOTA.html" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "WebsiteFiles/favicon.ico" TEXT)
target_add_binary_data(${COMPONENT_TARGET} "WebsiteFiles/jquery-3.4.1.min.js" TEXT)
