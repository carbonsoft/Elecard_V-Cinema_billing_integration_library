#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <billing.h>

#include "ini.c"

#include "http_lib.c"
#include "cJSON.c"

typedef struct {
	char *billing_ip;
	char *billing_psw;
	char *log_file;
	cJSON *json;
} billing_config;


/**
 *
 * @param instance
 * @param action
 * @return  if ( billing_verify_action ) { action is allowed } else { action is denyed }
!0 action is allowed
 0 action is denyed

  Warning  Return value is ignored for events:
		BILLING_EVENT_CONNECT
		BILLING_EVENT_DISCONNECT
*/
int				billing_verify_action		(void* instance, billing_action* action){
	billing_config*	conf = (billing_config*)instance;

	int			allowed = 0; // not allowed by default
//
//	if( conf->allow_to_all ){
//		allowed = 1;
//	}
	// How to change RTSP client_port
	if ( action->proto == BILLING_PROTO_RTSP ){
		if ( action->event == BILLING_EVENT_SETUP ){
			if ( strcmp("vod", action->xworks_module) == 0 ){
				//action->client_port1 = 2222;
			}
		}
	}
	return allowed;
}



void* billing_open_instance(const char* config_file_path, void* glib_main_loop){
	billing_config*	conf = malloc(sizeof(billing_config));
	read_config(conf, config_file_path);
	read_user_list(conf);
	read_user_list(conf);
	return conf;
}


void billing_close_instance(void* instance) {
	billing_config *conf = (billing_config *) instance;
	if (conf->json) cJSON_Delete(conf->json);
	free(conf);
}


static int read_config_handler(void* user, const char* section, const char* name, const char* value)
{
	billing_config* pconfig = (billing_config*)user;

#define MATCH(s, n) strcmp(section, s) == 0 && strcmp(name, n) == 0
	if (MATCH("billing", "log_file")) {
		pconfig->log_file = strdup(value);
	} else if (MATCH("billing", "billing_ip")) {
		pconfig->billing_ip = strdup(value);
	} else if (MATCH("billing", "billing_psw")) {
		pconfig->billing_psw = strdup(value);
	} else {
		return 0;  /* unknown section/name, error */
	}
	return 1;
}

int read_config(void* config, const char* config_file_path){
	billing_config*	conf = (billing_config*)config;

	if (ini_parse(config_file_path, read_config_handler, conf) < 0) {
		printf("Can't load '%s'\n", config_file_path);
		return 1;
	}

	printf("Config loaded from '%s': log_file=%s, billing_ip=%s, billing_psw=%s\n",
			config_file_path, conf->log_file, conf->billing_ip, conf->billing_psw);
	return 0;
}


int read_user_list(void* config){
	billing_config*	conf = (billing_config*)config;
	printf("Try read config from http://%s/  (passwd: %s)\n",
			conf->billing_ip, conf->billing_psw);

	int lg, ret;
	char typebuf[70];
	char *data=NULL,*filename=NULL;

	char url[255];
	sprintf(url, "http://%s:8089/index.html", conf->billing_ip);
	char *url_pnt;
	url_pnt = strdup(url);

	ret = http_parse_url(url_pnt, &filename);
	if (ret<0) {
		printf("Error read url!");
		free(url_pnt);
		return ret;
	}

	ret = http_get(filename, &data, &lg, typebuf);
	if (ret<0) {
		printf("Error read data from billing!\n");
		if (data) free(data);
		free(filename);
		free(url_pnt);
		return ret;
	}

	if (ret==200){
        cJSON *json = cJSON_Parse(data);
        if (!json) {
            printf("Error before: [%s]\n", cJSON_GetErrorPtr());
            return -1;
        }

        if (json->type != cJSON_Object){
            printf("Wrong type of json\n");
            return -1;
        }

        if (conf->json) cJSON_Delete(conf->json);
        conf->json = json;
        printf("Ok update JSON statement\n");
	}

	if (data) free(data);
	free(filename);
	free(url_pnt);
}

void test_get_ip(void* config, const char* ip){
    billing_config*	conf = (billing_config*)config;
    cJSON *ip_item = cJSON_GetObjectItem(conf->json, ip);

    if (ip_item){
        char *out;
        out = cJSON_Print(ip_item);
        printf(out);
        printf("\n");
        free(out);
    }
}

int check_permission(void* config, const char* ip, const char* channel_name){
    printf("Abonent %s try open channel %s    -     ", ip, channel_name);
    billing_config*	conf = (billing_config*)config;
    cJSON *ip_item = cJSON_GetObjectItem(conf->json, ip);
    if (! ip_item){
        printf("IP not found!\n");
        return -1;
    }
    int i;
    for (i = 0 ; i < cJSON_GetArraySize(ip_item) ; i++){
        cJSON * subitem = cJSON_GetArrayItem(ip_item, i);
        if (cJSON_strcasecmp(subitem->valuestring, channel_name) == 0){
            printf("Accept!\n");
            return 0;
        }
    }
    printf("Denied!\n");
    return -1;
}
