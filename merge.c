#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

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

#define STOCKS_DATA_BUFFER_LENGTH	(2 * 1024 * 1024)
static char *get_stocks_data(char *date)
{
	int fp, ret;
	char *data;
	char path[128];
	memset(path, 0, sizeof(path));
	sprintf(path, "data/%s", date);
	if (access(path, F_OK) < 0) {
		printf("File does not exit: %s\n", path);
		return 0;
	}
	fp = open(path, O_RDONLY);
	if (fp < 0) {
		printf("Can't open file: %s\n", path);
		return 0;
	}
	data = (char *)malloc(STOCKS_DATA_BUFFER_LENGTH);
	ret = read(fp, data, STOCKS_DATA_BUFFER_LENGTH);
	close(fp);
	if (ret <= 0) return 0;
	return data;
}

#define ONE_BUFFER_LENGTH	(1024)

static int gopen = 0;
static int gyesclose = 0;
static int gtoclose = 0;
static int ghigh = 0;
static int glow = 0;
static long long gvolume = 0;
static long long gturnover = 0;
static char *comma[64];
static int cnum;

static int get_commas(char *data)
{
	int i;
	int l = strlen(data);
	if (l <= 0) {
		printf("Wrong data length: %s\n", data);
		return -1;
	}
	cnum = 0;
	for (i = 0; i < l; i ++) {
		if (data[i] == ',') {
			comma[cnum] = &data[i];
			cnum ++;
		}
	}
	return cnum;
}

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

static int get_open(void)
{
	char buf[32];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, comma[0] + 1, comma[1] - comma[0] - 1);
	gopen = string_to_int(buf);
	return gopen;
}

static int get_yesclose(void)
{
	char buf[32];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, comma[1] + 1, comma[2] - comma[1] - 1);
	gyesclose = string_to_int(buf);
	return gyesclose;
}

static int get_toclose(void)
{
	char buf[32];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, comma[2] + 1, comma[3] - comma[2] - 1);
	gtoclose = string_to_int(buf);
	return gtoclose;
}

static int get_high(void)
{
	char buf[32];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, comma[3] + 1, comma[4] - comma[3] - 1);
	ghigh = string_to_int(buf);
	return ghigh;
}

static int get_low(void)
{
	char buf[32];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, comma[4] + 1, comma[5] - comma[4] - 1);
	glow = string_to_int(buf);
	return glow;
}

static long long get_volume(void)
{
	char buf[32];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, comma[7] + 1, comma[8] - comma[7] - 1);
	gvolume = string_to_int(buf);
	return gvolume;
}

static long long get_turnover(void)
{
	char buf[32];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, comma[8] + 1, comma[9] - comma[8] - 1);
	gturnover = string_to_int(buf);
	return gturnover;
}

static char *get_one_stock_original_data(char *stocks_data, char *code)
{
	char *data, *p;
	char key[16];
	memset(key, 0, sizeof(16));
	sprintf(key, "%s%s", code[0]=='6'? "sh":"sz", code);
	p = strstr(stocks_data, key);
	if (!p) return 0;
	if (*(p + 10) == '"') return 0;
	data = (char *)malloc(ONE_BUFFER_LENGTH);
	memset(data, 0, ONE_BUFFER_LENGTH);
	memcpy(data, p, 300);

	return data;
}

static int parse_original_data(char *data)
{
	int ret;
	ret = get_commas(data);
	if (ret < 8) return -1;
	get_open();
	get_yesclose();
	get_toclose();
	get_high();
	get_low();
	get_volume();
	get_turnover();
	return ret;
}

static void do_look_one(char *data, char *code)
{
	char *p = 0;
	gopen = 0; gyesclose = 0; gtoclose = 0;
	ghigh = 0; glow = 0;
	gvolume = 0; gturnover = 0;
	
	p = get_one_stock_original_data(data, code);
	if (!p) return;
	parse_original_data(p);
	free(p);
}

static char merge_s[256];
//{"day":"2019-06-28","open":"12.090","high":"12.130","low":"11.510","close":"11.680","volume":"229070858"},
static void gen_merge_string(char *date)
{
	memset(merge_s, 0, sizeof(merge_s));
	sprintf(merge_s, ",{\"day\":\"%s\",\"open\":\"%d.%d0\",\"high\":\"%d.%d0\",\"low\":\"%d.%d0\",\"close\":\"%d.%d0\",\"volume\":\"%lld\"}]",
		date,
		gopen / 100, gopen % 100,
		ghigh / 100, ghigh % 100,
		glow / 100, glow % 100,
		gtoclose / 100, gtoclose % 100,
		gvolume / 100);
	//printf("%s\n", merge_s);
}

static int do_merge_one(char *code)
{
	int fp, ret;
	char path[32];
	memset(path, 0, sizeof(path));
	sprintf(path, "data2/data2/%s", code);
	if (access(path, F_OK) < 0) return -1;
	fp = open(path, O_RDWR);
	if (fp < 0) return -1;
	ret = lseek(fp, -1, SEEK_END);
	if (ret < 0) {
		close(fp);
		return -1;
	}
	ret = write(fp, merge_s, strlen(merge_s));
	close(fp);
	return 0;
}

static void do_merge_all(char *data, char *day)
{
	int i;
	for (i = 0; i < amount; i ++) {
		do_look_one(data, stocks[i]);
		if (gopen == 0) continue;
		gen_merge_string(day);
		do_merge_one(stocks[i]);
	}
}

int main(int argc, char *argv[])
{
	char *date = 0;

	if (argc < 2) {
		printf("Please input day\n");
		return 0;
	}

	// get all stocks list
	do_get_list();
	printf("amount=%d\n", amount);

	// get date data for merge
	date = get_stocks_data(argv[1]);
	do_merge_all(date, argv[1]);

	if (amount) free(stocks);
	if (date) free(date);

	printf("==== end ====\n");
	return 0;
}
