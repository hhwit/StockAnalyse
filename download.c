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

static int date_check(char *date)
{
	char cmd[32];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "data/%s", date);
	if (access(cmd, F_OK) < 0) {
		printf("Directory does not exist: %s\n", date);
		memset(cmd, 0, sizeof(cmd));
		sprintf(cmd, "mkdir -p data/%s", date);
		system(cmd);
	}
	return 0;
}

static int download_stock(char *code, char *date)
{
	char cmd[128];
	if (code_check(code) < 0)
		return -1;
	if (date_check(date) < 0)
		return -1;
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, 
		"wget -O data/%s/%s http://hq.sinajs.cn/list=%s%s",
		date,
		code,
		code[0]=='6'? "sh" : "sz",
		code);
	system(cmd);
	return 0;
}

static void do_get_list(void)
{
	char *fdata;
	fdata = get_list_data("list");
	amount = handle_list_data(fdata);
	stocks = get_all_stocks_code(fdata, amount);
	printf("amount=%d\n", amount);
}

static void do_download(char *date)
{
	int i;
	for (i = 0; i < amount; i ++) {
		//printf("i=%d\n", i);
		download_stock(stocks[i], date);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Please input date\n");
		return 0;
	}
	do_get_list();
	do_download(argv[1]);

	printf("==== end ====\n");
	return 0;
}
