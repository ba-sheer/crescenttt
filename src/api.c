#define _POSIX_C_SOURCE 200809L
#include "api.h"
#include "http.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static api_result make_error(api_status status,long http_status,const char *msg){
    api_result r= {0};
    r.status = status;
    r.http_status = http_status;
    r.message = strdup(msg);
    r.data = NULL;
    return r;
}

static api_result do_get(const char *path,const char *api_key){
    char url[512];
    snprintf(url,sizeof(url),"%s%s",CRESCENT_BASE_URL, path);
    http_response resp;
    int rc = http_get(url,api_key,&resp);
    if(rc != 0){
        api_result r= make_error(API_ERR_TRANSPORT,0,
            resp.transport_error ? resp.transport_error : "network error"
        );
        http_response_free(&resp);
        return r;
    }
    char *perr = NULL;
    json_value *body = resp.body_len ? json_parse(resp.body,&perr):NULL;
    long http_status = resp.status;
    if(resp.status == 200){
        if(!body){
            char msg[256];
            snprintf(msg,sizeof(msg),"server returned invalid JSON (%s)",
            perr ? perr :"empty body"
        );
        free(perr);
        http_response_free(&resp);
        return make_error(API_ERR_BAD_JSON,http_status,msg);
        }
        api_result r ={0};
        r.status = API_OK;
        r.http_status = 200;
        r.data = body;
        r.message = NULL;
        http_response_free(&resp);
        return r;
    }

    const char *code = body ? json_get_string(body,"error",NULL): NULL;
    api_status status;
    char msg[256];
    switch (resp.status)
    {
    case 401:
        status = API_ERR_UNAUTHORIZED;
        snprintf(msg,sizeof(msg),"unauthorized: no key, or a key that is wrong or revoked");
        break;
    case 403:
        status = API_ERR_FORBIDDEN;
        snprintf(msg,sizeof(msg),"forbidden: this account is banned");
        break;
    case 404:
        status = API_ERR_NOT_FOUND;
        snprintf(msg,sizeof(msg),"not found");
        break;
    case 429:
        status = API_ERR_RATE_LIMIT;
        long retry = body ? json_get_int(body,"retryAfter",60):60;
        snprintf(msg,sizeof(msg),"rate limited:over 60 requests/min,retry in %lds",retry);
        api_result r = make_error(status,resp.status,msg);
        r.retry_after_secs= (int)retry;
        json_free(body);
        free(perr);
        http_response_free(&resp);
        return r;
    case 503:
        status = API_ERR_DISABLED;
        snprintf(msg,sizeof(msg),"The Crescent API is switched off right now");
        break;
    default:
        status = API_ERR_OTHER;
        snprintf(msg,sizeof(msg),"unexpected HTTP %ld%s%s",resp.status,code ? ":":"",code?code:"");
        break;
    }
    json_free(body);
    free(perr);
    http_response_free(&resp);
    return make_error(status, http_status,msg);
}

api_result api_get_me(const char *api_key){
    return do_get("/api/v1/me",api_key);
}
api_result api_list_projects(const char *api_key){
    return do_get("/api/v1/me/projects",api_key);
}
api_result api_get_project(const char *api_key,long project_id){
    char path[128];
    snprintf(path,sizeof(path),"/api/v1/me/projects/%ld", project_id);
    return do_get(path,api_key);
}
api_result api_list_orders(const char *api_key){
    return do_get("/api/v1/me/orders",api_key);
}
api_result api_list_notifications(const char *api_key,int limit){
    if(limit<1) limit = 1;
    if(limit>50) limit =50;
    char path[64];
    snprintf(path,sizeof(path),"/api/v1/me/notifications?limit=%d",limit);
    return do_get(path,api_key);
}
api_result api_list_announcements(const char *api_key){
    return do_get("/api/v1/announcements",api_key);
}
api_result api_get_shop_items(void){
    return do_get("/api/v1/shop/items",NULL);
}
void api_result_free(api_result *r){
    if(!r)return;
    free(r->message);
    json_free(r->data);
    r->message=NULL;
    r->data = NULL;
}

