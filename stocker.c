/*
 * huang hongwen <hhwit@126.com> 2021-09-24
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <time.h>

#define BUFFER_LENGTH	(7 * 5000)
static char **stocks = 0;
static int amount;

#define VOLUME_MAX	150
struct volume_s {
	int index;
	int time_c[VOLUME_MAX];
	int time_s[VOLUME_MAX];
	int volume[VOLUME_MAX];
	int tipped;
};

struct stock_s {
	int open;
	int yesclose;
	int current;
	struct volume_s vol;
};

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

static char *get_original_data(char *stocks_data, char *code)
{
	char *data, *p;
	char key[16];
	memset(key, 0, sizeof(16));
	sprintf(key, "%s%s", code[0]=='6'? "sh":"sz", code);
	p = strstr(stocks_data, key);
	if (!p) return 0;
	data = (char *)malloc(ONE_BUFFER_LENGTH);
	memset(data, 0, ONE_BUFFER_LENGTH);
	memcpy(data, p, 300);

	return data;
}

static int parse_original_data(char *data)
{
	int ret;
	ret = get_commas(data);
	if (ret <= 0) return ret;
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
	char *p = get_original_data(data, code);
	if (!p) return;
	parse_original_data(p);
	free(p);
}

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

static int code_check(char *code)
{
	int i;
	if (strlen(code) != 6) {
		printf("Wrong code length: %s\n", code);
		return -1;
	}
	for (i = 0; i < 6; i ++) {
		if (code[i] < '0' || code[i] > '9') {
			printf("Wrong code number: %s\n", code);
			return -1;
		}
	}
	return 0;
}

static int path_check(char *path)
{
	int i, n;
	char cmd[256], buf[200];
	memset(buf, 0, sizeof(buf));
	sprintf(buf, "%s", path);
	for (i = strlen(buf) - 1; i >= 0; i --) {
		if (buf[i] == '/') {
			buf[i] = 0;
			break;
		}
	}
	if (i <= 0) return 0;
	if (access(buf, F_OK) < 0) {
		printf("Directory does not exist: %s\n", buf);
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "mkdir -p %s", buf);
		system(cmd);
	}
	return 0;
}

static int download_stock(char **code, int num, char *path)
{
	int i;
	char cmd[1024], buf[16];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "wget  -q -O %s http://hq.sinajs.cn/list=", path);
	for (i = 0; i < num; i ++) {
		if (code_check(code[i]) < 0) continue;
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s%s%s", code[i][0] == '6' ? "sh" : "sz",
					code[i],
					i == num - 1 ? "" : ",");
		strcat(cmd, buf);
	}
	system(cmd);
	return 0;
}

static void do_get_list(char *list)
{
	char *fdata;
	fdata = get_list_data(list);
	amount = handle_list_data(fdata);
	stocks = get_all_stocks_code(fdata, amount);
	printf("amount=%d\n", amount);
}

#define TMP_PATH       "tmp/"
#define DOWNLOAD_MAX    100

static char stock_tmp_path[64];

static void do_download_data(void)
{
	int i, n, m;
	time_t timep;
	struct tm *t;
	char systime[32] = {0};
	char saved_path[256];
	char cmd[256];

	path_check(TMP_PATH);
	n = amount / DOWNLOAD_MAX;
	m = amount % DOWNLOAD_MAX;

	time(&timep);
	t = localtime(&timep);
	sprintf(systime, "20%02d.%02d.%02d-%02d.%02d.%02d",
			t->tm_year - 100,
			t->tm_mon + 1,
			t->tm_mday,
			t->tm_hour,
			t->tm_min,
			t->tm_sec);

	for (i = 0; i < n; i ++) {
		memset(saved_path, 0, sizeof(saved_path));
		sprintf(saved_path, "%s/%s-%02d", TMP_PATH, systime, i);
		//printf("saved_path: %s\n", saved_path);
		download_stock(&stocks[i*DOWNLOAD_MAX], DOWNLOAD_MAX, saved_path);
	}
	if (m != 0) {
		memset(saved_path, 0, sizeof(saved_path));
		sprintf(saved_path, "%s/%s-%02d", TMP_PATH, systime, i);
		//printf("saved_path: %s\n", saved_path);
		download_stock(&stocks[i*DOWNLOAD_MAX], m, saved_path);
	}
	memset(stock_tmp_path, 0, sizeof(stock_tmp_path));
	sprintf(stock_tmp_path, "%s%s-all", TMP_PATH, systime);

	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "cat %s/%s* > %s", TMP_PATH, systime, stock_tmp_path);
	system(cmd);
}

static char *stock_file_data_p = 0;

#define STOCKS_DATA_BUFFER_LENGTH	(2 * 1024 * 1024)
static void get_stocks_file_data(char *path)
{
	int fp, ret;
	if (access(path, F_OK) < 0) {
		printf("File does not exit: %s\n", path);
		return;
	}
	fp = open(path, O_RDONLY);
	if (fp < 0) {
		printf("Can't open file: %s\n", path);
		return;
	}
	if (!stock_file_data_p)
		stock_file_data_p = (char *)malloc(STOCKS_DATA_BUFFER_LENGTH);
	ret = read(fp, stock_file_data_p, STOCKS_DATA_BUFFER_LENGTH);
	close(fp);
	if (ret <= 0) return;
}

static struct stock_s *stocks_sp;

static void do_data_init(void)
{
	int i;
	stocks_sp = (struct stock_s *)malloc(sizeof(struct stock_s) * amount);
	if (!stocks_sp) return;
	memset(stocks_sp, 0, sizeof(struct stock_s) * amount);
	do_download_data();
	get_stocks_file_data(stock_tmp_path);
	for (i = 0; i < amount; i ++) {
		do_look_one(stock_file_data_p, stocks[i]);
		stocks_sp[i].open = gopen;
		stocks_sp[i].yesclose = gyesclose;
		printf("%s: %d\n", stocks[i], stocks_sp[i].open);
	}
}

static void do_process(void);

int main(int argc, char *argv[])
{
	do_get_list("list.txt");
	do_data_init();
	while (1) do_process();

	if (stocks[0]) free(stocks[0]);
	if (stocks) free(stocks);
	printf("==== end ====\n");
	return 0;
}

void do_process(void)
{
	int i;
}
