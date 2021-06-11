#ifndef RestServer_h
#define RestServer_h

#ifdef __cplusplus
extern "C" {
#endif

    void systemRebootTask(void * parameter);
    void start_rest_server(unsigned int port);
    void stop_webserver();

#ifdef __cplusplus
}
#endif

#endif // RestServer_h