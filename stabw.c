/*
 * huang hongwen <hhwit@126.com> 2021-07-21
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "cJSON.h"


static int gopen = 0;
static int gclose = 0;
static int ghigh = 0;
static int glow = 0;
static long long gvolume = 0;

static char *comma[64];
static int cnum;

static char stock_weeks_data[512];

static int is_one_stab(void);

static int string_check(char *s)
{
	int i, l;
	l = strlen(s);
	if (l < 1 || l > 16) {
		printf("Wrong string length: %s\n", s);
		return -1;
	}
	for (i = 0; i < l; i ++) {
		if ((s[i] >= '0' && s[i] <= '9') || s[i] == '.') {
			continue;
		} else {
			printf("Wrong string formate: %s\n", s);
			return -1;
		}
	}
	return 0;
}

static long long str_to_decimal(int order, char s)
{
	long long a = 0;
	long long b = s - '0';
	switch (order) {
	case 1:
		a = b * 100;
		break;
	case 2:
		a = b * 1000;
		break;
	case 3:
		a = b * 10000;
		break;
	case 4:
		a = b * 100000;
		break;
	case 5:
		a = b * 1000000;
		break;
	case 6:
		a = b * 10000000;
		break;
	case 7:
		a = b * 100000000;
		break;
	case 8:
		a = b * 1000000000;
		break;
	case 9:
		a = b * 10000000000;
		break;
	case 10:
		a = b * 100000000000;
		break;
	case 11:
		a = b * 1000000000000;
		break;
	case 12:
		a = b * 10000000000000;
		break;
	case 13:
		a = b * 100000000000000;
		break;
	case 14:
		a = b * 1000000000000000;
		break;
	case 15:
		a = b * 10000000000000000;
		break;
	case 16:
		a = b * 100000000000000000;
		break;
	default:
		printf("Wrong number formate: %d\n", order);
	}
	return a;
}

static long long string_to_int(char *s)
{
	char *p;
	long long a;
	int i, j, n, m;
	int l = strlen(s);
	if (l <= 0) return -1;
	if (string_check(s) < 0) return -1;
	p = strstr(s, ".");
	if (p) {
		n = p - s;
		m = l - n - 1;
	} else {
		n = l;
		m = 0;
	}
	for (i = 0, j = n, a = 0; i < n; i ++, j --) {
		a += str_to_decimal(j, s[i]);
	}
	if (m >= 2)
		a += (*(p + 1) - '0') * 10 + *(p + 2) - '0';
	else if (m == 1)
		a += *(p + 1) - '0';
	return a;
}

static void do_look_one(int index)
{
	char *string;
	cJSON *root, *item, *target;
	gopen = 0; gclose = 0;
	ghigh = 0; glow = 0;
	gvolume = 0;

	root = cJSON_Parse(stock_weeks_data);
	if (cJSON_GetArraySize(root) <= 0) {
		cJSON_Delete(root);
		return;
	}
	item = cJSON_GetArrayItem(root, index);

	target = cJSON_GetObjectItem(item, "open");
	string = cJSON_GetStringValue(target);
	gopen = string_to_int(string);

	target = cJSON_GetObjectItem(item, "close");
	string = cJSON_GetStringValue(target);
	gclose = string_to_int(string);

	target = cJSON_GetObjectItem(item, "high");
	string = cJSON_GetStringValue(target);
	ghigh = string_to_int(string);

	target = cJSON_GetObjectItem(item, "low");
	string = cJSON_GetStringValue(target);
	glow = string_to_int(string);

	target = cJSON_GetObjectItem(item, "volume");
	string = cJSON_GetStringValue(target);
	gvolume = string_to_int(string);

	cJSON_Delete(root);
}

#define BUFFER_LENGTH	(7 * 5000)
static char **stocks;
static int amount;

static char *get_list_data(char *path)
{
	int fp, ret;
	char *data;
	if (access(path, F_OK) < 0) {
		printf("List does not exit: %s\n", path);
		return 0;
	}
	fp = open(path, O_RDONLY);
	if (fp < 0) {
		printf("Can't open list: %s\n", path);
		return 0;
	}
	data = (char *)malloc(BUFFER_LENGTH);
	memset(data, 0, BUFFER_LENGTH);
	ret = read(fp, data, BUFFER_LENGTH);
	if (ret <= 0) return 0;
	return data;
}

static int handle_list_data(char *data)
{
	int i, j, c = 0;
	char *tmp;
	tmp = (char *)malloc(BUFFER_LENGTH);
	memcpy(tmp, data, BUFFER_LENGTH);
	memset(data, 0, BUFFER_LENGTH);
	for (i = 0, j = 0; i < BUFFER_LENGTH; i ++) {
		if (tmp[i] >= '0' && tmp[i] <= '9') {
			data[j] = tmp[i];
			j ++;
		} else {
			if (data[j - 1] != 0) {
				j ++;
				c ++;
			}
		}
	}
	free(tmp);
	return c;
}

static char **get_all_stocks_code(char *d, int n)
{
	int i;
	char **s;
	s = (char **)malloc(n * sizeof(char *));
	for (i = 0; i < n; i ++) {
		s[i] = i * 7 + d;
	}
	return s;
}

static void do_get_list(void)
{
	char *fdata;
	fdata = get_list_data("list.all");
	amount = handle_list_data(fdata);
	stocks = get_all_stocks_code(fdata, amount);
}

static int get_stock_weeks_data(char *code)
{
	int fp, ret;
	char *data;
	char path[128];
	memset(path, 0, sizeof(path));
	sprintf(path, "dataw/%s", code);
	if (access(path, F_OK) < 0) {
		printf("File does not exit: %s\n", path);
		return -1;
	}
	fp = open(path, O_RDONLY);
	if (fp < 0) {
		printf("Can't open file: %s\n", path);
		return -1;
	}
	memset(stock_weeks_data, 0, sizeof(stock_weeks_data));
	ret = read(fp, stock_weeks_data, sizeof(stock_weeks_data));
	close(fp);
	return ret;
}

static void do_all_stab(void)
{
	int i, c = 0;
	do_get_list();
	if (amount <= 0) {
		printf("No stock found\n");
		return;
	}
	for (i = 0; i < amount; i ++) {
		get_stock_weeks_data(stocks[i]);
		if (is_one_stab()) {
			printf("%s\n", stocks[i]);
			c ++;
		}
	}
	printf("Found: %d\n", c);
}

int main(int argc, char *argv[])
{
	//if (argc < 2) {
	//	printf("Please input week index\n");
	//	return 0;
	//}
	do_get_list();
	printf("amount:%d\n", amount);
	do_all_stab();

	printf("==== end ====\n");
	return 0;
}

static int is_one_stab(void)
{
	int up1, obj1, down1, total1;
	int up2, obj2, down2, total2;
	int h1, h2, o1, o2, c1, c2, l1, l2;

	do_look_one(0);
	if (gopen <= 0
		|| gclose <= 0
		|| ghigh <= 0
		|| glow <= 0) return 0;
	h1 = ghigh;
	o1 = gopen;
	c1 = gclose;
	l1 = glow;
	if (o1 <= c1) return 0;
	total1 = h1 - l1;
	up1 = h1 - o1;
	if (up1 > total1 * 40 / 100) return 0;
	obj1 = o1 - c1;
	if (obj1 < c1 * 1.5 / 100) return 0;
	down1 = c1 - l1;

	do_look_one(1);
	if (gopen <= 0
		|| gclose <= 0
		|| ghigh <= 0
		|| glow <= 0) return 0;
	h2 = ghigh;
	o2 = gopen;
	c2 = gclose;
	l2 = glow;
	if (c2 < o2) return 0;
	total2 = h2 - l2;
	up2 = h2 - c2;
	if (up2 > total2 * 40 / 100) return 0;
	obj2 = c2 - o2;
	if (obj2 < o2 * 1 / 100) return 0;
	down2 = o2 - l2;
	if (down2 > total2 * 40 / 100) return 0;

	//if (o2 > c1 + (obj1 / 5)) return 0;
	//if (c2 > o1 + (obj1 / 5)) return 0;

	return 1;
}
