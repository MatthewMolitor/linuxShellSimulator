
MINODE* mialloc()
{
    int i ;
    for(i = 0; i<NMINODE;i++)
    {
        MINODE* mp = &minode[i];
        if(mp->refCount ==0)
        {
            mp->refCount = 1;
            return mp;
        }
    }
    printf("panic! out of minodes!");
    return(0);
}


MINODE *miget(int dev, int ino)
{

    MINODE *mip;
    MTABLE *mp;
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];
    //printf("in miget\n");
    //search in-memory minodes first
    for(i=0;i<NMINODE;i++)
    {
        MINODE *mip = &minode[i];
        if(mip->refCount &&(mip->dev == dev) && (mip->ino==ino))
        {
            //printf("miget >>> success!\n");
            return mip;
        }
    }
    //printf("creating new mip...\n");
    mip = mialloc();
    mip->dev = dev;
    mip->ino = ino;
    block = (ino-1)/8 + iblock;
    offset = (ino-1)%8;
    get_block(dev,block,buf);
    ip = (INODE*)buf + offset;
    mip->INODE =*ip;

    mip->refCount = 1;
    mip->mounted = 0;
    mip->dirty = 0;
    mip->mptr = 0;
   // printf("new mip\n");
    return mip;
}




int mgetino(char*pathname)
{
    MINODE* mip;
    int i, ino, len;
    int p =222;
     printf("pathname:%s\n", pathname);  
    if(strcmp(pathname, "/") == 0){
        return 2; 
    }
 
    if(pathname[0] == "/"){
        mip = root;
    }
    else{
        mip = running ->cwd;
    }
    mip->refCount++;

    len = mtokenize(pathname);


    for(i=0;i<len;i++){
        if(!S_ISDIR(mip->INODE.i_mode)){
            printf("%s is not a dir%s\n", name[i]);
            miput(mip);
            return 0;
        }
        ino =  msearch(mip, name[i]);
        //printf("search success ino = %d\n",ino);
        if(!ino){
            miput(mip);
            return 0;
        }
     
    miput(mip);              //release current minode
    mip = miget(dev,ino);     //switch to new minode
    }

    miput(mip);
    //printf("get ino success %d\n",ino);
    return ino;
}

int msearch(MINODE *mip, char *name)
{
int i, t;
char *cp, temp[256], sbuf[BLKSIZE];
DIR* dp;
for(i=0;i<120;i++)
{
    if(mip->INODE.i_block[i]==0)
        return 0;

    get_block(mip->dev, mip->INODE.i_block[i], sbuf);
    dp = (DIR*)sbuf;
    cp = sbuf;
    while(cp<sbuf+BLKSIZE)
    {
        strncpy(temp, dp->name,dp->name_len);
        temp[dp->name_len]=0;
        printf("%8d%8d%8u %s\n",
            dp->inode, dp->rec_len, dp->name_len, temp);

        if(strcmp(name, temp)==0)
        {
            t = dp->inode;
            printf("found %s : inumber = %d\n", name, t);

            return t; 
        }
        cp+=dp->rec_len;
        dp = (DIR*)cp;
    }
}
printf("no inode exists\n");
return 0;
}





int midalloc(MINODE *mip)
{
    mip->refCount = 0;
}




int miput(MINODE *mip)
{
    INODE *ip;
    int i, block, offset;
    char buf[BLKSIZE];
   // printf("in miput\n");
    if(mip == 0)
    {
       // printf("miput >>> empty\n");
        return;
    } 
    mip->refCount --;

    if(mip->refCount>0) 
    {
       // printf("miput>>> still has user\n");
        return;
    }//still has user
    if(mip->dirty ==0) 
    {
       // printf("miput>>> dirty\n");
        return;
    } //no need to write back

    //write INODE to disk
    block = (mip->ino - 1)/8 + iblock;
    offset = (mip->ino -1)%8;

    //get block containing this node
    get_block(mip->dev, block, buf);
    ip = (INODE*)buf + offset;
    *ip = mip->INODE;
    put_block(mip->dev, block, buf);
    midalloc(mip);
    //printf("here");
    return;
}

int mtokenize(char *command)		//returns n number of tokens
{
   //commdand:%s\n", command);
   //gpath[0] = '\0';
   //name[0] = NULL;
   const char delim = '/';
   int n = 0, i = 0, k = 0, q = 0;	//number of tokens, size of pathname
   if(command[0] == delim)  q++;

   //printf("command:%s\n", command);
   strcpy(gpath, command + q);
   //printf("gpath = %s\n", gpath);
 

   name[n] = gpath;

   while(gpath[i] != '\0')    //terminates at null character
    {
     if(gpath[i] == delim)    //enter when gpath[i] is whitespace
     {
       
       gpath[i] = '\0';       //replace whitsepace with null character
       if(gpath[i+1]!='\0' && gpath[i+1]!=delim) 
       {
         n++;
         name[n] = gpath+i+1;  
       }
     }

     i++;
    }

   if(name[n+1])
     name[n+1] = NULL;
    printf("name[0] = %s\ngpath = %s\n", name[0], gpath);

   return n+1;
   //returns number of VALID tokens in name
   //name may still contain tokens past n
}


