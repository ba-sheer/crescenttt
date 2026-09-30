#ifndef CRESCENT_JSON_H
#define CRESCENT_JSON_H

#include<stddef.h>

typedef enum{
    JSON_NULL = 0,
    JSON_BOOL,
    JSON_NUMBER,
    JSON_STRING,
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
#endif