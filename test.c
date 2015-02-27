#include <billing.c>

#include <stdio.h>

int main(int argc, char *argv[]){
    printf("Started...\n");
    billing_config *conf = billing_open_instance("test_/test_config", NULL);
    //test_get_ip(conf, "127.0.0.1");
    int res = check_permission(conf, "127.0.0.1", "one");
    res = check_permission(conf, "127.0.0.1", "two");
    res = check_permission(conf, "127.0.0.1", "NTV");
    res = check_permission(conf, "127.0.0.3", "NTV");

    billing_close_instance(conf);
    return 0;
}
