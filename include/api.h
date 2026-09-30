#ifndef CRESCENT_API_H
#define CRESCENT_API_H

#include"json.h"
#define CRESCENT_BASE_URL "https://crescent.hackclub.com"

typedef enum{
    API_OK =0,
    API_ERR_TRANSPORT,
    API_ERR_UNAUTHORIZED,
    API_ERR_FORBIDDEN,
    API_ERR_NOT_FOUND,
    API_ERR_RATE_LIMIT,
    API_ERR_DISABLED,
    API_ERR_BAD_JSON,
    API_ERR_OTHER
}api_status;

typedef struct 
{
    api_status status;
    long http_status;
    int retry_after_secs;
    char *message;
    json_value *data;
}api_result;

api_result api_get_me(const char *api_key);
api_result api_list_projects(const char *api_key);
api_result api_get_projects(const char *api_key,long project_id);
api_result api_get_orders(const char *api_key);
api_result api_list_notifications(const char *api_key,int limit);
api_result api_get_announcements(const char *api_key);
api_result api_get_shop_items(void);

void api_result_free(api_result *r);



#endif