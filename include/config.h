#ifndef CRESCENT_CONFIG_H
#define CRESCENT_CONFIG_H

//API-details
typedef struct 
{
    char api_key[128];
    char region[8];//as per docs eg:region:"us"
} crescent_config;
//loading config from tui or from file
void config_load(crescent_config *cfg);
//writing to file   0 if succeded
int config_save(const crescent_config *cfg);

#endif