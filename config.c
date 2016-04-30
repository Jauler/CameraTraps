#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <ctype.h>

#include "errors.h"
#include "config.h"

struct config_t
{
	char *line;
	char *key;
	char *value;
	struct config_t *next;
};

static char *CFG_trimLeadingWhitespace(char *str)
{
	if (!str)
		return str;

	while (isspace(*str))
		str++;

	return str;
}

static char *CFG_trimTrailingWhitespace(char *str)
{
	char *tmp = NULL;

	if (!str)
		return str;

	if (strlen(str) <= 0)
		return str;

	tmp = str + strlen(str) - 1;

	while (isspace(*tmp) && tmp >= str){
		*tmp = 0;
		tmp--;
	}

	return str;
}

static struct config_t *CFG_Search(struct config_t *cfg, char *key)
{
	struct config_t *tmp;
	for (tmp = cfg; tmp != NULL; tmp = tmp->next)
		if (strcmp(tmp->key, key) == 0)
			return tmp;

	return NULL;
}

struct config_t *CFG_ParseConfigFile(char *filename)
{
	FILE *f = NULL;
	struct config_t head_cfg = {NULL, NULL, NULL, NULL};
	struct config_t *last_cfg = &head_cfg;

	f = fopen(filename, "r");
	if (!f){
		WARN(errno, "Error reading config file");
		return NULL;
	}

	char *line = NULL;
	char *key = NULL;
	char *value = NULL;
	size_t len = 0;
	int line_count = 0;

	while (getline(&line, &len, f) != -1)
	{
		line_count++;

		// Allow for comments
		char *keyvalue = strtok(line, "#");
		if (!keyvalue || *line == '#')
			goto ERR_CONTINUE;

		// seperate keys and values
		key = strtok(keyvalue, "=");
		value = strtok(NULL, "=");

		//Do not allow multiple equalities
		if (strtok(NULL, "=") != NULL) {
			WARN(EINVAL, "%s:%d: Multiple equality signs", filename, line_count);
			goto ERR_CONTINUE;
		}

		// remove leading/trailing whitespaces
		key = CFG_trimLeadingWhitespace(key);
		value = CFG_trimLeadingWhitespace(value);
		key = CFG_trimTrailingWhitespace(key);
		value = CFG_trimTrailingWhitespace(value);
		if (!key)
			goto ERR_CONTINUE;
		if (*key == '\0')
			goto ERR_CONTINUE;

		//allocate key-value instance
		struct config_t *cfg = NULL;
		last_cfg->next = malloc(sizeof(struct config_t));
		cfg = last_cfg->next;
		if (!cfg){
			WARN(ENOMEM, "Error while parsing config file: %s\n", strerror(ENOMEM));
			CFG_destroy(head_cfg.next);
			return NULL;
		}

		//strore key-value
		cfg->line = line;
		cfg->key = key;
		cfg->value = value;
		cfg->next = NULL;
		last_cfg = cfg;
		goto SUCC_CONTINUE;

ERR_CONTINUE:
		free(line);

SUCC_CONTINUE:
		line = NULL;
		len = 0;
		key = NULL;
		value = NULL;
	}
	free(line);

	fclose(f);
	return head_cfg.next;
}

int CFG_IsSet(struct config_t *cfg, char *key)
{
	return CFG_Search(cfg, key) != NULL;
}

char *CFG_GetValue(struct config_t *cfg, char *key)
{
	struct config_t *value = CFG_Search(cfg, key);

	if (!value)
		return NULL;

	return value->value;
}

void CFG_destroy(struct config_t *cfg)
{
	struct config_t *tmp;

	while (cfg != NULL)
	{
		tmp = cfg->next;
		free(cfg->line);
		free(cfg);
		cfg = tmp;
	}

	return;
}

