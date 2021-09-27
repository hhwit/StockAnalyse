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

#define LIST_FILE_BUF_LEN	(500 * 1024)
#define LIST_LIST_BUF_LEN	(7 * 5000)
static char **stocks = 0;
static int amount;

#define HISTORY_MAX	5
struct history_s {
	int price;
	int time;
	long long volume;
};

struct stock_s {
	int open;
	int yesclose;
	int begin;
	int next;
	struct history_s his[HISTORY_MAX];
	int tipped;
	int invalid;
};

#define ONE_BUFFER_LENGTH	(1024)

static int gopen = 0;
static int gyesclose = 0;
static int gtoclose = 0;
static int ghigh = 0;
static int glow = 0;
static long long gvolume = 0;
static long long gturnover = 0;
static int gtime = 0;

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

static int get_time(void)
{
	int h, m, s;
	char buf[32];
	memset(buf, 0, sizeof(buf));
	memcpy(buf, comma[30] + 1, comma[31] - comma[30] - 1);
	buf[2] = 0;
	buf[5] = 0;
	h = string_to_int(&buf[0]);
	m = string_to_int(&buf[3]) / 100;
	s = string_to_int(&buf[6]) / 100;
	gtime = h * 36 + m  * 60 + s;
	return gtime;
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
	get_time();
	return ret;
}

static void do_look_one(char *data, char *code)
{
	char *p = get_original_data(data, code);
	if (!p) return;
	parse_original_data(p);
	free(p);
}

static char *get_list_data(char *path, int *len)
{
	int fp, ret;
	char *data;
	if (access(path, F_OK) < 0) {
		printf("File NOT found: %s\n", path);
		return 0;
	}
	fp = open(path, O_RDONLY);
	if (fp < 0) {
		printf("Can't open list: %s\n", path);
		return 0;
	}
	data = (char *)malloc(LIST_FILE_BUF_LEN);
	memset(data, 0, LIST_FILE_BUF_LEN);
	ret = read(fp, data, LIST_FILE_BUF_LEN);
	close(fp);
	if (ret <= 0) return 0;
	*len = ret;
	return data;
}

static char *handle_list_data(char *data, int len, int *num)
{
	int i, n;
	char *tmp, *sp;
	char *p = data;
	tmp = (char *)malloc(LIST_LIST_BUF_LEN);
	memset(tmp, 0, sizeof(LIST_LIST_BUF_LEN));
	for (i = 0; i < len; i ++, p ++)
		if ( *p < '0' || *p > '9') *p = 0;
	sp = tmp; p = data;
	for (i = 0, n = 0; i < len; i ++, p ++) {
		if (!(*p)) continue;
		if (strlen(p) == 6) {
			memcpy(sp, p, 6);
			sp += 7; p += 6; i += 6; n ++;
		}
	}
	*num = n;
	free(data);
	return tmp;
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

static char *stock_all_data_p = 0;
#define STOCKS_DATA_BUFFER_LENGTH	(2 * 1024 * 1024)

static int download_stock(char *save, char **code, int num)
{
	FILE *fp;
	int i, n, len = 0;
	char cmd[1024], buf[16];
	char *ret;
	char *p = save;
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "%s", "wget  -q -O - http://hq.sinajs.cn/list=");
	for (i = 0; i < num; i ++) {
		if (code_check(code[i]) < 0) continue;
		memset(buf, 0, sizeof(buf));
		sprintf(buf, "%s%s%s", code[i][0] == '6' ? "sh" : "sz",
					code[i],
					i == num - 1 ? "" : ",");
		strcat(cmd, buf);
	}
	fp = popen(cmd, "r");
	if (fp <= 0) {
		perror("popen failed");
		return 0;
	}
	while (1) {
		ret = fgets(p, 255, fp);
		if (ret == 0) break;
		n = strlen(ret);
		len += n;
		p += n;
	}
	pclose(fp);
	return len;
}

static void do_get_list(char *list)
{
	int i, len;
	char *fdata, *list_buf;
	fdata = get_list_data(list, &len);
	if (fdata == 0) return;
	list_buf = handle_list_data(fdata, len, &amount);
	stocks = get_all_stocks_code(list_buf, amount);
	//printf("Stocks count: %d\n", amount);
	//for (i = 0; i < amount; i ++) {
	//	printf("%s\t", stocks[i]);
	//}
	//printf("\n");
}

