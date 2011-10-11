/* C */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>

/* Unix */
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

/* Linux */
#include <linux/types.h>
#include <linux/input.h>
#include <linux/hidraw.h>

int *report_list;
int report_list_size;

int desc_data(__u8 *value, int data_size)
{
	int data = 0;

	if (data_size >= 1)
		data += value[data_size - 1];
	if (data_size >= 2)
		data += value[data_size - 2] << 8;
	if (data_size >= 3)
		data += value[data_size - 3] << 16;

	return data;
}

void add_report_id(int report_id)
{
	report_list = realloc(report_list, report_list_size+1);
	report_list[report_list_size] = report_id;
	report_list_size++;
}

int fill_report_list(int fd)
{
	int i, res, desc_size, current_usage_page, data_size, data;
	struct hidraw_report_descriptor rpt_desc;
	res = ioctl(fd, HIDIOCGRDESCSIZE, &desc_size);
	if (res < 0)
		return 1;

	memset(&rpt_desc, 0x0, sizeof(rpt_desc));
	rpt_desc.size = desc_size;
	res = ioctl(fd, HIDIOCGRDESC, &rpt_desc);
	if (res < 0)
		return 1;

	report_list = NULL;
	report_list_size = 0;

	for (i = 0; i < rpt_desc.size; i++) {
		data_size = rpt_desc.value[i] & 3;
		if ((rpt_desc.value[i] >> 2) == 1)
			current_usage_page = desc_data(
				&rpt_desc.value[i+1], data_size);
		else if ((rpt_desc.value[i] >> 2) == 33)
			if (current_usage_page == 0xFF) {
				add_report_id(desc_data(
					&rpt_desc.value[i+1], data_size));
			}
		i += data_size;
	}
	return 0;
}

int valid_report_id(__u8 report_id)
{
	int i;
	for (i = 0; i < report_list_size; i++) {
		if (report_id == report_list[i])
			return 1;
	}
	return 0;
}

int main(int argc, char **argv)
{
	int fd, one_read;
	int i, res;
	char buf[256], *line;
	size_t length;
	fd_set fds;
	struct timeval timeout;

	fd = open(argv[1], O_RDWR|O_NONBLOCK);

	if (fd < 0) {
		perror("Unable to open device");
		return 1;
	}

	if (fill_report_list(fd)) {
		perror("Can't read report descriptor");
		close(fd);
		return 2;
	}

	using_history();
	while(1) {
		line = readline(">> ");
		length = strlen(line);
		if(length > 0) {
			if(line[0] == ';') {
				continue;
			}

			add_history(line);
			for(i = 0; sscanf(line + i, "%hx", &buf[i / 3]) && i < length; i += 3);
			res = write(fd, buf, i/3);
			if (res < 0) {
				perror("write");
			}
		}

		one_read = 0;
		do {
			if(!one_read) {
				timeout.tv_sec = 1;
				timeout.tv_usec = 0;
				FD_ZERO(&fds);
				FD_SET(fd, &fds);
				if(select(fd+1, &fds, NULL, NULL, &timeout) == 0)
					break;
			}
			res = read(fd, buf, 46);
			if(res <= 0)
				continue;

			if(!valid_report_id(buf[0]))
				continue;

			one_read = 1;
			printf("<< ");
			for (i = 0; i < res; i++) {
				printf("%02hhx ", buf[i]);
			}
			puts("");
		}
		while(res > 0);
	}
	close(fd);
	return 0;
}
