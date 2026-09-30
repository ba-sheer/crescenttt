#ifndef CRESCENT_HTTP_H
#define CRESCENT_HTTP_H
#include<stddef.h>
typedef struct 
{
    char *body;
    size_t body_len;
    long status;
    char *transport_error;
}http_response;

int http_get(const char *url ,const char *bearer_token,http_response *out);
void http_response_free(http_response *r);
void http_global_init(void);
void http_global_cleanup(void);

#endif