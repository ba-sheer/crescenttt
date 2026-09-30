#define _POSIX_C_SOURCE 200809L
#include "http.h"
#include<curl/curl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
struct buf{
    char *data;
    size_t len;
    size_t cap;
};
static size_t write_cb(char *ptr,size_t size,size_t nmemb,void *userdata){
    struct buf *b = (struct buf*)userdata;
    size_t add = size * nmemb;
    if(b->len+add+1>b->cap){
        size_t newcap = (b->cap == 0)? 4096 : b->cap;
        while(newcap < b->len+add +1) newcap*=2;
        b->data=realloc(b->data,newcap);
        b->cap = newcap;
    }
    memcpy(b->data +b->len,ptr,add);
    b->len+=add;
    b->data[b->len]='\0';
    return add;
}

void http_global_init(void){
    curl_global_init(CURL_GLOBAL_DEFAULT);
}
void http_global_init(void){
    curl_global_cleanup();
}
int http_get(const char *url,const char *bearer_token,http_response *out){
    memset(out,0,sizeof(*out));
    CURL *curl = curl_easy_init();
    if(!curl){
        out->transport_error = strdup("failed to initialize lib curl");
        return-1;
    }
    struct buf b ={0};
    struct curl_slist *headers = NULL;
    headers = curl_slist_append(headers,"Accept:application/json");
    headers = curl_slist_append(headers,"User-agent:crescent-tui/1.0(+https://crescent.hackclub.com)");
    if (bearer_token && *bearer_token){
        char auth[512];
        snprintf(auth,sizeof(auth),"Authorization: Bearer %s",bearer_token);
        headers = curl_slist_append(headers,auth);
    }
    curl_easy_setopt(curl, CURLOPT_URL, url);
    curl_easy_setopt(curl, CURLOPT_HTTPHEADER, headers);
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, write_cb);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &b);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 15L);
    curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT, 10L);

    char errbuf[CURL_ERROR_SIZE];
    errbuf[0]="\0";
    curl_easy_setopt(curl,CURLOPT_ERRORBUFFER,errbuf);
    CURLcode res = curl_easy_perform(curl);

    if(res!=CURLE_OK){
        out->transport_error = strdup(errbuf[0]?errbuf:curl_easy_strerror(res));
        curl_slist_free_all(headers);
        curl_easy_cleanup(curl);
        free(b.data);
        return -1;
    }

    long status = 0;
    curl_easy_getinfo(curl,CURLINFO_RESPONSE_CODE,&status);
    out->status= status;
    out->body=b.data?b.data :strdup("");
    out->body_len = b.len;
    

    curl_slist_free_all(headers);
    curl_easy_cleanup(curl);
    return 0;

}


void http_response_free(http_response *r){
    if(!r) return;
    free(r->body);
    free(r->transport_error);
    r->body=NULL;
    r->transport_error=NULL;
}