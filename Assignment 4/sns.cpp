#include <bits/stdc++.h>
#include <errno.h>
#include <execinfo.h>
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

#include <cassert>
#include <chrono>
#include <cstring>
#include <iostream>
#include <vector>

using namespace std;

#define COLOR_RED "\033[1;31m"
#define COLOR_GREEN "\033[1;32m"
#define COLOR_BLUE "\033[1;34m"
#define COLOR_RESET "\033[0m"

string filename = "musae_git_edges.csv";

// Returns a random number between low and high
int rand(int low, int high)
{
    return low + rand() % (high - low + 1);
}

// A wrapper around pthread_mutex_lock for error detection
void LOCK(pthread_mutex_t *mutex)
{
    int status = pthread_mutex_lock(mutex);
    if (status != 0)
    {
        printf(COLOR_RED "pthread_mutex_lock failed: %s\n" COLOR_RESET, strerror(status));
        exit(1);
    }
}

// A wrapper around pthread_mutex_unlock for error detection
void UNLOCK(pthread_mutex_t *mutex)
{
    int status = pthread_mutex_unlock(mutex);
    if (status != 0)
    {
        printf(COLOR_RED "pthread_mutex_unlock failed: %s\n" COLOR_RESET, strerror(status));
        exit(1);
    }
}

typedef struct
{
    int user_id;        // It is just the node_id for the node
    int action_id;      // Additional Information about 4th like, 5th post
    string action_type; // one of "post, comment, like"
    time_t timestamp;   // Simple Unix/Linux Timestamp

    void Action(int user_id, int action_id, string action_type)
    {
        this->user_id = user_id;
        this->action_id = action_id;
        this->action_type = action_type;
    }
} Action;

typedef struct
{
    int cnt_post;
    int cnt_comment;
    int cnt_like;
    deque<Action> actl;
    void ActionQueue()
    {
        this->cnt_comment = 0;
        this->cnt_like = 0;
        this->cnt_post = 0;
        actl.clear();
    }
} ActionQueue;

typedef struct
{
    int id; // node id
    int degree;
    int order; // Encoded as 0 <- priority and 1 <- chronological (To be Set at Random)
    map<int,int> neighbours; // Hashmap to store the neighbours of the given Node
    ActionQueue Wall;  // Action by U itself
    ActionQueue Feed; //  Action by All it's Neighbours
    void init(int id)
    {
        this->id = id;
        this->degree = 0;
        this->neighbours.clear();
        this->order = rand()%2;
        this->Wall.ActionQueue();
        this->Feed.ActionQueue();
    }
    void addEdge(int neigh_id)
    {
        this->degree++;
        this->neighbours[neigh_id] = 1;
    }
} Node;

signed main()
{
    // Open the CSV file for reading
    ifstream infile(filename);
    map<int,set<int>> test;
    map<int, Node> graph;
    // Read the CSV file line by line
    string line;
    int i = 0, id1, id2;
    while (getline(infile, line)) {
        // Split the line by commas
        if(i == 0)
        {
            i++;
            continue;
        }
        sscanf(line.c_str(), "%d,%d", &id1, &id2);
        i++;
        // Add the ids to the vector
        Node n1, n2;
        if(graph.count(id1)==0)
        {
            n1.init(id1);
            graph[id1] = n1;
        }
        else
            n1 = graph[id1];
        if(graph.count(id2)==0)
        {
            n2.init(id2);
            graph[id2] = n2;
        }
        else
            n2 = graph[id2];
        n1.addEdge(id2);
        graph[id1] = n1; 
        n2.addEdge(id1);
        graph[id2] = n2;
        test[id1].insert(id2);
        test[id2].insert(id1);
    }
    infile.close();
    ofstream file1("output_graph.txt");
    for(auto it: graph)
    {
        file1 << it.first << ": ";
        for(auto x: it.second.neighbours)
        {
            file1 << x.first << " ";
        }
        file1 << endl;
    }
    file1.close();
    ofstream file2("output_test.txt");
    for(auto it: test)
    {
        file2 << it.first << ": ";
        for(auto x: it.second)
        {
            file2 << x << " ";
        }
        file2 << endl;
    }
    return 0;
}