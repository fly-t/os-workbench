#include <assert.h>
#include <ctype.h>
#include <dirent.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ucontext.h>

#define BUFF_SIZE 32
#define FILE_BUFFER_SIZE 1024

int read_file(const char *__restrict __format, ...);
int read_dir(char *dir_path);
int is_numeric(const char *file_name);
int extract_value(const char *str_buf,const char *str_key, char *str_value);

int extract_value(const char *str_buf, const char *str_key, char *str_value){
  /* find the key pos of buffer */
  char *pos = strstr(str_buf, str_key);
  if (pos==NULL) {
    return -1;
  }
  /* jump end of str_key */
  pos+=strlen(str_key);
  
  /* find the start index of str_value */
  while (*pos == ' ' || *pos == ':' || *pos == '\t') {
    /* step over the space */
    pos++;
  }
  /* find the end index of str_value */
  char *end = pos;
  while (*end != '\n' && *end != '\0' && *end != ' ') {
    end++;
  }
  strncpy(str_value,pos,(end-pos));
  str_value[end - pos] = '\0';
  return 0;
}

/* 0: means not number 
*  1: means is number
*/
int is_numeric(const char *file_name){
  while (*file_name) {
    if (!isdigit(*file_name)) {
      return 0;
    }
    file_name++;
  }
  return 1;
}


int read_dir(char *dir_path) {
  DIR *dir;
  struct dirent *entry;
  dir = opendir(dir_path);

  if (dir == NULL) {
    perror("无法打开目录");
    return -1;
  }

  while ((entry = readdir(dir)) != NULL) {
    if (entry->d_type == DT_DIR) {
      // 在类Unix系统中，包括Linux，每个文件夹（目录）下确实都存在两个特殊的目录：`.`
      // 和 `..`。
      if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
        printf("%s->", entry->d_name);
        if(is_numeric(entry->d_name)){
          read_file("/proc/%s/status", entry->d_name);
        }
      }
    }
  }

  return 0;
}
static int parent_buf[1000];
static int childern_buf[1000];
static int paren_count = 0;
static int childern_count = 0;
int read_file(const char *__restrict __format, ...) {
  va_list args;
  va_start(args, __format);
  char fname_buf[BUFF_SIZE];
  int len = vsnprintf(fname_buf, sizeof(fname_buf), __format, args);
  va_end(args);

  /* 判断是否溢出 */
  assert(len >= 0 && len < BUFF_SIZE);

  FILE *fp;
  long file_size;
  char read_buffer[FILE_BUFFER_SIZE];

  fp = fopen(fname_buf, "r");
  if (fp == NULL) {
    perror("无法打开文件");
    return -1;
  }

  /* read file content to buffer */
  int bytes_read =  fread(read_buffer, 1, FILE_BUFFER_SIZE, fp);

  char ppid[20];
  char pid[20];
  
  extract_value(read_buffer, "PPid", ppid);
  extract_value(read_buffer, "Pid", pid);
  parent_buf[paren_count] = atoi(ppid);
  childern_buf[childern_count] = atoi(pid);
  paren_count++;
  childern_count++;
  printf("Pid:%s ppid:%s\n", pid, ppid);
  fclose(fp);
  return 0;
}

/* 去重 */
void remove_duplicates(int arr[], int *size) {
  int i, j, k;

  // 遍历数组中的每个元素
  for (i = 0; i < *size; i++) {
    // 检查是否已经存在与前面的元素相同的元素
    for (j = 0; j < i; j++) {
      if (arr[i] == arr[j]) {
        // 如果存在重复元素，将其后面的元素向前移动一个位置
        for (k = i; k < *size - 1; k++) {
          arr[k] = arr[k + 1];
        }
        // 更新数组的大小
        (*size)--;
        // 由于数组大小减小了，因此当前位置需要重新检查
        i--;
        break;
      }
    }
  }
}

#define MAX_CHILDREN 100

// 进程节点结构体
typedef struct ProcessNode {
  int pid;                                    // 进程ID
  struct ProcessNode *children[MAX_CHILDREN]; // 子节点指针数组
  int num_children;                           // 子节点数量
} ProcessNode;

// 创建进程节点
ProcessNode *createProcessNode(int pid) {
  ProcessNode *node = (ProcessNode *)malloc(sizeof(ProcessNode));
  if (node != NULL) {
    node->pid = pid;
    node->num_children = 0;
    for (int i = 0; i < MAX_CHILDREN; i++) {
      node->children[i] = NULL;
    }
  }
  return node;
}

// 添加子节点
void addChild(ProcessNode *parent, ProcessNode *child) {
  if (parent != NULL && child != NULL && parent->num_children < MAX_CHILDREN) {
    parent->children[parent->num_children++] = child;
  }
}

// 递归构建进程树
ProcessNode *buildProcessTree(int *pids, int *ppids, int num_processes,
                              int pid) {
  ProcessNode *node = createProcessNode(pid);
  if (node == NULL) {
    return NULL;
  }
  for (int i = 0; i < num_processes; i++) {
    if (ppids[i] == pid) {
      addChild(node, buildProcessTree(pids, ppids, num_processes, pids[i]));
    }
  }
  return node;
}

// 递归打印进程树
void printProcessTree(ProcessNode *root, int level) {
  if (root == NULL) {
    return;
  }
  // 打印当前进程节点
  for (int i = 0; i < level; i++) {
    printf("  "); // 输出缩进，表示层级关系
  }
  printf("|-- %d\n", root->pid); // 打印进程ID
  // 递归打印子进程节点
  for (int i = 0; i < root->num_children; i++) {
    printProcessTree(root->children[i], level + 1);
  }
}

int main(int argc, char *argv[]) {
  for (int i = 0; i < argc; i++) {
    assert(argv[i]);
    printf("argv[%d] = %s\n", i, argv[i]);
  }
  assert(!argv[argc]);

  read_dir("/proc/");
  // 从根节点开始递归构建进程树
  ProcessNode *root = buildProcessTree(childern_buf, parent_buf, childern_count, 0);
  // 打印进程树
    printProcessTree(root, 0);
  return 0;
}
