int symlink(char*oldName, char* newName)
{
    //old file must exist and new file must not exist yet

    // newfile = a/b/c  old_file = x/y/z

    char parent[64], child[64];
    int ino, oino, cino;
    int numDirs;
    MINODE *mip, *omip, *cmip;
    printf("%s\n%s\n", newName, oldName);


    //verify we have enough arguments
    if(!newName ||!oldName)
    {
        printf("ERROR: not enough arguments\n");
        return 0;
    }

    numDirs = mtokenize(oldName);
    if(numDirs>1)   
    {
        
        strcpy(parent, name[0]);
        strcpy(child, name[numDirs-1]);
        for(int i = 1; i<numDirs-1;i++)
        {
            strcat(parent, "/");
            strcat(parent, name[i]);
        }

        dev = root->dev; 
        oino = getino(parent);
        if(oino ==0 )   {   printf("ERROR: %s does not exist\n",parent);return 0;  }
        omip = miget(dev, oino);
        if(!S_ISDIR(omip->INODE.i_mode))   {   printf("ERROR: %s is not a directory\n", parent);return 0;  }
        cino = mgetino(child);
        cmip = miget(dev, cino);
    }
    else    
    {
     printf("here");
        dev = running->cwd->dev;
        omip = running->cwd;
        cino = mgetino(newName);
        cmip = miget(dev, cino);
    }


    //verify source file 
    //if(oino ==0 )   {   printf("ERROR: %s does not exist\n",oldName);return 0;  }
   // omip = miget(dev, oino);



    //verify dest file does not already exist
    if(mgetino(newName))   {   printf("ERROR: file already exists\n"); return 0;}

    ino = mcreat(newName);

    mip = miget(dev,ino);
    printf("ino:%u\n", ino);
    //mip->INODE = cmip->INODE;

    mip->INODE.i_mode = 0120000;
    mip->INODE.i_size = strlen(oldName);

    mip->dirty = 1;
    miput(mip);
    miput(cmip);

    
    printf("we got this far\n");
    return;
}