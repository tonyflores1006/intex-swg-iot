#ifndef OTAServer_h
#define OTAServer_h

#ifdef __cplusplus
extern "C" {
#endif

//extern httpd_handle_t OTA_server;

void systemRebootTask(void * parameter);
	
//httpd_handle_t start_OTA_webserver(unsigned int port);
//void stop_OTA_webserver(httpd_handle_t server);
void start_OTA_webserver(unsigned int port);
void stop_OTA_webserver();

#ifdef __cplusplus
}
#endif

#endif // OTAServer_h
