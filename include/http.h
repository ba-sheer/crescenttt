#ifdef CRESCENT_HTTP_H
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

#endif