#include "config.h"
#include "http.h"
#include "ui.h"
int main(){
    http_global_init();

    crescent_config cfg;
    config_load(&cfg);

    ui_run(&cfg);


    http_global_cleanup();


    return 0;
}