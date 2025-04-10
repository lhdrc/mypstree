#include<stdio.h>
#include<stdlib.h>
#include<unistd.h>
#include<dirent.h>
#include<string.h>
#include<assert.h>
#define _BSD_SOURCE  // 或 _DEFAULT_SOURCE

typedef struct Treenode
{
    int pid;
    int ppid;
    char *name;
    struct Treenode* firstchild;
    struct Treenode* nextsibling;
    struct Treenode* parent;
} Treenode;

// 动态分配进程节点数组
Treenode** process = NULL;
int max_pid = 0;

Treenode* createNode(int pid, int ppid, char *name)
{
    Treenode* newNode = (Treenode*)malloc(sizeof(Treenode));
    if(!newNode)
    {
        assert(0 && "Memory allocation failed");
        return NULL;
    }
    newNode->pid = pid;
    newNode->ppid = ppid;
    newNode->name = strdup(name); // Duplicate the name to avoid dangling pointers
    newNode->firstchild = NULL;
    newNode->nextsibling = NULL;
    newNode->parent = NULL;
    return newNode;
}

void addChild(Treenode* parent, Treenode* child)
{
    if(!parent || !child)
    {
        return;
    }
    if(!parent->firstchild)
    {
        parent->firstchild = child;
    }
    else
    {
        Treenode* tmp = parent->firstchild;
        while(tmp->nextsibling)
        {
            tmp = tmp->nextsibling;
        }
        tmp->nextsibling = child;
    }
}

// 获取进程信息
void getProcessInfo()
{
    char path[512];
    int pid, ppid;
    char name[256];
    struct dirent *entry = NULL;
    DIR *dir = opendir("/proc");
    if(dir)
    {
        while((entry = readdir(dir)) != NULL)
        {
            if(entry->d_type == 4)
            {
                // 检查是否为数字目录
                int valid_pid = 1;
                for(int i = 0; entry->d_name[i] != '\0'; i++)
                {
                    if(entry->d_name[i] < '0' || entry->d_name[i] > '9')
                    {
                        valid_pid = 0;
                        break;
                    }
                }
                if(valid_pid)
                {
                    pid = atoi(entry->d_name);
                    if(pid > max_pid)
                    {
                        // 动态调整数组大小
                        Treenode** new_process = (Treenode**)realloc(process, (pid + 1) * sizeof(Treenode*));
                        if(!new_process)
                        {
                            assert(0 && "Memory allocation failed");
                            continue;
                        }
                        process = new_process;
                        for(int i = max_pid + 1; i <= pid; i++)
                        {
                            process[i] = NULL;
                        }
                        max_pid = pid;
                    }
                    snprintf(path, sizeof(path), "/proc/%s/stat", entry->d_name);
                    FILE *statFile = fopen(path, "r");
                    if(statFile)
                    {
                        int result = fscanf(statFile, "%d %s %*c %d", &pid, name, &ppid);
                        if (result == 3 && ppid >= 0) {
                            // 去除括号
                            char *start = strchr(name, '(');
                            char *end = strchr(name, ')');
                            if(start && end)
                            {
                                int len = end - start - 1;
                                strncpy(name, start + 1, len);
                                name[len] = '\0';
                            }
                        } 
                        else {
                            fclose(statFile);
                            continue; // Skip invalid entries
                        }
                        Treenode* newNode = createNode(pid, ppid, name);
                        if(!newNode)
                        {
                            fclose(statFile);
                            continue;
                        }
                        process[pid] = newNode;
                        if (ppid > 0 && ppid <= max_pid && process[ppid]) {
                            newNode->parent = process[ppid];
                            addChild(process[ppid], newNode);
                        }
                        fclose(statFile);
                    }
                }
            }
        }
        closedir(dir);
    }
}

// Forward declaration of printTree
void printTree(Treenode* node, int level);

void printTreeroot(int level)
{
    for(int i = 0; i <= max_pid; i++)
    {
        if(process[i] && process[i]->ppid == 0)
        {
            for(int j = 0; j < level; j++)
            {
                printf("  ");
            }
            printf("%s\n", process[i]->name);

            Treenode* child = process[i]->firstchild;
            while(child)
            {
                printTree(child, level + 1);
                child = child->nextsibling;
            }
        }
    }
}

void printTree(Treenode* node, int level)
{
    if(!node)
    {
        return;
    }
    for(int i = 0; i < level; i++)
    {
        printf("   ");
        
    }
    printf("|--");
    
    printf("%s\n", node->name);

    Treenode* child = node->firstchild;
    while(child)
    {
        printTree(child, level + 1);
        child = child->nextsibling;
    }
}

// 释放内存
void freeProcessTree()
{
    for(int i = 0; i <= max_pid; i++)
    {
        if(process[i])
        {
            free(process[i]->name);
            free(process[i]);
        }
    }
    free(process);
}

int main()
{
    getProcessInfo();
    printTreeroot(0);
    freeProcessTree();
    return 0;
}
