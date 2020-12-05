
findname(MINODE *mip, int ino, char* name)
{
int i, t;
char *cp, temp[256], sbuf[BLKSIZE];
DIR* dp;
for(i=0;i<12;i++)
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
            
        if(dp->inode == ino)
        {
           // t = dp->inode;
            printf("found %s : inumber = %d\n", dp->name, dp->inode);
            strcpy(name,dp->name);
            name[dp->name_len] = '\0';
            return 1; 
        }
        cp+=dp->rec_len;
        dp = (DIR*)cp;
    }
}
printf("no inode exists\n");
return 0;
}



int incFreeInodes(int dev)
{
    char buf[BLKSIZE];
    //inc free inodes count in SUPER and GD
    get_block(dev,1,buf);
    sp = (SUPER*)buf;
    sp->s_free_inodes_count++;
    put_block(dev,1,buf);
    get_block(dev,2,buf);
    gp = (GD*)buf;
    gp->bg_free_inodes_count++;
    put_block(dev,2,buf);

}

int incFreeBlocks(int dev)
{
    char buf[BLKSIZE];
    //inc free inodes count in SUPER and GD
    get_block(dev,1,buf);
    sp = (SUPER*)buf;
    sp->s_free_blocks_count++;
    put_block(dev,1,buf);
    get_block(dev,2,buf);
    gp = (GD*)buf;
    gp->bg_free_blocks_count++;
    put_block(dev,2,buf);

}

clr_bit(char* buf, int bit)
{
    buf[bit/8] &= ~(1<<(bit%8));
}
idalloc(int dev, int ino)
{
    int i;
    char buf[BLKSIZE];
    MTABLE *mp = mtable;
    if(ino > mp->ninodes) //ninodes global
    {
        printf("inumber %d is out of range\n",ino);
        return;
    }
    get_block(dev, mp->imap, buf);
    clr_bit(buf, ino-1);
    //write buf back
    put_block(dev, mp->imap, buf);
    //update free inode count in super and gd
    incFreeInodes(dev);
}

//similar to above, but uses devices blocks bitmap and it increments the free block count in both superblock and groupdestrictor
bdalloc(int dev, int bno)
{
    printf("in here\n");
    int i;
    char buf[BLKSIZE];
    MTABLE *mp = mtable;
    
    if(bno > mp->nblocks) //ninodes global
     {
       printf("inumber %d is out of range\n",bno);
        return;
     }
    printf("in here\n");
    get_block(dev, mp->bmap, buf);
    clr_bit(buf, bno-1);
    //write buf back
    put_block(dev, mp->bmap, buf);
    //update free inode count in super and gd
    incFreeBlocks(dev);
}