#define DOWNLOAD_MAX    100
static void do_download_data(void)
{
	char *p;
	int ret, i, n, m;
	if (stock_all_data_p == 0)
		stock_all_data_p = (char *)malloc(STOCKS_DATA_BUFFER_LENGTH);
	memset(stock_all_data_p, 0, sizeof(STOCKS_DATA_BUFFER_LENGTH));
	p = stock_all_data_p;
	n = amount / DOWNLOAD_MAX;
	m = amount % DOWNLOAD_MAX;
	for (i = 0; i < n; i ++) {
		ret = download_stock(p, &stocks[i*DOWNLOAD_MAX], DOWNLOAD_MAX);
		p += ret;
	}
	if (m != 0) {
		download_stock(p, &stocks[i*DOWNLOAD_MAX], m);
	}
}

static int get_sys_time(void)
{
	time_t timep;
	struct tm *t;
	time(&timep);
	t = localtime(&timep);
	return (t->tm_hour * 3600) + (t->tm_min * 60) + t->tm_sec;
}

static struct stock_s *stocks_sp;
static int g_time_calibrate = 0;
static int g_loop_time = 0;

static void is_trading_hour(void)
{
	int sys_t;
	while (1) {
		sys_t = get_sys_time() + g_time_calibrate;
		if ((sys_t >= (9 * 3600) + (30 * 60) &&
			sys_t <= (11 * 3600) + (30 * 60)) ||
			(sys_t >= 13 * 3600  &&
			sys_t <= 15 * 3600)) break;
		if (sys_t % 10 == 0)
			printf("It's NOT trading hour: %02d:%02d:%02d\n", 
				sys_t / 3600, 
				(sys_t % 3600) / 60, 
				(sys_t % 3600) % 60);
		usleep(1 * 1000 * 1000);
	}
	g_loop_time = sys_t;
}

static int time_calibrate(void)
{
	int i, sys_t;
	long long sum_t;
	sys_t = get_sys_time();
	for (i = 0, sum_t = 0; i < amount; i ++) {
		sum_t += stocks_sp[i].begin;
	}
	sum_t /= amount;
	if ((sum_t > (9 * 3600) + (30 * 60) &&
		sum_t < (11 * 3600) + (30 * 60)) ||
		(sum_t > 13 * 3600  &&
		sum_t < 15 * 3600)) {
		g_time_calibrate =  (int)sum_t - sys_t;
		return 1;
	}
	printf("Last trading hour: %02d:%02d:%02d (current time: %02d:%02d:%02d)\n", 
		(int)sum_t / 3600, ((int)sum_t % 3600) / 60, ((int)sum_t % 3600) % 60,
		sys_t / 3600, (sys_t % 3600) / 60, (sys_t % 3600) % 60);
	return 0;
}

static void do_data_init(void)
{
	int i;
	stocks_sp = (struct stock_s *)malloc(sizeof(struct stock_s) * amount);
	if (!stocks_sp) return;
	memset(stocks_sp, 0, sizeof(struct stock_s) * amount);
	while (1) {
		do_download_data();
		for (i = 0; i < amount; i ++) {
			do_look_one(stock_all_data_p, stocks[i]);
			stocks_sp[i].open = gopen;
			stocks_sp[i].yesclose = gyesclose;
			stocks_sp[i].begin = gtime;
			printf("%d. %s\t%d.%d\n", i + 1, stocks[i], gtoclose / 100, gtoclose % 100);
		}
		if (time_calibrate()) break;
		usleep(10 * 1000 * 1000);
	}
}

static void do_process(void);

static void update_stocks_data(void)
{
	int i, next;
	do_download_data();
	for (i = 0; i < amount; i ++) {
		do_look_one(stock_all_data_p, stocks[i]);
		next = stocks_sp[i].next;
		stocks_sp[i].his[next].price = gtoclose;
		stocks_sp[i].his[next].volume = gvolume;
		stocks_sp[i].his[next].time = gtime;
		next ++;
		stocks_sp[i].next = next >= HISTORY_MAX ? 0 : next;
	}
}

#define VOLUME_PERIOD	60

