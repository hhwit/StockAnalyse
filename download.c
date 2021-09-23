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
	//printf("n=%d\n", n * sizeof(int));
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
	sprintf(cmd, "wget  -O %s http://hq.sinajs.cn/list=", path);
	//printf("cmd: %s\n", cmd);
	for (i = 0; i < num; i ++) {
		if (code_check(code[i]) < 0) continue;
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s%s%s", code[i][0] == '6' ? "sh" : "sz",
					code[i],
					i == num - 1 ? "" : ",");
		//printf("buf: %s\n", buf);
		strcat(cmd, buf);
	}
	//printf("cmd:%s\n", cmd);
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

static void do_download(char *path)
{
	int i, n, m;
	time_t timep;
	struct tm *t;
	char systime[32] = {0};
	char saved_path[256];
	char cmd[256];
	time(&timep);
	t = localtime(&timep);
	sprintf(systime, "20%02d.%02d.%02d-%02d.%02d.%02d",
			t->tm_year - 100,
			t->tm_mon + 1,
			t->tm_mday,
			t->tm_hour,
			t->tm_min,
			t->tm_sec);
	#define TMP_PATH       "tmp/"
	#define DOWNLOAD_MAX    100
	path_check(TMP_PATH);
	n = amount / DOWNLOAD_MAX;
	m = amount % DOWNLOAD_MAX;
	for (i = 0; i < n; i ++) {
		memset(saved_path, 0, sizeof(saved_path));
		sprintf(saved_path, "%s/%s-%02d", TMP_PATH, systime, i);
		printf("saved_path: %s\n", saved_path);
		download_stock(&stocks[i*DOWNLOAD_MAX], DOWNLOAD_MAX, saved_path);
	}
	if (m != 0) {
		memset(saved_path, 0, sizeof(saved_path));
		sprintf(saved_path, "%s/%s-%02d", TMP_PATH, systime, i);
		printf("saved_path: %s\n", saved_path);
		download_stock(&stocks[i*DOWNLOAD_MAX], m, saved_path);
	}
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "cat %s/%s* > %s", TMP_PATH, systime, path);
	system(cmd);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Please input path for save\n");
		return 0;
	}
	do_get_list("list.all");
	path_check(argv[1]);
	do_download(argv[1]);

	if (stocks[0]) free(stocks[0]);
	if (stocks) free(stocks);
	printf("==== end ====\n");
	return 0;
}
