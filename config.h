#ifndef ZOS_CONFIG_H
#define ZOS_CONFIG_H

typedef struct {
    int timezone;
    int klayout;
} ZOSConfig;

ZOSConfig zconfig = {
    .timezone = 1,
    .klayout = 1,
};

#endif // ZOS_CONFIG_H