int rm_child(MINODE* mip, char* name)
{
int i, t, k;
char *cp, *lc, temp[256], sbuf[BLKSIZE];
DIR *dp;
DIR *lp;       //dp is current lp is last directory
printf("searching for %s\n",name);
k = 0;


for(i=0;i<12;i++)
{
    if(mip->INODE.i_block[i]==0)
        return 0;

    get_block(dev, mip->INODE.i_block[i], sbuf);
    cp = sbuf;
    dp = (DIR*)sbuf;

    printf("in dp->%s\n",dp->name);

    while(cp<sbuf+BLKSIZE)
    {
        strncpy(temp, dp->name,dp->name_len);
        temp[dp->name_len]=0;
        printf("%8d%8d%8u %s\n",
             dp->inode, dp->rec_len, dp->name_len, temp);
            
   

        if(strcmp(name, temp)==0)
        {
             printf("found %s \n", name);
            // printf("dp->rec_len = %d\ncp=%d\n",dp->rec_len, cp);
            // printf("sbuf = %d\n",sbuf );
           //  printf("buf - cp  = %d\n", sbuf-cp);
            // printf("does |%d = %d |\n",(cp+dp->rec_len),(sbuf + 1024));


             //case 1: first and only entry
            if(k==2)
            {
                        
                printf("case 1 i = %u dp->name =%s \n",i, dp->name);
                //dealloc the block
                idalloc(mip->dev, mip->INODE.i_block[i]);
                bdalloc(mip->dev, mip->INODE.i_block[i]);
                //reduce parents file size by blcksize;
                mip->INODE.i_size -= BLKSIZE;;
                
                printf("mip next %u\n", mip->INODE.i_block[i+1]);
                //compact parents i_block[] array to eliminate entry if its between non zero entries
                for(int t = 2;((t) <12);t++)
                {
                    

                    printf("t = %u\n", t);
                    get_block(mip->dev, mip->INODE.i_block[t], sbuf);
                    put_block(mip->dev, mip->INODE.i_block[t-1], sbuf);
                    
                }
            //get_block(mip->dev, mip->INODE.i_block[i], sbuf);
            //put_block(mip->dev, mip->INODE.i_block[i], sbuf);
            mip->dirty = 1;
            miput(mip);
            return;
            }
             //case 2: last entry in block
            else if(cp+dp->rec_len ==sbuf+BLKSIZE)
            {
                printf("case 2 k = %u name found %s\n",k, dp->name);
                //absorb dirs rec_len into its predecessors 
                printf("lp->name = %s, rec_len = %u\n",lp->name, lp->rec_len);
                lp->rec_len += dp->rec_len;
                
                printf("lp->name = %s, rec_len = %u\n",lp->name, lp->rec_len);
                
                put_block(mip->dev, mip->INODE.i_block[i], sbuf);
                mip->dirty = 1;
                miput(mip);
                return;
            }

             //case 3: all else
             else
             {
                 printf("too bad~\n");
                 return;
               //move all entries left to overlay the deleted entry
            					//not last entry, this is where we have problems

               //add deleted rec_len to the LAST entry do not change parents size
             }
             
            
            // return; 
         }
        lp = dp;
        cp+=dp->rec_len;
        dp = (DIR*)cp;
        k++;
        if(k>22)
         {return;}
    }
}
printf("no inode exists\n");
return 0;
}
int rmdir(char* pathname)
{
    int numDirectories = mtokenize(pathname);
    int ino, pino;
    MINODE *mip, *pmip;
    //get inumber of pathname
    char parent[64];
    char child[64];
    char ipName[64];

    strcpy(child, name[numDirectories-1]);
    ino = mgetino(pathname);
    mip = miget(dev, ino);

    if(numDirectories>1)   
    {
        printf("num dir = %u\n", numDirectories);
        strcpy(parent, name[0]);
        for(int i = 1; i<numDirectories-1;i++)
        {
            strcat(parent, "/");
            strcat(parent, name[i]);
        }

        pino = mgetino(parent);
        pmip = miget(mip->dev, pino);
    }
    else    
    {
        printf("cwd rmdir\n");
        pmip = running->cwd;    
        if(pmip == root) printf("in root\n");
    }

    if(!S_ISDIR(mip->INODE.i_mode))   {   printf("ERROR: %s is not a directory\n",child);  return 0;  }

    //verify DIR is empty
    if(mip->INODE.i_links_count>2)
    {
        printf("ERROR: unable to delete a non-empty directory\n");
        return 0;
    }

    //get name from parent DIRS block
    findname(pmip, ino, ipName);


    //deallocate its data blocks and inode
    for(int i = 0; i<15 && mip->INODE.i_block[i] !=0;i++)
    {
        bdalloc(mip->dev, mip->INODE.i_block[i]);
    }
    idalloc(mip->dev, mip->ino);



    //remove name from parrent DIR
    rm_child(pmip, ipName);

    //dec parents link_count by 1; mark parent pimp dirt;
    pmip->INODE.i_links_count--;
    pmip->dirty = 1;
    
    miput(pmip);


    mip->dirty = 1;
    miput(mip);


    
}