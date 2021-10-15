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

static void do_get_list(char *list)
{
	char *fdata;
	fdata = get_list_data(list);
	amount = handle_list_data(fdata);
	stocks = get_all_stocks_code(fdata, amount);
	printf("amount=%d\n", amount);
}

static void download_stock(char *code, int num, char *path)
{
	int i;
	char cmd[1024], buf[16];
	memset(cmd, 0, sizeof(cmd));
	sprintf(cmd, "wget -O %s/%s \"https://money.finance.sina.com.cn/quotes_service/api/json_v2.php/CN_MarketData.getKLineData?symbol=%s%s&scale=1680&ma=no&datalen=%d\"", path, code, code[0]=='6'?"sh":"sz", code, num);
	//printf("cmd:%s\n", cmd);
	system(cmd);
}

static void do_download(char *path,  int n)
{
	int i;
	for (i = 0; i < n; i ++) {
		download_stock(stocks[i], n, path);
	}
}

int main(int argc, char *argv[])
{
	if (argc < 2) {
		printf("Please input how many weeks for saving\n");
		return 0;
	}
	do_get_list("list.all");
	#define SAVED_PATH	"dataw/"
	path_check(SAVED_PATH);
	do_download(SAVED_PATH, string_to_int(argv[1])/100);

	if (stocks[0]) free(stocks[0]);
	if (stocks) free(stocks);
	printf("==== end ====\n");
	return 0;
}
