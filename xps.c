/*Little implementation of 'ps' identical to:
ps ax -o pid,ruser,comm,rss --no-headers --sort -rss */

#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <unistd.h>
#include <sys/stat.h>
#include <ctype.h>
#include <string.h>
#include <pwd.h>

#define proc_path "/proc/"

char *user=NULL, *rss=NULL, *name=NULL;

int is_number(char *string)
//Check whether a given string contains only digits. Returns 1 if true and 0 if false.
{
	size_t str_len=strlen(string);
	for (unsigned i=0;i<str_len;i++)
		if (!(isdigit(string[i])))
			return 0;
	return 1;
}

char *get_user_by_uid(uid_t uid)
{
	struct passwd *pw;
	pw=getpwuid(uid);
	return pw->pw_name;
}

char *xstrbtw(char *str)
{
	size_t str_len=strlen(str);
	char *clean_str=NULL;
	for (unsigned i=0;i<str_len;i++) {
		if (isspace(str[i]) && !isspace(str[i+1])) {
			unsigned j=0;
			clean_str=calloc(20, sizeof(char *));
			while (!isspace(str[++i]))
				clean_str[j++]=str[i];
			clean_str[j]='\0';
			break;
		}
	}
	return clean_str;
}

void get_proc_info(pid_t pid)
{
	FILE *proc_fp;
	char *status_file=calloc(30, sizeof(char *));
	sprintf(status_file, "/proc/%d/status", pid);
	proc_fp=fopen(status_file, "r");
	free(status_file);
	if (proc_fp == NULL) return;
	char line[256]="";
	while(fgets(line, sizeof(line), proc_fp)) {
		if (strncmp(line, "Name", 4) == 0)
			name=xstrbtw(line);
		if (strncmp(line, "Uid", 3) == 0) {
			char *uid=xstrbtw(line);
			user=get_user_by_uid((pid_t)atoi(uid));
			free(uid);
		}
		if (strncmp(line, "VmRSS", 5) == 0)
			rss=xstrbtw(line);
	}
	fclose(proc_fp);
}

int get_pids(const struct dirent *entry)
{
	if (entry->d_type == DT_DIR && is_number((char *)entry->d_name))
		return 1;
	else return 0; 
}

void sort_list(int procs_n, int rss_array[], char **str)
{
	int tmp=0;
	for (int i=0;i<procs_n;i++) {
		for (int j=0;j<procs_n-1;j++) {
			//Order the int array. Otherwise, the condition will not be updated
			if (rss_array[j]<rss_array[j+1]) {
				tmp=rss_array[j];
				rss_array[j]=rss_array[j+1];
				rss_array[j+1]=tmp;
				//Now, order the string array
				char *tmp_str=calloc(strlen(str[j])+1, sizeof(char *));
				strcpy(tmp_str, str[j]);
				strcpy(str[j], str[j+1]);
				strcpy(str[j+1], tmp_str);
				free(tmp_str);
			}
		}
	}
}

void list_cur_procs(void)
{
	struct dirent **proc_pids=NULL;
	int files_n=scandir(proc_path, &proc_pids, get_pids, alphasort);
	if (files_n < 0) return;
	char **output=calloc(files_n, sizeof(char **));
	int rss_array[files_n]; 
	int i=0, j=0;
	for (i=0;i<files_n;i++) {
		get_proc_info((pid_t)atoi(proc_pids[i]->d_name));
		if (rss) rss_array[j]=atoi(rss);
		else rss_array[j]=0;
		
		output[j]=calloc(40, sizeof(char *));
		sprintf(output[j++], "%5s %-9.8s%-15.14s %5s", proc_pids[i]->d_name, (user) ? user : "??", 
									   (name) ? name : "??", (rss) ? rss : "0");
		if (name) { 
			free(name);
			name=NULL;
		}
		if (rss) {
			free(rss);
			rss=NULL;
		}
		free(proc_pids[i]);
	}
	free(proc_pids);
	
	//Sort the array
	sort_list(j, rss_array, output);
	//Print and free
	for (i=0;i<j;i++) {
		if (output[i]) {
			printf("%s\n", output[i]);
			free(output[i]);
		}
	}
	free(output);
}

int main(void)
{
	list_cur_procs();
	return 0;
}
