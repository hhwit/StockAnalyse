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
	int index;
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
	m_start_index = 0;
	m_end_index = 0;
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
	for (i = n - 1; i >= 0; i --) {
		item = cJSON_GetArrayItem(root, i);
		target = cJSON_GetObjectItem(item, "day");
		string = cJSON_GetStringValue(target);
		if (string == 0) continue;
		if (strcmp(string, end) == 0)
			m_end_index = i;
	}
	if (m_end_index <= 0) return -1;
	if (m_end_index - m_start_index + 1 < GAP_DAYS_MIN) return -1;
	m_root = root;

	return 0;
}

static int ghigh = 0;
static int glow = 0;
static int gopen = 0;
static int gclose = 0;

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

static void get_open(int index)
{
	gopen = get_element(index, "open");
}

static void get_close(int index)
{
	gclose = get_element(index, "close");
}

static void do_look_one(int index)
{
	get_high(index);
	get_low(index);
	get_open(index);
	get_close(index);
}

static int gaps_parse(void)
{
	int i;
	int h1, l1, h2, l2;
	m_gaps_len = 0;
	for (i = m_end_index - 2; i >= m_start_index; i --) {
		do_look_one(i);
		h1 = ghigh; l1 = glow;
		do_look_one(i + 1);
		h2 = ghigh; l2 = glow;
		if (l2 > h1) {
			m_gaps[m_gaps_len].index = i;
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
			//printf("h:%d l:%d\n", h1, l1);
			if (m_gaps[j].high == 0) continue;
			//printf("%d\n", __LINE__);
			if (m_gaps[j].low == 0) continue;
			//printf("%d\n", __LINE__);
			if (i <= m_gaps[j].index + 1) continue;
			//printf("%d\n", __LINE__);
			if (l1 >= m_gaps[j].high)
				continue;
			//printf("%d\n", __LINE__);
			if (h1 <= m_gaps[j].low) {
				m_gaps[j].high = 0;
				m_gaps[j].low = 0;
				continue;
			}
			//printf("%d\n", __LINE__);
			if (h1 >= m_gaps[j].high) {
				if (l1 > m_gaps[j].low) {
					m_gaps[j].high = l1;
				} else {
					m_gaps[j].high = 0;
					m_gaps[j].low = 0;
				}
			} else {
				m_gaps[j].low = h1;
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

static int m_gap_distance = 0;
static int m_gap_close_low = 0;
static int m_gap_close_high = 0;
static int h1, h2, o1, o2, c1, c2, l1, l2;

static int is_one_stab(void)
{
	int up1, obj1, down1, total1;
	int up2, obj2, down2, total2;

	if (o1 <= c1) return 0;
	total1 = h1 - l1;
	//up1 = h1 - o1;
	//if (up1 > total1 * 40 / 100) return 0;
	obj1 = o1 - c1;
	if (obj1 < c1 * 1.5 / 100) return 0;
	down1 = c1 - l1;

	if (c2 < o2) return 0;
	total2 = h2 - l2;
	//up2 = h2 - c2;
	//if (up2 > total2 * 40 / 100) return 0;
	obj2 = c2 - o2;
	if (obj2 < o2 * 1 / 100) return 0;
	//down2 = o2 - l2;
	//if (down2 > total2 * 40 / 100) return 0;

	if (o2 > c1 + (obj1 / 1)) return 0;
	if (c2 > o1 + (obj1 / 1)) return 0;
	if (40 > (c2-o2) * 1000 / o2) return 0;

	return 1;
}

static int is_one_jump(void)
{
	int i, c3;
	//printf("s:%d e:%d\n", m_start_index, m_end_index);
	gaps_parse();
	if (m_gaps_len <= 0) return 0;
	//printf("m_gaps_len=%d\n", m_gaps_len);
	//print_gaps();
	gaps_fill();
	//print_gaps();
	for (i = 0; i < m_gaps_len; i ++) {
		if (m_gaps[i].high) break;
	}
	if (i >= m_gaps_len) return 0;

	do_look_one(m_end_index - 1);
	if (glow <= 0) return 0;
	o1 = gopen;
	h1 = ghigh;
	c1 = gclose;
	l1 = glow;
	do_look_one(m_end_index);
	if (glow <= 0) return 0;
	o2 = gopen;
	h2 = ghigh;
	c2 = gclose;
	l2 = glow;

	//if (l2 > m_gaps[i].high * 102 / 100) return 0;
	if (l1 > m_gaps[i].high * 102 / 100 &&
		l2 > m_gaps[i].high * 102) return 0;
	if (is_one_stab() == 0) return 0;

	m_gap_distance = m_gaps[i].index;

	m_gap_close_low = 0; m_gap_close_high = 0;
	do_look_one(m_gaps[i].index);
	c2 = gclose;
	if (m_gaps[i].index != m_start_index) {
		do_look_one(m_gaps[i].index - 1);
		c1 = gclose;
		m_gap_close_low = (c2 - c1) * 1000 / c1;
	}
	do_look_one(m_gaps[i].index + 1);
	c3 = gclose;
	m_gap_close_high = (c3 - c2) * 1000 / c2;

	return 1;
}

static void do_all_jumps(char *start, char *end)
{
	int i, c = 0;
	for (i = 0; i < amount; i ++) {
		if (gaps_init(stocks[i], start, end) < 0)
			continue;
		if (is_one_jump()) {
			printf("%s\t%02d\t%03d\t%03d\n", 
				stocks[i],
				m_end_index - m_gap_distance - 1,
				m_gap_close_low,
				m_gap_close_high);
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
