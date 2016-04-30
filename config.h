#ifndef CONFIG_H_INCLUDED
#define CONFIG_H_INCLUDED

struct config_t;

struct config_t *CFG_ParseConfigFile(char *filename);

int CFG_IsSet(struct config_t *cfg, char * key);
char * CFG_GetValue(struct config_t *cfg, char * key);

void CFG_destroy(struct config_t *cfg);

#endif
