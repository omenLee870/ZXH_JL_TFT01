#ifndef ZXH_TEST_PLATFORM_H
#define ZXH_TEST_PLATFORM_H
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
typedef int OS_MUTEX;
#define SEC_USED(section)
#define late_initcall(fn)
#define CTP_NOTIFY_COMMAND 3
struct ctp_map_entry {
    void *unused; const char *topic;
    int (*get)(void *, char *); int (*put)(void *, char *);
};
enum json_type { json_type_object, json_type_string, json_type_int };
struct json_object { enum json_type type; char value[256]; int count;
    char keys[32][32]; struct json_object *children[32]; };
static uint32_t now;
static int sends, last_code, last_confirmed, assertions, submissions;
static char last_phase[32], last_id[64];
static char last_actions[256];
static char reply_keys[32][32], reply_values[32][256];
static unsigned reply_count;
static struct json_object *incoming;
static int os_mutex_create(OS_MUTEX *m) { *m=0; return 0; }
static int os_mutex_pend(OS_MUTEX *m,int t) { assert(!*m); *m=1;return 0; }
static void os_mutex_post(OS_MUTEX *m) { assert(*m); *m=0; }
static uint32_t sys_timer_get_ms(void) { return now; }
static int sys_timer_add(void *p,void (*f)(void *),int ms) { return 1; }
static struct json_object *json_object_new_object(void) { return calloc(1,sizeof(struct json_object)); }
static struct json_object *json_object_new_string(const char *s) {
    struct json_object *o=json_object_new_object();o->type=json_type_string;
    assert(strlen(s)<sizeof(o->value));strcpy(o->value,s);return o;
}
static struct json_object *json_object_new_int(int n) {
    char s[24];snprintf(s,sizeof(s),"%d",n);return json_object_new_string(s);
}
static void json_object_put(struct json_object *o) {
    int i;if(!o)return;for(i=0;i<o->count;i++)json_object_put(o->children[i]);free(o);
}
static void json_object_object_add(struct json_object *o,const char *key,struct json_object *v) {
    int i;for(i=0;i<o->count;i++)if(!strcmp(o->keys[i],key)){json_object_put(o->children[i]);o->children[i]=v;return;}
    assert(o->count<32);strcpy(o->keys[o->count],key);o->children[o->count++]=v;
}
static struct json_object *json_object_object_get(struct json_object *o,const char *key) {
    int i;if(!o)return NULL;for(i=0;i<o->count;i++)if(!strcmp(o->keys[i],key))return o->children[i];return NULL;
}
static int json_object_is_type(struct json_object *o,enum json_type type) { return o->type==type; }
static const char *json_object_get_string(struct json_object *o) { return o->value; }
/* Tests supply structured requests; the target build uses the SDK's real json-c parser. */
static struct json_object *json_tokener_parse(const char *ignored) { return incoming; }
static const char *json_object_to_json_string(struct json_object *o) {
    struct json_object *p=json_object_object_get(o,"param");
    unsigned i;reply_count=p->count;
    for(i=0;i<reply_count;i++) { strcpy(reply_keys[i],p->keys[i]);strcpy(reply_values[i],p->children[i]->value); }
    struct json_object *capabilities=json_object_object_get(p,"actions");
    strcpy(last_actions,capabilities?json_object_get_string(capabilities):"");
    last_code=atoi(json_object_get_string(json_object_object_get(p,"code")));
    last_confirmed=atoi(json_object_get_string(json_object_object_get(p,"confirmed")));
    strcpy(last_phase,json_object_get_string(json_object_object_get(p,"phase")));
    strcpy(last_id,json_object_get_string(json_object_object_get(p,"id")));
    return "{}";
}
static const char *reply_value(const char *key) {
    unsigned i;for(i=0;i<reply_count;i++)if(!strcmp(key,reply_keys[i]))return reply_values[i];return "";
}
static int send_ctp_string(int cmd,char *text,const char *topic,void *client) { assert(client);sends++;free(text);return 0; }
#endif
