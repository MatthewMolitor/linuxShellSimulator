/************* cd_ls_pwd.c file **************/
#include <sys/types.h> 
#include <sys/stat.h>
#include <time.h>




int chdir(char *pathname)   
{
  printf("chdir %s\n", pathname);
  //printf("under construction READ textbook HOW TO chdir!!!!\n");
  // READ Chapter 11.7.3 HOW TO chdir
  char temp[64];

  int ino = mgetino(pathname);
  MINODE* mip = miget(dev, ino);
  //MINODE *mip = running->cwd;
  //MINODE *cip = miget(dev, ino);


  if(ino ==0)
  {
    printf("no such directory\n");
    return;
  }

  if(!S_ISDIR(mip->INODE.i_mode))
  {
    printf("not a directory %s\n",temp);
    return;
  }
  printf("changing directory to %s : %d\n", pathname, ino);
  printf("MINODE is %u\n", mip->ino);
  //running->cwd = cip;
  miput(running->cwd);
  running->cwd = mip;
}

int ls_file(MINODE *mip, char *name)
{
  //printf("ls_file: to be done: READ textbook for HOW TO!!!!\n");
  // READ Chapter 11.7.3 HOW TO ls

  struct stat stats;


  
  printf((S_ISDIR(mip->INODE.i_mode)?"D":"-"));
  printf(((mip->INODE.i_mode & S_IRUSR))?"R":"-");
  printf(((mip->INODE.i_mode & S_IWUSR))?"W":"-");
  printf(((mip->INODE.i_mode  & S_IXUSR))?"X":"-");
  printf(((mip->INODE.i_mode  & S_IRGRP))?"R":"-");
  printf(((mip->INODE.i_mode  & S_IWGRP))?"W":"-");
  printf(((mip->INODE.i_mode  & S_IXGRP))?"X":"-");
  printf(((mip->INODE.i_mode  & S_IROTH))?"R":"-");
  printf(((mip->INODE.i_mode  & S_IWOTH))?"W":"-");
  printf(((mip->INODE.i_mode  & S_IXOTH))?"X":"-");
  printf("  links:%4d  %4d  %4d %.19s %10s %10s\n", 
                    mip->INODE.i_links_count,mip->INODE.i_uid, mip->INODE.i_size, 
                    ctime((time_t)&mip->INODE.i_mtime), " ");
                  

}

int ls_dir(MINODE *mip)
{
  char buf[BLKSIZE], temp[256];
  DIR *dp;
  char *cp;
  MINODE *cip=mip;


  // Assume DIR has only one data block i_block[0]
  get_block(dev, mip->INODE.i_block[0], buf); 
  dp = (DIR *)buf;
  cp = buf;
  while (cp < buf + BLKSIZE && dp->name_len>0){
     strncpy(temp, dp->name, dp->name_len);
     temp[dp->name_len] = 0;
     cip = miget(dev,dp->inode);
     ls_file(cip, temp);
     cp += dp->rec_len;
     dp = (DIR *)cp;
  }
  printf("\n");
}


int ls(char *pathname)  
{
    MINODE *mip;
    int ino;
    char temp[128];

    mip = running ->cwd;
    if(strcmp(pathname,"")==0)
    {
      ls_dir(mip);
    }

    else if(pathname) {
      printf("pathname =%s\n",pathname);
      ino =mgetino(pathname);
      if(ino ==0) {
        printf("returned 0\n");
        return;
      }
      mip = miget(dev, ino);
      if(S_ISDIR(mip->INODE.i_mode)){
        ls_dir(mip);
        miput(mip);
        return 1;
      }
      
      else{
        printf("Not a directory\n");
      }
    }
    else
    {
      printf("something went wrong\n");
      return;
    }
    
}



int get_myino(MINODE *mip, int *parent_ino)
{
  INODE *ip;
  char buf[1024];
  char *cp;
  DIR *dp;
  int ino;



  if(!mip)
  {
    printf("error\n");
    return 1;
  }

  ip = &mip->INODE;
  if(!S_ISDIR(ip->i_mode))
  {
    printf("not a directory");
    return -1;
  }

  get_block(dev, ip->i_block[0], buf);
  dp = (DIR*)buf;
  cp = buf;

  ino = dp->inode;

  cp+= dp->rec_len;
  dp = (DIR*)cp;

  *parent_ino = dp->inode;

  return ino;
}

int get_myname(MINODE* parent_minode, int my_ino, char *my_name)
{
int i = 0;
char *cp, sbuf[BLKSIZE];
DIR* dp;


while(i<12)
{
  //printf("why is i resetting>??? %d", i);
    if(parent_minode->INODE.i_block[i]==0)
    {
        printf("returned 0\n");
        return 0;
    }

    get_block(dev, parent_minode->INODE.i_block[i], sbuf);
    dp = (DIR*)sbuf;
    cp = sbuf;
    while(cp<sbuf+BLKSIZE)
    {
        if(dp->inode == my_ino)
        {
          strncpy(my_name, dp->name,dp->name_len);
          my_name[dp->name_len]=0;
         // printf("new name: %s", dp->name);
          return 0;
        }
        else
        {
        cp+=dp->rec_len;
        dp = (DIR*)cp;
        }

    }
    //printf(" i = %d\n", i);
    i=i+1;
   // printf(" i = %d\n", i);
}
printf("failed\n");
return 0;
}



char* rpwd(MINODE *wd)
{
	char temp[128];
  char buf[128];
	int ino, parentino;
	MINODE *pip;
  
  int i = 0;
  

  //if wd == root return
  if(wd == root)
  {
      printf("/");
      return ""; 
  }

//from wd->inode.i[block[0]]
  ino = get_myino(wd,&parentino);

  pip = miget(dev, parentino);

  get_myname(pip, ino, temp);
  rpwd(pip);
  printf("%s/",temp);
  return temp;

}

char *pwd(MINODE *wd)
{
  if(wd ==root)
  {
    printf("/\n");
    return wd;
  }
  else 
    return rpwd(wd);
    
}



int cd(char*pathname)
{
  int i = 0;
  int k = 0;
  char *lname[32];

  i = mtokenize(pathname);

  printf("in cd\n");
  for(k =0;k<i;k++)
  {
    lname[k] = name[k];
  }

  for(k=0;k<i;k++)
  {
    printf("call chdir(%s)\n", lname[k]);
    chdir(lname[k]);
    pwd(running->cwd);
    printf("\n");
  }
  return;
}