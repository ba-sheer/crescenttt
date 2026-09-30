#include "config.h"
#include "http.h"
int main(){
    http_global_init();

    crescent_config cfg;
    config_load(&cfg);


    http_global_cleanup();


    return 0;
}