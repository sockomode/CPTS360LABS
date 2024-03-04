#include "header.h"

typedef struct node {
	char  name[64];       
	char  type;
	struct node *child, *sibling, *parent;
} NODE;

typedef struct nDir {
    NODE *newDir;
    int status;
} NDIR;

// global variables
NODE *root; 
NODE *cwd;
NODE *pCur;
NODE* pPrev;

int initialize() 
{

	root = (NODE *)malloc(sizeof(NODE));
	strcpy(root->name, "/");
	root->parent = root;
	root->sibling = 0;
	root->child = 0;
	root->type = 'D';
	cwd = root;
	printf("Filesystem initialized!\n");
	return 1;
}

char* tokenize(char* pathName)
{
    char* token = strtok(pathName, "/"); char* tmpStr = malloc(sizeof(char)*100);
    while(token != NULL)
    {
        strcpy(tmpStr,token);
        token = strtok(NULL,"/");
    }
}

NODE* reset(NODE* cwd)
{
    while(cwd->parent != NULL)
    {
        cwd = cwd->parent;
    }
    return cwd;
}

NDIR searchTree(NODE *cwd, char *pathName, int isNode)
{
    NODE *pCur = cwd;    NODE*curChild; char *tmpPath = malloc(sizeof(char)*1000); int found = 0;
    strcpy(tmpPath, pathName);
    char *token = strtok(tmpPath,"/");

    if(tmpPath[0] == '/')
    { 
       pCur = reset(cwd);
    }
    while(token != NULL)
    {
        if(strcmp(token, "..") == 0)
        {
 
            pCur = pCur->parent;
        }
        else
        {
            curChild = pCur->child;
            for(int i = 0; curChild != NULL; i++)
            {
                if(strcmp(curChild->name, token) == 0)
                {
                    found = 1;
                    pCur = curChild;
                    break;
                }
                else
                {
                    curChild = curChild->sibling;
                }
            }
            if(found != 1)
            {
                NDIR state;
                if((isNode == 1) && strtok(NULL,"/") == NULL)
                {
                    state.newDir = pCur; state.status = 1;
                    return state;
                }
                else{
                    state.newDir = NULL; state.status = 0;
                    return state;
                }
            }
        }
         token = strtok(NULL, "/");
    }
    if(isNode == 1)
    {
        NDIR state;
        state.newDir = NULL; state.status = 2;
        printf("Error: File Already Exists\n");
        return state;
    }
    NDIR state;
    state.newDir = pCur; state.status = 1;
    return state;
}

void deleteNode(NODE* ptr)
{
    if(ptr != ptr->parent->child)
    {
        NODE *pCur = ptr->parent->child;
        while(pCur->sibling != ptr)
        {
            pCur = pCur->sibling;
        }
        pCur->sibling = ptr->sibling;
    }
    if(ptr == ptr->parent->child)
    {
        ptr->parent->child = ptr->sibling;
    }
}

int insertNode(char *input, char fileType, NODE *cwd)
{
    NODE *pCur; NODE*curPath; char*baseName = malloc(sizeof(char) * 100);
    NDIR curState = searchTree(cwd,input,1);
    if(curState.status != 1)
    {
        if(curState.status == 0)
        {
            printf("Not Found\n");
            return 0;
        }
        if(curState.status == 2)
        {
            printf("Already Exists\n");
            return 0;
        }
    }
    pCur = curState.newDir;
    if(pCur->child == NULL)
    { 
        NODE *pMem = (NODE*)malloc(sizeof(NODE));
        if(pMem != NULL){
            pMem = pCur;
            curPath = pMem;
            curPath->sibling = 0;
            curPath->child = 0;
            curPath->type = fileType;
        }
    }
    else if (pCur->child != NULL)
    { 
        pCur = pCur->child;
        while(pCur->sibling != NULL)
        { 
            pCur = pCur->sibling;
        }
        NODE* pMem = (NODE*)malloc(sizeof(NODE));
        if(pMem != NULL){
            pCur->sibling = pMem;
            pMem = pCur;
            curPath = pMem;
            curPath->sibling = 0;
            curPath->child = 0;
            curPath->type = fileType;
        }
    }
    baseName = tokenize(input);
    if(baseName != NULL)
    {
        strcpy(curPath->name,baseName);
    }
    return 1;
}

