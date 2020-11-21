
int tst_bit(char *buf, int bit)
{
    //in chapter 11.31
    return buf[bit/8] & (1<<(bit%8));
}


int set_bit(char *buf, int bit)
{
//in chapter 11.3.1
   return buf[bit/8] |= (1<<(bit %8));
}

int decFreeInodes(int dev)
{
    //count in SUPER and GD
    char buf[BLKSIZE];
    get_block(dev,1,buf);
    sp = (SUPER*)buf;
    sp->s_free_inodes_count--;
    put_block(dev,1,buf);
    get_block(dev,2,buf);
    gp = (GD*)buf;
    gp->bg_free_inodes_count--;
    put_block(dev,2,buf);
}


int ialloc(int dev) //alocate an inode number from inode_bitmap
{
    int i,j;
    char buf[BLKSIZE];

    //read inode_bitmap block
    get_block(dev, imap, buf);

    for(i=0; i<ninodes; i++)
    {
        if(tst_bit(buf, i)==0)
        {
            set_bit(buf,i);
            //update free inode count
            put_block(dev, imap, buf);
            printf("allocated ino =%d\nblock = %u\n", i+1); //bits count from 0 inode from 1
            decFreeInodes(dev);
            return i +1;
        }
    }
    return 0;   //out of free inodes;
}

//write your own Balloc functon which returns a free disk block number

int balloc(int dev)
{
    printf("in balloc\n");

    int i;
    char buf[BLKSIZE];
    printf("bmap is %d\n", bmap);

    get_block(dev, bmap, buf);

    for(i = 0; i<nblocks;i++)
    {
        if(tst_bit(buf,i)==0)
        {
            set_bit(buf,i);
            put_block(dev, bmap, buf);
            printf("bmap is %d\n", bmap);
            decFreeInodes(dev);
            return i;
        }
    }
    printf("ERROR: Failed to balloc\n");
    return 0;   //out of free block space
}


int enter_name(MINODE *pip, int ino, char *newname)
{

    char buf[BLKSIZE];
    DIR *dp;
    char*cp;
    int ideal_length, need_length, remain;

    MINODE* parent = pip;


    for(int i =0; i<=pip->INODE.i_blocks;i++)
    {   
        if(pip->INODE.i_block[i] ==0 ) {   goto allocBlock; }
        printf("name = %s",newname);
        //1 get parents data block into buf
        get_block(dev,pip->INODE.i_block[i]);



        //2 in a data block of the parent dir, each dir_entry has an ideal length
        //  ideal_len = 4*[(8+name_len + 3)/4]

        //3 in order to enter a new entry of name with n_len, the needed length is
        //  need_length = 4*[(8+n_len + 3)/4]
        need_length = 4*((8+strlen(newname) + 3)/4);

        printf("ideal length = %u   need length = %u\n", ideal_length, need_length);
        //4 step into last entry of data block
        get_block(parent->dev, parent->INODE.i_block[i], buf);
        dp = (DIR*) buf;
        cp = buf;

        while(cp+dp->rec_len<buf +BLKSIZE)
        {
            cp+=dp->rec_len;
            dp = (DIR*)cp;
        }
        //dp now points at last entry in the block
        ideal_length = 4*((8+dp->name_len +3)/4);

        remain = dp->rec_len - ideal_length;

        printf("name = %s",newname);
        if(remain>=need_length)
        {
            printf("in last step... i = %u\n",i);
            //enter the new entry as the last entry and trim the previous entry reclen to its ideal rec_len

            dp->rec_len = ideal_length;
            cp+=dp->rec_len;
            dp = (DIR*)cp;

            dp->inode = ino;
            dp->rec_len = BLKSIZE - ((u32)cp - (u32)buf);
            dp->name_len = strlen(newname);
            dp->file_type = EXT2_FT_DIR;
            strcpy(dp->name, newname);


            put_block(dev, pip->INODE.i_block[i],buf);
            printf("name = %s, dp->name = %s\n",newname, dp->name);

            return 1;
        }

    }


    allocBlock:
        printf("\nin allocBlock\n");

        return -1;
}

