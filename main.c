/****************************************************************************
*                   KCW testing ext2 file system                            *
*****************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <ext2fs/ext2_fs.h>
#include <string.h>
#include <libgen.h>
#include <sys/stat.h>
#include <time.h>
#include "type.h"



// global
MINODE minode[NMINODE];
MTABLE mtable[NMTABLE];
MINODE *root;

PROC   proc[NPROC], *running;

char gpath[128]; // global for tokenized components
char *name[32];  // assume at most 32 components in pathname
char *rootdev = "mydisk";
int   n;         // number of component strings

int fd, dev;
int nblocks, ninodes, bmap, imap, inode_start, iblock;

//MINODE *iget();
#include "util.c"
#include "mkdir_creat.c"
#include "cd_ls_pwd.c"
#include "rmdir.c"
#include "link_unlink.c"
#include "symlink.c"



int init()
{
  int i, j;
  MINODE *mip;
  PROC   *p;

  printf("init()\n");

  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    mip->dev = mip->ino = 0;
    mip->refCount = 0;
    mip->mounted = 0;
    mip->mptr = 0;
  }
  for (i=0; i<NPROC; i++){
    p = &proc[i];
    p->pid = i;
    p->uid = p->gid = 0;
    p->cwd = 0;
    p->status = FREE;
    for (j=0; j<NFD; j++)
      p->fd[j] = 0;
  }
}


int fs_init()
{
  int i,j;
  for(i=0;i<NMINODE;i++)
    minode[i].refCount = 0;
  
  for(i=0;i<NMTABLE;i++)
    mtable[i].dev = 0;
  
  for(i=0; i<NPROC;i++)
  {
    proc[i].status = READY;
    proc[i].pid = i;
    proc[i].uid = i;
    for(j = 0; j<NFD; j++)
      proc[i].fd[j] = 0;
    proc[i].next = &proc[i+1];
  }
  proc[NPROC-1].next = &proc[0];
  running = &proc[0];
}


// load root INODE and set root pointer to it
int mount_root(char* rootdev)
{  
  printf("mount_root()\nrootdev = %s\n",rootdev);

int i;
MTABLE *mp;
SUPER *sp;
GD *gp;
char buf[BLKSIZE];

dev = open(rootdev, O_RDWR);
if(dev<0)
{
  printf("cant open root device");
  exit(1);
}
get_block(dev,1,buf);
sp= (SUPER*)buf;
if(sp->s_magic!=EXT2_SUPER_MAGIC)
{
  printf("super mafic = %x : %s is not an EXT2 filesys\n",sp->s_magic,rootdev);
  exit(0);
}

//fill mount table[0] with rootdev info
mp = &mtable[0];
mp->dev = dev;

ninodes = mp->ninodes = sp->s_inodes_count;
nblocks = mp->nblocks = sp->s_blocks_count;
strcpy(mp->devName, rootdev);
strcpy(mp->mntName, "/");

get_block(dev,2,buf);
gp = (GD*)buf;
bmap = mp->bmap = gp->bg_block_bitmap;
imap = mp->imap = gp->bg_inode_bitmap;
iblock = mp->iblock = gp->bg_inode_table;
printf("bmap=%d imap=%d iblock=%d\n",bmap,imap,iblock);

root = miget(dev,2);
mp->mntDirPtr = root;
root->mptr = mp;

for(i = 0; i<NPROC;i++)
{
  proc[i].cwd = miget(dev,2);

}
  printf("mount :%s mounted on / \n", rootdev);
}



char *disk = "diskimage";
int main(int argc, char *argv[ ])
{
  int ino;
  char buf[BLKSIZE];
  char line[128], cmd[32], pathname[128], pathname_2[128];

  if (argc > 1)
    disk = argv[1];

  printf("checking EXT2 FS ....");
  if ((fd = open(disk, O_RDWR)) < 0){
    printf("open %s failed\n", disk);
    exit(1);
  }
  dev = fd;   

  /********** read super block  ****************/
  get_block(dev, 1, buf);
  sp = (SUPER *)buf;

  /* verify it's an ext2 file system ***********/
  if (sp->s_magic != 0xEF53){
      printf("magic = %x is not an ext2 filesystem\n", sp->s_magic);
      exit(1);
  }     
  printf("EXT2 FS OK\n");
  ninodes = sp->s_inodes_count;
  nblocks = sp->s_blocks_count;

  get_block(dev, 2, buf); 
  gp = (GD *)buf;

  bmap = gp->bg_block_bitmap;
  imap = gp->bg_inode_bitmap;
  inode_start = gp->bg_inode_table;
  printf("bmp=%d imap=%d inode_start = %d\n", bmap, imap, inode_start);

  init();  
  fs_init();
  mount_root(disk);
  printf("root refCount = %d\n", root->refCount);

  printf("creating P0 as running process\n");
  running = &proc[0];
  running->status = READY;
  running->cwd = miget(dev, 2);
  printf("root refCount = %d\n", root->refCount);

  while(1){
    printf("input command : [ls|cd|pwd|mkdir|rmdir|creat|link|unlink|symlink|quit] ");
    fgets(line, 128, stdin);
    line[strlen(line)-1] = 0;

    if (line[0]==0)
       continue;
    pathname[0] = 0;
    pathname_2[0] = 0;

    sscanf(line, "%s %s %s", cmd, pathname, pathname_2);
    printf("cmd=%s pathname=%s %s\n", cmd, pathname, pathname_2);
  
    if (strcmp(cmd, "ls")==0)
       ls(pathname);
    if (strcmp(cmd, "cd")==0)
       cd(pathname);
    if(strcmp(cmd, "chdir") ==0)
      chdir(pathname);

    if(strcmp(cmd, "mkdir") ==0)
      make_dir(pathname);

    if(strcmp(cmd, "rmdir") ==0)
     rmdir(pathname);

    if(strcmp(cmd, "creat") ==0)
     mcreat(pathname);

    if (strcmp(cmd, "pwd")==0)
    {
      pwd(running->cwd);
      printf("\n");
    }
    if(strcmp(cmd, "link") ==0)
    link(pathname, pathname_2);

    if(strcmp(cmd, "unlink") ==0)
    unlink(pathname);

    if(strcmp(cmd, "symlink") ==0)
    symlink(pathname, pathname_2);

    if (strcmp(cmd, "quit")==0)
       quit();

  }
}

int quit()
{
  int i;
  MINODE *mip;
  for (i=0; i<NMINODE; i++){
    mip = &minode[i];
    if (mip->refCount > 0)
      miput(mip);
  }
  exit(0);
}
