#ifndef CRESCENT_JSON_H
#define CRESCENT_JSON_H

#include<stddef.h>

typedef enum{
    JSON_NULL = 0,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
    JSON_ARRAY,
    JSON_OBJECT
} json_type;

typedef struct json_value{
    json_type type;
    union 
    {
        int boolean;
        double number;
        char *string;
    struct 
    {
        struct json_value **items;
        size_t count;
        
    }array;
    struct 
    {
        char **keys;
        struct json_value **values;
        size_t count;
        
    }object;
    }u; 
}json_value;

json_value *json_parse(const char *text,char **err);
void json_free(json_value *v);
json_value *json_get(const json_value *obj,const char *key);
int json_is_null(const json_value *obj,const char *key);

const char *json_get_string(
    const json_value *obj,
    const char *key,
    const char *def

);

long json_get_int(
    const json_value *obj,
    const char *key,
    long def
);

double json_get_double(
    const json_value *obj,
    const char *key,
    double def
);
int json_get_bool(
    const json_value *obj,
    const char *key,
    int def
);
size_t json_array_count(const json_value *arr);
json_value *json_array_at(const json_value *arr,size_t idx);
#endif