int link(char *oldFile, char* newFile)
{
    // newfile = a/b/c  old_file = x/y/z
    //creates file z in x/y that is identical to a/b/c
    char newName[64], dest[64];
    int ino, oino;
    int numDirs;
    MINODE *mip, *omip;
    printf("%s\n%s\n", newFile, oldFile);

    dev = root->dev; 
    oino = mgetino(oldFile);

    if(!newFile ||!oldFile)
    {
        printf("ERROR: not enough arguments\n");
        return 0;
    }
    //verify source file exists and is not a directory
    if(oino ==0 )   {   printf("ERROR: %s does not exist\n",oldFile);return 0;  }
    omip = miget(dev, oino);
    if(S_ISDIR(omip->INODE.i_mode))   {   printf("ERROR: %s is a directory\n", oldFile);return 0;  }
    numDirs = mtokenize(newFile);


    //verify dest file does not already exist
    if(mgetino(newFile))   {   printf("ERROR: file already exists\n"); return 0;}


    //check x/y exists and is DIR but z does not yet exist in x/y
    if(numDirs>1)   
    {
        
        strcpy(dest, name[0]);
        strcpy(newName, name[numDirs-1]);
        for(int i = 1; i<numDirs-1;i++)
        {
            strcat(dest, "/");
            strcat(dest, name[i]);
        }
        dev = root->dev; 
        ino = mgetino(dest);
        if(ino == 0)    {   printf("ERROR: %s does not exist\n",dest);return 0;  }
        mip = miget(dev,ino);
        if(!S_ISDIR(mip->INODE.i_mode))   {   printf("ERROR: %s is not a directory\n", dest);return 0;  }
    }
    else
    {
        mip = running->cwd;
        strcpy(newName, newFile);
    }
    

    

    //add an entry to the data block of x/y which is the same ino as a/b/c
    printf("adding name%s\n", newName);
    enter_name(mip, oino,newName);

    //increment ilink count of INODE by 1
    mip->INODE.i_links_count++;
    mip->dirty = 1;

    //increment link count for the file being linked to
    omip->INODE.i_links_count++;
    omip->dirty = 1;
    //Write INODE back to disk
    miput(omip);
    miput(mip);


}

int unlink(char* pathname)
{

    int ino, pino;
    int numDirs;
    MINODE *mip, *pmip;
    char parent[64], child[64], temp[64];
    //get filesnames inode
    strcpy(temp, pathname);
    dev = root->dev;

    ino = mgetino(pathname);
    mip = miget(dev, ino);
    printf("got here\n");
    //remove name entry from parent DIR's data block
    numDirs = mtokenize(pathname);
    if(numDirs>1)   
    {
        
        strcpy(parent, name[0]);
        strcpy(child, name[numDirs-1]);
        for(int i = 1; i<numDirs-1;i++)
        {
            strcat(parent, "/");
            strcat(parent, name[i]);
        }
        pino = mgetino(parent);
        if(pino == 0)    {   printf("ERROR: %s does not exist\n",parent);return 0;  }
        pmip = miget(dev,pino);
        if(!S_ISDIR(pmip->INODE.i_mode))   {   printf("ERROR: %s is not a directory\n", parent);return 0;  }
    }
    else
    {
        pmip = running->cwd;
        strcpy(child, pathname);
        //return;
    }
    

    printf("got here\n");
    rm_child(pmip, child);
    pmip->dirty = 1;

    pmip->INODE.i_links_count--;
    miput(pmip);

    if(mip->INODE.i_links_count>0)
    {
        mip->dirty = 1;
    }
    else
    {
        for(int i = 0;i<12 &&mip->INODE.i_block[i];i++)
        {
            bdalloc(dev,mip->INODE.i_block[i]);
        }
    }
    mip->INODE.i_links_count--;
    miput(mip);

}