int main(int argc, char *argv[])
{
	do_get_list("list.txt");
	if (amount <= 0) {
		printf("Please input stock list to list.txt\n");
		return 0;
	}
	is_trading_hour();
	do_data_init();
	printf("==== Decteting Start ====\n");
	while (1) {
		is_trading_hour();
		//g_loop_time = get_sys_time();
		if (g_loop_time % VOLUME_PERIOD == 0) {
			update_stocks_data();
		#if 0
			int i, j;
			for (i = 0; i < amount; i ++) {
				printf("%s:\t", stocks[i]);
				for (j = 0; j < HISTORY_MAX; j ++) {
					printf("(%d:%d %02d:%02d:%02d %lld)\t",
						j,
						stocks_sp[i].his[j].price,
						stocks_sp[i].his[j].time / 3600,
						(stocks_sp[i].his[j].time % 3600) / 60,
						(stocks_sp[i].his[j].time % 3600) % 60,
						stocks_sp[i].his[j].volume);
				}
				printf("\n");
			}
			printf("\n");
		#endif
		}
		do_process();
		usleep(1 * 1000 * 1000);
	}

	if (stocks[0]) free(stocks[0]);
	if (stocks) free(stocks);
	printf("==== end ====\n");
	return 0;
}

static int p1, p2, p3;
static int t1, t2, t3;
static long long v1, v2, v3;

static int get_key_data(struct stock_s *s)
{
	int next = s->next;
	if (next == 0) {
		p1 = s->his[HISTORY_MAX - 3].price;
		p2 = s->his[HISTORY_MAX - 2].price;
		p3 = s->his[HISTORY_MAX - 1].price;
		t1 = s->his[HISTORY_MAX - 3].time;
		t2 = s->his[HISTORY_MAX - 2].time;
		t3 = s->his[HISTORY_MAX - 1].time;
		v1 = s->his[HISTORY_MAX - 3].volume;
		v2 = s->his[HISTORY_MAX - 2].volume;
		v3 = s->his[HISTORY_MAX - 1].volume;
	} else if (next == 1) {
		p1 = s->his[HISTORY_MAX - 2].price;
		p2 = s->his[HISTORY_MAX - 1].price;
		p3 = s->his[0].price;
		t1 = s->his[HISTORY_MAX - 2].time;
		t2 = s->his[HISTORY_MAX - 1].time;
		t3 = s->his[0].time;
		v1 = s->his[HISTORY_MAX - 2].volume;
		v2 = s->his[HISTORY_MAX - 1].volume;
		v3 = s->his[0].volume;
	} else if (next == 2) {
		p1 = s->his[HISTORY_MAX - 1].price;
		p2 = s->his[0].price;
		p3 = s->his[1].price;
		t1 = s->his[HISTORY_MAX - 1].time;
		t2 = s->his[0].time;
		t3 = s->his[1].time;
		v1 = s->his[HISTORY_MAX - 1].volume;
		v2 = s->his[0].volume;
		v3 = s->his[1].volume;
	} else {
		p1 = s->his[next - 3].price;
		p2 = s->his[next - 2].price;
		p3 = s->his[next - 1].price;
		t1 = s->his[next - 3].time;
		t2 = s->his[next - 2].time;
		t3 = s->his[next - 1].time;
		v1 = s->his[next - 3].volume;
		v2 = s->his[next - 2].volume;
		v3 = s->his[next - 1].volume;
	}
	if (p1 == 0 || p2 == 0 || p3 == 0
			|| t1 == 0 || t2 == 0 || t3 == 0
			|| v1 == 0 || v2 == 0 || v3 == 0) return 0;
	return 1;
}

void do_process(void)
{
	int i;
	int t_last, t_curr;
	long long v_last, v_curr;
	struct stock_s *s;
	for (i = 0; i < amount; i ++) {
		if (get_key_data(&stocks_sp[i]) == 0) continue;
		t_last = t2 - t1;
		t_curr = t3 - t2;
		v_last = v2 - v1;
		v_curr = v3 - v2;
		if (t_last > VOLUME_PERIOD + 2 ||
			t_last < VOLUME_PERIOD - 2) continue;
		if (t_curr > VOLUME_PERIOD + 2 ||
			t_curr < VOLUME_PERIOD - 2) continue;
		if (v_curr > v_last * 2) {
			if (stocks_sp[i].tipped == t3) continue;
			printf("%02d:%02d:%02d(%s)\t%d.%02d\t%s\n",
				t3 / 3600,
				(t3 % 3600) / 60,
				(t3 % 3600) % 60,
				stocks[i],
				p3 / 100,
				p3 % 100,
				p3 >= p2 ? "up" : "down");
			stocks_sp[i].tipped = t3;
		}
	}
}
