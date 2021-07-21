#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#define ONE_BUFFER_LENGTH	(1024)

static int gopen = 0;
static int gyesclose = 0;
static int gtoclose = 0;
static int ghigh = 0;
static int glow = 0;
static int gvolume = 0;
static int gturnover = 0;

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

static int string_to_int(char *s)
{
	int i, j, a, n, m;
	char *p;
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
		switch (j) {
		case 1:
			a += (s[i] - '0') * 100;
			break;
		case 2:
			a += (s[i] - '0') * 1000;
			break;
		case 3:
			a += (s[i] - '0') * 10000;
			break;
		case 4:
			a += (s[i] - '0') * 100000;
			break;
		case 5:
			a += (s[i] - '0') * 1000000;
			break;
		case 6:
			a += (s[i] - '0') * 10000000;
			break;
		case 7:
			a += (s[i] - '0') * 100000000;
			break;
		case 8:
			a += (s[i] - '0') * 1000000000;
			break;
		case 9:
			a += (s[i] - '0') * 10000000000;
			break;
		default:
			printf("Wrong number formate: %s\n", s);
			return -1;
		}
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
	//printf("%s\n", buf);
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

static char *get_original_data(char *code, char *date)
{
	int fp, ret;
	static char *data = 0;
	char path[128];
	memset(path, 0, sizeof(path));
	sprintf(path, "data/%s/%s", date, code);
	if (access(path, F_OK) < 0) {
		printf("File does not exit: %s\n", path);
		return 0;
	}
	fp = open(path, O_RDONLY);
	if (fp < 0) {
		printf("Can't open file: %s\n", path);
		return 0;
	}
	if (!data)
		data = (char *)malloc(ONE_BUFFER_LENGTH);
	memset(data, 0, ONE_BUFFER_LENGTH);
	ret = read(fp, data, ONE_BUFFER_LENGTH);
	close(fp);
	if (ret <= 0) return 0;
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
	return ret;
}

static void do_look_one(char *code, char *date)
{
	char *p = get_original_data(code, date);
	if (!p) return;
	parse_original_data(p);
}

static int is_one_stab(char *code, char *date1, char *date2, char *date3, char *date4)
{
	int up1, obj1, down1, total1;
	int up2, obj2, down2, total2;
	int h1, h2, o1, o2, c1, c2, l1, l2;
	int yc1, yc2;

	do_look_one(code, date1);
	if (gopen <= 0
		|| gtoclose <= 0
		|| ghigh <= 0
		|| glow <= 0) return 0;
	h1 = ghigh;
	o1 = gopen;
	c1 = gtoclose;
	l1 = glow;
	yc1 = gyesclose;
	total1 = h1 - l1;
	if (c1 > o1)
		up1 = h1 - c1;
	else
		up1 = h1 - o1;
	if (up1 < total1 * 50 / 100) return 0;
	if (total1 < yc1 * 3 /100) return 0;

	do_look_one(code, date2);
	if (gopen <= 0
		|| gtoclose <= 0
		|| ghigh <= 0
		|| glow <= 0) return 0;
	h2 = ghigh;
	o2 = gopen;
	c2 = gtoclose;
	l2 = glow;
	yc2 = gyesclose;
	total2 = h2 - l2;
	if (c2 > o2)
		down2 = o2 - l2;
	else
		down2 = c2 - l2;
	if (down2 < total2 * 50 / 100) return 0;
	if (total2 < yc2 * 3 /100) return 0;

	do_look_one(code, date3);
	if (gopen <= 0
		|| gtoclose <= 0
		|| ghigh <= 0
		|| glow <= 0) return 0;
	if (c1 < gtoclose) return 0;

	do_look_one(code, date4);
	if (gopen <= 0
		|| gtoclose <= 0
		|| ghigh <= 0
		|| glow <= 0) return 0;
	if (c1 < gtoclose) return 0;

	return 1;
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
	fdata = get_list_data("list");
	amount = handle_list_data(fdata);
	stocks = get_all_stocks_code(fdata, amount);
}

static void do_all_stab(char *date1, char *date2, char *date3, char *date4)
{
	int i, c = 0;
	do_get_list();
	if (amount <= 0) {
		printf("No stock found\n");
		return;
	}
	for (i = 0; i < amount; i ++) {
		if (is_one_stab(stocks[i], date1, date2, date3, date4)) {
			printf("%s\n", stocks[i]);
			c ++;
		}
	}
	printf("Found: %d\n", c);
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Please input date1 and date2 and date3 and date4\n");
		return 0;
	}
	do_all_stab(argv[1], argv[2], argv[3], argv[4]);

	printf("==== end ====\n");
	return 0;
}
