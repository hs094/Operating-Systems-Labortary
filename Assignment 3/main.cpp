#include <bits/stdc++.h>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/wait.h>
#include <vector>
#include <utility>
#include <string>
using namespace std;

int main(int argc, char *argv[])
{
    srand((unsigned long int)time(NULL));
    /*
        Start of Main Process
    */
    // Create shared memory
    char *shm, *s;
    // Create the segment of size 2MB
    int shmid_mem = shmget(200, 2 * 1024 * 1024, 0666 | IPC_CREAT);
    if (shmid_mem < 0)
    {
        perror("shmget");
        exit(1);
    }
    shm = (char *)shmat(shmid_mem, NULL, 0);
    if (shm == (char *)-1)
    {
        perror("shmat");
        shmctl(shmid_mem, IPC_RMID, NULL);
        exit(1);
    }

    // Now we open the file and load the initial graph into the shared memory
    // We also load the number of vertices and edges into the shared memory
    FILE *fp;
    fp = fopen("facebook_combined.txt", "r");
    if (fp == NULL)
    {
        perror("fopen");
        shmctl(shmid_mem, IPC_RMID, NULL);
        exit(1);
    }
    int n = 0;
    vector<pair<int, int>> edges;
    int u, v;
    while (fscanf(fp, "%d %d", &u, &v) != EOF)
    {
        edges.push_back(make_pair(u, v));
        n = max(n, max(u, v));
    }
    fclose(fp);
    // Now we have the edges in the vector
    // We need to load the edges into the shared memory
    // We also need to load the number of vertices and edges into the shared memory
    // We will load the number of vertices and edges in the first 2 integers
    // We will load the edges in the next 2*edges.size() integers
    int *shm_int = (int *)shm;
    shm_int[0] = 2 + edges.size()*2;
    shm_int[1] = n + 1;
    shm_int[2] = 3;
    map<int,int> degree;
    for (int i = 0; i < edges.size(); i++)
    {
        shm_int[2 * i + 3] = edges[i].first;
        shm_int[2 * i + 4] = edges[i].second;
        degree[edges[i].first]++;
        degree[edges[i].second]++;
    }
    /*
        End of Main Process
    */
    cout << shm_int[0] << " " << shm_int[1] << " " << shm_int[2] << endl;
    for(int i=3;i<shm_int[0];i+=2)
        cout << shm_int[i] << " " << shm_int[i+1] << endl;
    shmdt(shm_int);
    // shmctl(shmid_mem,IPC_RMID, NULL);
    return 0;
}
