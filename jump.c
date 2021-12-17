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

static long long string_to_int(char *s)
{
	char *p;
	long long a = 0;
	int i, j, n, m;
	int l = strlen(s);
	if (l <= 0) return 0;
	if (string_check(s) < 0) return 0;
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

struct gap_s {
	int high;
	int low;
};

static struct gap_s m_gaps[120];
static int m_gaps_len = 0;

static char *m_day_data = 0;

static int get_day_file_data(char *code)
{
	int fp, ret;
	char *data;
	char path[128];
	memset(path, 0, sizeof(path));
	sprintf(path, "data2/data2/%s", code);
	if (access(path, F_OK) < 0) {
		printf("File does not exit: %s\n", path);
		return -1;
	}
	fp = open(path, O_RDONLY);
	if (fp < 0) {
		printf("Can't open file: %s\n", path);
		return -1;
	}
	#define WEEK_FILE_LEN	20 * 1024
	if (m_day_data == 0) 
		m_day_data = malloc(WEEK_FILE_LEN);
	memset(m_day_data, 0, WEEK_FILE_LEN);
	ret = read(fp, m_day_data, WEEK_FILE_LEN);
	close(fp);
	return ret;
}

static cJSON *m_root;
static int m_start_index, m_end_index;
static int gaps_init(char *code, char *start, char *end)
{
	char *string;
	int ret, n, i;
	cJSON *root, *item, *target;

	ret = get_day_file_data(code);
	if (ret <= 0) return -1;
	root = cJSON_Parse(m_day_data);
	n = cJSON_GetArraySize(root);
#define GAP_DAYS_MIN	3
	if (n < GAP_DAYS_MIN) {
		cJSON_Delete(root);
		return -1;
	}
	m_start_index = -1;
	m_end_index = -1;
	for (i = 0; i < n; i ++) {
		item = cJSON_GetArrayItem(root, i);
		target = cJSON_GetObjectItem(item, "day");
		string = cJSON_GetStringValue(target);
		if (string == 0) continue;
		if (strcmp(string, start) == 0) {
			if (n - i < GAP_DAYS_MIN)
				return -1;
			else
				m_start_index = i;
		}
	}
	if (m_start_index < -1) m_start_index = 0;
	for (i = n - 1; i >= 0; i --) {
		item = cJSON_GetArrayItem(root, i);
		target = cJSON_GetObjectItem(item, "day");
		string = cJSON_GetStringValue(target);
		if (string == 0) continue;
		if (strcmp(string, end) == 0)
			m_end_index = i;
	}
	if (m_end_index < 0) return -1;
	if (m_end_index - m_start_index + 1 < GAP_DAYS_MIN) return -1;
	m_root = root;

	return 0;
}

static int ghigh = 0;
static int glow = 0;

static long long get_element(int index, char *key)
{
	char *string;
	cJSON *item, *target;
	item = cJSON_GetArrayItem(m_root, index);
	target = cJSON_GetObjectItem(item, key);
	string = cJSON_GetStringValue(target);
	return string_to_int(string);
}

static void get_high(int index)
{
	ghigh = get_element(index, "high");
}

static void get_low(int index)
{
	glow = get_element(index, "low");
}

static void do_look_one(int index)
{
	get_high(index);
	get_low(index);
}

static int gaps_parse(void)
{
	int i;
	int h1, l1, h2, l2;
	for (i = m_end_index - 1; i >= m_start_index; i --) {
		do_look_one(i);
		h1 = ghigh; l1 = glow;
		do_look_one(i + 1);
		h2 = ghigh; l2 = glow;
		if (l2 > h1) {
			m_gaps[m_gaps_len].high = l2;
			m_gaps[m_gaps_len].low = h1;
			m_gaps_len ++;
		}
	}
	return 0;
}

static int gaps_fill(void)
{
	int i, j;
	int h1, l1;
	for (i = m_end_index; i >= m_start_index + 2; i --) {
		do_look_one(i);
		h1 = ghigh; l1 = glow;
		for (j = 0; j < m_gaps_len; j ++) {
			if (m_gaps[j].high == 0) continue;
			if (m_gaps[j].low == 0) continue;
			if (h1 <= m_gaps[j].low || l1 >= m_gaps[j].high)
				continue;
			if (h1 >= m_gaps[j].high) {
				if (l1 > m_gaps[j].low) {
					m_gaps[j].high = l1;
				} else {
					m_gaps[j].high = 0;
					m_gaps[j].low = 0;
				}
			} else {
				m_gaps[j].high = h1;
			}
		}
	}
}

static void gaps_deinit(void)
{
	cJSON_Delete(m_root);
}

static void print_gaps(void)
{
	int i;
	for (i = 0; i < m_gaps_len; i ++) {
		printf("%02d: %d  %d\n", i, m_gaps[i].high, m_gaps[i].low);
	}
}

static int is_one_jump(void)
{
	gaps_parse();
	if (m_gaps_len <= 0) return 0;
	printf("m_gaps_len=%d\n", m_gaps_len);
	print_gaps();
	gaps_fill();
	print_gaps();
	return 1;
}

static void do_all_jumps(char *start, char *end)
{
	int i, c = 0;
	for (i = 0; i < amount; i ++) {
		if (gaps_init(stocks[i], start, end) < 0)
			continue;
		if (is_one_jump()) {
			printf("%s\n", stocks[i]);
			c ++;
		}
		gaps_deinit();
	}
	printf("Found: %d\n", c);
}

int main(int argc, char *argv[])
{
	if (argc < 3) {
		printf("Please input start day & end day\n");
		return 0;
	}
	do_get_list();
	printf("amount=%d\n", amount);

	do_all_jumps(argv[1], argv[2]);

	if (amount) free(stocks);
	printf("==== end ====\n");
	return 0;
}
