#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ONE_BUFFER_LENGTH	(1024)

static char *stock_data = 0;
static int stock_data_length = 0;

static char *get_original_data(char *code, char *dir)
{
	int fp, ret;
	char path[128];
	char *data = stock_data;
	memset(path, 0, sizeof(path));
	sprintf(path, "%s/%s", dir, code);
	if (access(path, F_OK) < 0) {
		printf("File does not exit: %s\n", path);
		return 0;
	}
	fp = open(path, O_RDONLY);
	if (fp < 0) {
		printf("Can't open file: %s\n", path);
		return 0;
	}
	if (!data) {
		data = (char *)malloc(ONE_BUFFER_LENGTH);
		stock_data = data;
	}
	memset(data, 0, ONE_BUFFER_LENGTH);
	ret = read(fp, data, ONE_BUFFER_LENGTH);
	close(fp);
	if (ret <= 0) return 0;
	stock_data_length = ret;
	return data;
}

static int do_merge_one(int fp, char *code, char *dir)
{
	char *p = get_original_data(code, dir);
	if (!p) return -1;
	return write(fp, p, stock_data_length);
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
	close(fp);
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

static int create_merge_file(char *date)
{
	int fp;
	char path[32];
	memset(path, 0, sizeof(path));
	sprintf(path, "%s", date);
	fp = open(path, O_RDWR | O_CREAT, 0755);
	if (fp <= 0)
		printf("Can't open %s\n", path);
	return fp;
}

static void do_merge(char *src, char *des)
{
	int fp;
	int i, c = 0;
	do_get_list();
	if (amount <= 0) {
		printf("No stock found\n");
		return;
	}
	fp = create_merge_file(des);
	if (fp <= 0) return;
	for (i = 0; i < amount; i ++) {
		if (do_merge_one(fp, stocks[i], src) > 0) {
			c ++;
		}
	}
	close(fp);
	printf("Merged: %d\n", c);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("Please input src and des\n");
		return 0;
	}
	do_merge(argv[1], argv[2]);

	if (stocks[0]) free(stocks[0]);
	if (stocks) free(stocks);
	printf("==== end ====\n");
	return 0;
}