int myCreat(MINODE* pip, char* newDir)
{
    MINODE* mip;
    INODE *ip;
    int ino, bno;
    char buf[BLKSIZE];

    //allocate an inode and a disk block for the new directory
    ino = ialloc(dev);
    //bno = balloc(dev);
    printf("ino = %u    bno = %u\n",ino, bno);
    
    mip = miget(dev, ino);      //load the inode into minode[] to write contents into memory
    ip = &mip->INODE;

    ip->i_mode = 0x0644;
    ip->i_uid = running->uid;
    ip->i_gid = running->gid;
    ip->i_size = 0;
    ip->i_links_count = 1;   
    ip->i_atime = time(0L);
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);
    ip->i_blocks = 2;
    ip->i_block[0] = bno;
    ip->i_block[1] = 0;
    ip->i_block[2] = 0;
    ip->i_block[3] = 0;
    ip->i_block[4] = 0;
    ip->i_block[5] = 0;
    ip->i_block[6] = 0;
    ip->i_block[7] = 0;
    ip->i_block[8] = 0;
    ip->i_block[9] = 0;
    ip->i_block[10] = 0;
    ip->i_block[11] = 0;
    ip->i_block[12] = 0;
    ip->i_block[13] = 0;
    ip->i_block[14] = 0;
    mip->dirty = 1;
    miput(mip);

    put_block(dev, bno, buf);
    get_block(dev, bno, buf);
    
    bzero(buf, BLKSIZE);    //optional, clear buf[] to 0;
    dp = (DIR*)buf;
    //make '.' entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    //make '..' entry
    //pino = parent DIR ino, blk = allocated block
    dp = (char*) dp+12;
    dp->inode = ino;
    dp->rec_len = BLKSIZE -12;  //rec-len spans block
    dp->name_len =2;
    dp->name[0] = '.';
    dp->name[1] = '.';

    put_block(dev,bno,buf); //write to blk on disk

    enter_name(pip, ino, newDir);

    return ino;
}
int mymkdir(MINODE* pip, char* newDir)
{
    MINODE* mip;
    INODE *ip;
    int ino, bno;
    char buf[BLKSIZE];

    //allocate an inode and a disk block for the new directory
    ino = ialloc(dev);
    bno = balloc(dev);
    printf("ino = %u    bno = %u\n",ino, bno);
    
    mip = miget(dev, ino);      //load the inode into minode[] to write contents into memory
    ip = &mip->INODE;

    ip->i_mode = 0x41ED;
    ip->i_uid = running->uid;
    ip->i_gid = running->gid;
    ip->i_size = BLKSIZE;
    ip->i_links_count = 2;      // '.' and '..'
    ip->i_atime = time(0L);
    ip->i_ctime = time(0L);
    ip->i_mtime = time(0L);
    ip->i_blocks = 2;
    ip->i_block[0] = bno;
    ip->i_block[1] = 0;
    ip->i_block[2] = 0;
    ip->i_block[3] = 0;
    ip->i_block[4] = 0;
    ip->i_block[5] = 0;
    ip->i_block[6] = 0;
    ip->i_block[7] = 0;
    ip->i_block[8] = 0;
    ip->i_block[9] = 0;
    ip->i_block[10] = 0;
    ip->i_block[11] = 0;
    ip->i_block[12] = 0;
    ip->i_block[13] = 0;
    ip->i_block[14] = 0;
    mip->dirty = 1;
    miput(mip);

    //put_block(dev, bno, buf);
    get_block(dev, bno, buf);
    
    bzero(buf, BLKSIZE);    //optional, clear buf[] to 0;
    dp = (DIR*)buf;
    //make '.' entry
    dp->inode = ino;
    dp->rec_len = 12;
    dp->name_len = 1;
    dp->name[0] = '.';
    //make '..' entry
    //pino = parent DIR ino, blk = allocated block
    dp = (char*) dp+12;
    dp->inode = ino;
    dp->rec_len = BLKSIZE -12;  //rec-len spans block
    dp->name_len =2;
    dp->name[0] = '.';
    dp->name[1] = '.';

    put_block(dev,bno,buf); //write to blk on disk

    enter_name(pip, ino, newDir);

    return 1;
}

int make_dir(char* pathname)
{
    MINODE *start;
    char parent [64];
    int numParentDirs = 0;
    int pino, cino;
    MINODE *pip;


    numParentDirs = mtokenize(pathname);
//1.
//pathname = absolute: start = root;    // relative: start = running->cwd
    if(numParentDirs>1)   
    {
        
        strcpy(parent, name[0]);
        for(int i = 1; i<numParentDirs-1;i++)
        {
            strcat(parent, "/");
            strcat(parent, name[i]);
        }

        dev = root->dev; 
        pino = getino(parent);
        if(pino ==0 )   {   printf("ERROR: %s does not exist\n",parent);return 0;  }
        pip = miget(dev, pino);
        if(!S_ISDIR(pip->INODE.i_mode))   {   printf("ERROR: %s is not a directory\n", parent);return 0;  }
    }
    else    
    {
        dev = running->cwd->dev;
        pip = running->cwd;
    }

    cino = getino(pathname);
    if(cino !=0 )    {   printf("ERROR: %s already exists\n", name[numParentDirs-1]);return 0;}

    if(pip == root)
        printf("in root\n");
    printf("making new directory: %s\n", name[numParentDirs-1]);
    mymkdir(pip,name[numParentDirs-1]);
    pip->INODE.i_links_count ++;
    pip ->INODE.i_atime = time(0L);
    pip->dirty = 1;
    miput(pip);
    printf("success\n");
    return 1;
}


int mcreat(char* pathname)
{
    MINODE *start;
    char parent [64];
    int numParentDirs = 0;
    int pino, cino, ret;
    MINODE *pip;


    numParentDirs = mtokenize(pathname);
//1.
//pathname = absolute: start = root;    // relative: start = running->cwd
    if(numParentDirs>1)   
    {
        
        strcpy(parent, name[0]);
        for(int i = 1; i<numParentDirs-1;i++)
        {
            strcat(parent, "/");
            strcat(parent, name[i]);
        }

        dev = root->dev; 
        pino = getino(parent);
        if(pino ==0 )   {   printf("ERROR: %s does not exist\n",parent);return 0;  }
        pip = miget(dev, pino);
        if(!S_ISDIR(pip->INODE.i_mode))   {   printf("ERROR: %s is not a directory\n", parent);return 0;  }
    }
    else    
    {
        dev = running->cwd->dev;
        pip = running->cwd;
    }

    cino = getino(pathname);
    if(cino !=0 )    {   printf("ERROR: %s already exists\n", name[numParentDirs-1]);return 0;}

    if(pip == root)
        printf("in root\n");
    printf("making new file: %s\n", name[numParentDirs-1]);
    ret = myCreat(pip,name[numParentDirs-1]);
   // pip->INODE.i_links_count ++;
    pip ->INODE.i_atime = time(0L);
    pip->dirty = 1;
    miput(pip);
    printf("success\n");
    return ret;


}