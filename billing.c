#include <glib.h>

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <time.h>

#include <billing.h>

#include "ini.c"
#include "http_lib.c"
#include "cJSON.c"

typedef struct {
	char *billing_ip;
	char *billing_psw;
	char *log_file;
	FILE *fp_log;
	cJSON *json;
	int have_json;
	cJSON *json_new;
	int have_new_json;
	int loader_pth;
	time_t timer;
	pthread_t pth;
} billing_config;

void* read_user_list(void* config);

int	billing_verify_action(void* instance, billing_action* action){
	billing_config*	conf = (billing_config*)instance;

	if (conf->loader_pth == 0) {
		if (conf->have_new_json == 1) {
			if (conf->have_json == 1) cJSON_Delete(conf->json);
			conf->json = conf->json_new;
			conf->have_json = 1;
			conf->have_new_json = 0;
		}

		time_t delta = (time(NULL) - conf->timer) / 60;


//		fprintf(conf->fp_log, "Time delta = %ld min.\n", delta);
		if (delta > 5) {
			fprintf(conf->fp_log, "Start download access list in thread...\n");
			conf->loader_pth = 1;
			pthread_create(&conf->pth, NULL, read_user_list, conf);
		}
	}


	fprintf(conf->fp_log, "Action %d by %s channel %s\n", (int)action->event, action->ip, action->url);

	if ((int)action->event != 7){
		return 1;
	}


	if (conf->have_json == 1){
		//check is user can play this
		return check_permission(instance, action->ip, action->url);
	}

	fprintf(conf->fp_log, "WARNING!!! No json loaded! Default policy accept.\n");
	return 1;

}


void* billing_open_instance(const char* config_file_path, void* glib_main_loop){
	billing_config*	conf = malloc(sizeof(billing_config));
	conf->fp_log = fopen("/tmp/billing-stup_preloading.log", "a+");
	setbuf(conf->fp_log, NULL);
	int res = read_config(conf, config_file_path);
	if (res != 0) {
		fclose(conf->fp_log);
		free(conf);
		return NULL;
	}
	fclose(conf->fp_log);
	conf->fp_log = fopen(conf->log_file, "a+");
	if (conf->fp_log == NULL){
		free(conf);
		return NULL;
	}
	setbuf(conf->fp_log, NULL);

	conf->have_json = 0;
	conf->have_new_json = 0;
	conf->loader_pth = 1;
	pthread_create(&conf->pth, NULL, &read_user_list, conf);
	conf->json = NULL;
	return conf;
}


void billing_close_instance(void* instance) {
	billing_config *conf = (billing_config *) instance;
	if (conf->loader_pth) {
		pthread_cancel(conf->pth);
		pthread_join(conf->pth, NULL);
	}
	if (conf->json) cJSON_Delete(conf->json);
	fclose(conf->fp_log);
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

char* concat(const char *s1, const char *s2)
{
    size_t len1 = strlen(s1);
    size_t len2 = strlen(s2);
    char *result = malloc(len1+len2+1);//+1 for the zero-terminator
    //in real code you would check for errors in malloc here
    memcpy(result, s1, len1);
    memcpy(result+len1, s2, len2+1);//+1 to copy the null-terminator
    return result;
}

int read_config(void* config, const char* config_file_path){
	billing_config*	conf = (billing_config*)config;
    fprintf(conf->fp_log, "Try load config...\n");
    char* config_file_path_new = concat(config_file_path, "/billing-stub.conf");

	if (ini_parse(config_file_path_new, read_config_handler, conf) < 0) {
		fprintf(conf->fp_log, "Can't load '%s'\n", config_file_path_new);
		free(config_file_path_new);
		return -1;
	}

	fprintf(conf->fp_log, "Config loaded from '%s': log_file=%s, billing_ip=%s, billing_psw=%s\n",
			config_file_path_new, conf->log_file, conf->billing_ip, conf->billing_psw);
    free(config_file_path_new);
	return 0;
}


void* read_user_list(void* config){
	billing_config*	conf = (billing_config*)config;

	int lg, ret;
	char typebuf[70];
	char *data=NULL,*filename=NULL;

	char url[1255];
	sprintf(url, "http://%s:8082/system_api/?model=Collector&method1=collector_manager.collector_iptv_accept_list_get&arg1={}&context=collector&psw=%s", conf->billing_ip, conf->billing_psw);
	fprintf(conf->fp_log, "Try read access list from %s  (passwd: %s)\n",
			url, conf->billing_psw);
	char *url_pnt;
	url_pnt = strdup(url);

	ret = http_parse_url(url_pnt, &filename);
	if (ret<0) {
		fprintf(conf->fp_log, "Error read url!");
		free(url_pnt);
		conf->loader_pth = 0;
		conf->timer = time(NULL);
		return;
//		return ret;
	}

	ret = http_get(filename, &data, &lg, typebuf);
	if (ret<0) {
		fprintf(conf->fp_log, "Error read data from billing! errno=%d\n", ret);
		if (data) free(data);
		free(filename);
		free(url_pnt);
		conf->loader_pth = 0;
		conf->timer = time(NULL);
		return;
//		return ret;
	}

	if (ret==200){
		fprintf(conf->fp_log, "Data:\n\n%s\n\n\n", data);
        cJSON *json = cJSON_Parse(data);
        if (!json) {
            fprintf(conf->fp_log, "Error before: [%s]\n", cJSON_GetErrorPtr());
			conf->loader_pth = 0;
			conf->timer = time(NULL);
			return;
//		return -1;
        }

        if (json->type != cJSON_Object){
            fprintf(conf->fp_log, "Wrong type of json\n");
			conf->loader_pth = 0;
			conf->timer = time(NULL);
			return;
//		return -1;
        }

        conf->json_new = json;
		conf->have_new_json = 1;

        fprintf(conf->fp_log, "Ok update JSON statement\n");
	} else {
		fprintf(conf->fp_log, "Fail http get with code %d\n", ret);
	}

	if (data) free(data);
	free(filename);
	free(url_pnt);
	conf->loader_pth = 0;
	conf->timer = time(NULL);
}

int check_permission(void* config, const char* ip, const char* channel_name){
    billing_config*	conf = (billing_config*)config;
    fprintf(conf->fp_log, "Abonent %s try open channel %s    -     ", ip, channel_name);
    cJSON *ip_item = cJSON_GetObjectItem(conf->json, ip);
    if (! ip_item){
        fprintf(conf->fp_log, "IP not found!\n");
        return 0;
    }
    int i;
    for (i = 0 ; i < cJSON_GetArraySize(ip_item) ; i++){
        cJSON * subitem = cJSON_GetArrayItem(ip_item, i);
        if (cJSON_strcasecmp(subitem->valuestring, channel_name) == 0){
            fprintf(conf->fp_log, "Accept!\n");
            return 1;
        }
    }
    fprintf(conf->fp_log, "Denied!\n");
    return 0;
}