void my_mkdir_function(NODE *cwd, char *input)
{
    if(input != NULL)
    {
        insertNode(input, 'D', cwd);
    }
}

void my_rmdir_function(char *pathName, NODE *cwd)
{
    NDIR curState = searchTree(cwd, pathName, 0); NODE* pCur = curState.newDir;

    if(pathName == NULL){ // no input given
        printf("Error: Not enough arguments\n");
    }
    if(curState.status == 1){
        if(pCur->child != NULL){
            printf("Error");
        }
        if(pCur->type == 'F'){
            printf("Error: Not a Directory");
        }
        deleteNode(pCur);
    }
    if(curState.status != 1){
        printf("DIR %s does not exist!\n",pathName);
    }
}

void my_ls_function(NODE *cwd, char*pathName)
{
    NODE* pCur; NDIR curState = searchTree(cwd,pathName,0);

    if(pathName == NULL){ 
        pCur = cwd->child;
        return;
    }
    if(pathName != NULL){
        if(curState.status != 1)
        {
            pCur = cwd;
            return;
        }
        if(curState.status == 1)
        {
            pCur = curState.newDir->child;
        }
    }
    while(pCur != NULL)
    {
        printf("%c %s\n",pCur->type,pCur->name);
        pCur = pCur->sibling;
    }
}

void my_cd_function(NODE **cwd, char *pathName)
{
    NODE *pCur; NDIR curState = searchTree(*cwd, pathName, 0);
    if(pathName != NULL)
    {
        if(curState.status == 0)
        {
            printf("Error no directory found!\n");
        }
        pCur = curState.newDir;
    }
    else{
        pCur = reset(*cwd);
    }
}

void my_pwd_function(NODE *cwd)
{
    char *pathName = malloc(sizeof(char) * 1000); char* tempStr = malloc(sizeof(char) * 1000);
    if(cwd->parent != cwd)
    {
        while(!(cwd->parent == cwd))
        {
            strcpy(tempStr, "/");
            strcat(tempStr, cwd->name);
            cwd = cwd->parent;
        }
    }
    if(cwd->parent == cwd)
    {
        strcpy(tempStr, "/");
    }
    strcpy(pathName, tempStr);
    printf("%s\n",pathName);
}   

void my_creat_function(NODE *cwd, char *pathName)
{
    if(pathName == NULL)
    {
        printf("Error: No path was given\n");
    }
    int success = insertNode(pathName,'F', cwd);
}

void my_rm_function(NODE *cwd, char *pathName)
{
    NDIR curState = searchTree(cwd,pathName,0);
    if(pathName == NULL)
    {
        printf("Error: No path was given\n");
    }
    if(curState.newDir->type == 'D')
    {
        printf("Error: No file found\n");
    }
    deleteNode(curState.newDir);
}

void my_reload_function(NODE *root, char *pathName)
{
    char* tmpStr = malloc(sizeof(char)* 100); char *token = malloc(sizeof(char)*100);
    FILE* infile = fopen(pathName, "r");
    if(pathName == NULL){
        printf("Error: path not given\n");
    }
    while(fgets(tmpStr,sizeof(tmpStr), infile))
    {
        token = strtok(tmpStr," \n");
        if((strcmp(token, "F") == 0) || (strcmp(token, "D"))){
            printf("Error\n");
        }
        if(strcmp(token, "F") == 0){
            token = strtok(NULL," \n");
            token[strcspn(token, "\n")] = 0;
            my_creat_function(root,token);
        }
        else if(strcmp(token, "D") == 0){
            token = strtok(NULL," \n");
            token[strcspn(token, "\n")] = 0;
            my_mkdir_function(root,token);
        }
    }
}
