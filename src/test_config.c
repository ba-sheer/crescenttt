#include<stdio.h>
#include"config.h"
int main(void){
    crescent_config cfg;
    config_load(&cfg);
    printf("api key%s",cfg.api_key);
    printf("region%s",cfg.region);
    return 0;
}