#include <bits/stdc++.h>
#include <fstream>
#include <cstring>
#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <fstream>
#include <sstream>
#include <vector>
#include <map>
#include <set>
#include <random>
#include <chrono>
#include <cstring>
#include <unistd.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
using namespace std;
#define MAX_NUM_NODES 90000
vector<vector<int>> g(MAX_NUM_NODES);

// void print_dist_vec(vector<vector<int>> &dist)
// {
//     ofstream file("output4.txt");
//     if (!file.is_open())
//     {
//         cerr << "Failed to open output file "
//              << "output4.txt" << endl;
//         exit(1);
//     }
//     for (int i = 0; i < dist.size(); i++)
//     {
//         file << "dist[" << i << "]:";
//         for (auto x : dist[i])
//             file << x << " ";
//         file << endl;
//     }
//     file.close();
// }

// // djikstra algorithm to find shortest path from source to all other nodes in a graph with non-negative edge weights using adjacency list representation of graph
// vector<int> djikstra(int src, int n)
// {
//     vector<int> dist(n + 1, INT_MAX);
//     dist[src] = 0;

//     priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
//     pq.push({0, src});

//     while (!pq.empty())
//     {
//         int u = pq.top().second;
//         pq.pop();

//         for (int v : g[u])
//         {
//             if (dist[v] > dist[u] + 1)
//             {
//                 dist[v] = dist[u] + 1;
//                 pq.push({dist[v], v});
//             }
//         }
//     }
//     for (int i = 0; i <= n; i++)
//         if (dist[i] == INT_MAX)
//             dist[i] = -1;
//     return dist;
// }

// void update_distance(vector<int> &dist, int u, int v)
// {
//     queue<int> q, temp;
//     q.push(v);
//     dist[v] = dist[u] + 1;
//     int val = 2;
//     while (!q.empty())
//     {
//         int x = q.front();
//         q.pop();
//         for (int y : g[x])
//         {
//             if (dist[y] == -1)
//             {
//                 dist[y] = dist[u] + val;
//                 q.push(y);
//             }
//             else if (dist[y] > dist[u] + val)
//             {
//                 dist[y] = dist[u] + val;
//                 temp.push(y);
//             }
//         }
//         if (q.empty())
//         {
//             q = temp;
//             temp = queue<int>();
//             val++;
//         }
//     }
// }

// void add_more_edges(vector<pair<int, int>> &new_edges)
// {
//     fstream file;
//     string word, t, q, filename;
//     int u, v;

//     new_edges.clear();
//     filename = "new_nodes.txt";
//     file.open(filename.c_str());
//     while (file >> word)
//     {
//         // cout<<word<<endl;
//         u = stoi(word);
//         file >> word;
//         v = stoi(word);

//         g[u].push_back(v);
//         g[v].push_back(u);
//         new_edges.push_back({u, v});
//     }
//     file.close();
// }

// void update_dist_vec(vector<vector<int>> &dist, vector<pair<int, int>> &new_edges)
// {
//     for (int i = 0; i < dist.size(); i++)
//     {
//         for (auto x : new_edges)
//         {
//             if (dist[i][x.first] >= 0 || dist[i][x.second] >= 0)
//             {
//                 int a, b;
//                 if (dist[i][x.first] < 0)
//                 {
//                     a = x.second;
//                     b = x.first;
//                 }
//                 else if (dist[i][x.second] < 0)
//                 {
//                     a = x.first;
//                     b = x.second;
//                 }
//                 else if (dist[i][x.first] >= 0 && ((dist[i][x.first] + 1) < dist[i][x.second]))
//                 {
//                     a = x.first;
//                     b = x.second;
//                 }
//                 else if (dist[i][x.second] >= 0 && ((dist[i][x.second] + 1) < dist[i][x.first]))
//                 {
//                     a = x.second;
//                     b = x.first;
//                 }
//                 else
//                     continue;

//                 cout << "i:" << i << " a:" << a << " b:" << b << endl;
//                 update_distance(dist[i], a, b);
//             }
//         }
//     }
// }
void print_dist_vec(vector<int> &dist, vector<vector<int>> &path, int n, string s)
{
    std::ofstream file;
    file.open(s.c_str(), std::ios_base::app);
    if (!file.is_open())
    {
        cerr << "Failed to open output file "
             << s << endl;
        exit(1);
    }
    for (int i = 0; i <= n; i++)
    {
        file << i << ":" << dist[i] << ": ";
        for (int j = 0; j < path[i].size(); j++)
            file << path[i][j] << " ";
        file << endl;
    }
}

// djikstra algorithm to find shortest path from source to all other nodes in a graph with non-negative edge weights using adjacency list representation of graph
vector<int> djikstra(int src, int n)
{
    vector<int> dist(n + 1, INT_MAX);
    dist[src] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    pq.push({0, src});

    while (!pq.empty())
    {
        int u = pq.top().second;
        pq.pop();

        for (int v : g[u])
        {
            if (dist[v] > dist[u] + 1)
            {
                dist[v] = dist[u] + 1;
                pq.push({dist[v], v});
            }
        }
    }
    for (int i = 0; i <= n; i++)
        if (dist[i] == INT_MAX)
            dist[i] = -1;
    return dist;
}

// djikstra algorithm to find shortest path from source to all other nodes in a graph with non-negative edge weights using adjacency list representation of graph while saving the path in parameter vector of vectors
vector<int> djikstra_with_path(int src, int n, vector<vector<int>> &path)
{
    vector<int> dist(n + 1, INT_MAX);
    path.clear();
    dist[src] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    pq.push({0, src});

    while (!pq.empty())
    {
        int u = pq.top().second;
        pq.pop();

        for (int v : g[u])
        {
            if (dist[v] > dist[u] + 1)
            {
                dist[v] = dist[u] + 1;
                pq.push({dist[v], v});

                path[v].clear();
                path[v] = path[u];
                path[v].push_back(u);
            }
        }
    }
    for (int i = 0; i <= n; i++)
        if (dist[i] == INT_MAX)
            dist[i] = -1;
        else
            path[i].push_back(i);
    return dist;
}

// get path from source to destination using dfs in a graph with non-negative edge weights using adjacency list representation of graph
vector<int> get_path(int src, int dest, int n)
{
    vector<int> dist(n + 1, INT_MAX);
    vector<vector<int>> path(n + 1);
    dist[src] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int>>, greater<pair<int, int>>> pq;
    pq.push({0, src});

    while (!pq.empty())
    {
        int u = pq.top().second;
        pq.pop();

        for (int v : g[u])
        {
            if (dist[v] > dist[u] + 1)
            {
                dist[v] = dist[u] + 1;
                pq.push({dist[v], v});

                path[v].clear();
                path[v] = path[u];
                path[v].push_back(u);
            }
        }
    }
    for (int i = 0; i <= n; i++)
        if (dist[i] == INT_MAX)
            dist[i] = -1;
        else
            path[i].push_back(i);
    return path[dest];
}

void update_distance(vector<int> &dist, vector<vector<int>> &path, int u, int v)
{
    int src = path[u][0];
    queue<pair<int, int>> q, temp;
    q.push({v, u});
    dist[v] = dist[u] + 1;
    int val = 2;
    while (!q.empty())
    {
        int x = q.front().first;
        int h = q.front().second;

        path[x].clear();
        path[x] = path[h];
        path[x].push_back(h);
        path[x].push_back(x);
        path[x] = get_path(src, x, g.size() - 1);

        q.pop();
        for (auto y : g[x])
        {
            if (dist[y] == -1)
            {
                dist[y] = dist[u] + val;
                path[y].clear();
                path[y] = path[x];
                path[y].push_back(y);
                q.push({y, x});
            }
            else if (dist[y] > dist[u] + val)
            {
                dist[y] = dist[u] + val;
                path[y].clear();
                path[y] = path[x];
                path[y].push_back(y);
                temp.push({y, x});
            }
        }
        if (q.empty())
        {
            q = temp;
            temp = queue<pair<int, int>>();
            val++;
        }
    }
}

void get_initial_graph(int &n, int &m)
{
    int la;
    fstream file;
    string word, t, q, filename;

    filename = "file.txt";
    file.open(filename.c_str());
    int lno = 0;

    file >> word;
    m = stoi(word);

    file >> word;
    n = stoi(word);

    file >> word;
    la = stoi(word);

    g.clear();
    g.resize(n + 1);

    int e = 0, u, v;
    while (e <= m)
    {
        file >> word;
        u = stoi(word);

        file >> word;
        v = stoi(word);

        g[u].push_back(v);
        g[v].push_back(u);
        e++;
    }
    file.close();
}

void add_more_edges(vector<pair<int, int>> &new_edges)
{
    fstream file;
    string word, t, q, filename;
    int u, v;

    new_edges.clear();
    filename = "new_nodes.txt";
    file.open(filename.c_str());
    while (file >> word)
    {
        // cout<<word<<endl;
        u = stoi(word);
        file >> word;
        v = stoi(word);

        g[u].push_back(v);
        g[v].push_back(u);
        new_edges.push_back({u, v});
    }
    file.close();
}

void update_dist_vec(vector<vector<int>> &dist, vector<vector<vector<int>>> &path, vector<pair<int, int>> &new_edges)
{
    for (int i = 0; i < dist.size(); i++)
    {
        for (auto x : new_edges)
        {
            if (dist[i][x.first] >= 0 || dist[i][x.second] >= 0)
            {
                int a, b;
                if (dist[i][x.first] < 0)
                {
                    a = x.second;
                    b = x.first;
                }
                else if (dist[i][x.second] < 0)
                {
                    a = x.first;
                    b = x.second;
                }
                else if (dist[i][x.first] >= 0 && ((dist[i][x.first] + 1) < dist[i][x.second]))
                {
                    a = x.first;
                    b = x.second;
                }
                else if (dist[i][x.second] >= 0 && ((dist[i][x.second] + 1) < dist[i][x.first]))
                {
                    a = x.second;
                    b = x.first;
                }
                else
                    continue;

                cout << "i:" << i << " a:" << a << " b:" << b << endl;
                update_distance(dist[i], path[i], a, b);
            }
        }
    }
}
int main(int argc, char *argv[])
{
    if (argc > 2)
    {
        cout << "Usage: ./main [-optimize]" << endl;
        exit(1);
    }
    if (argc == 2)
    {
        string s = argv[1];
        if (s != "-optimize")
        {
            cout << "Usage: ./main [-optimize]" << endl;
            exit(1);
        }
    }
    bool optimize = false;
    if (argc == 2)
    {
        optimize = true;
    }
    srand(time(NULL));
    int shm_id = shmget(200, 2 * 1024 * 1024, 0666);
    if (shm_id < 0)
    {
        cout << "Error: could not create shared memory segment." << endl;
        return 1;
    }
    if (optimize)
    {
        // Optimised
        g.clear();
        void *shm_ptr = shmat(shm_id, NULL, 0);
        if (shm_ptr == (void *)-1)
        {
            cout << "Error: could not attach shared memory segment." << endl;
            return 1;
        }
        // cast the pointer to a pointer of Node pointers
        int *shm_int_p = static_cast<int *>(shm_ptr);
        size_t NUM_NODES = (shm_int_p[1]);
        size_t NUM_EDGES = ((shm_int_p[0] - 3) / 2);
        for (int i = g.size(); i < shm_int_p[1]; i++)
            g.push_back(vector<int>());
        ofstream file("output3.txt");
        if (!file.is_open())
        {
            cerr << "Failed to open output file "
                 << "output3.txt" << endl;
            exit(1);
        }
        file << shm_int_p[0] << " " << shm_int_p[1] << " " << shm_int_p[2] << endl;
        for (int i = shm_int_p[2]; i < shm_int_p[0]; i += 2)
        {
            g[shm_int_p[i]].push_back(shm_int_p[i + 1]);
            g[shm_int_p[i + 1]].push_back(shm_int_p[i]);
            file << shm_int_p[i] << " " << shm_int_p[i + 1] << endl;
        }
        shm_int_p[2] = shm_int_p[0];
        shmdt(shm_ptr);
        vector<vector<int>> dist(1);
        dist[0] = djikstra(0, MAX_NUM_NODES);
        // print_dist_vec(dist);
        file << endl;
        file.close();
        while (1)
        {
            sleep(5);
            void *shm_ptr_p = shmat(shm_id, NULL, 0);
            if (shm_ptr_p == (void *)-1)
            {
                cout << "Error: could not attach shared memory segment." << endl;
                return 1;
            }
            // cast the pointer to a pointer of Node pointers
            int *shm_int = static_cast<int *>(shm_ptr_p);
            if (fork() == 0)
            {
                size_t NUM_NODES = (shm_int[1]);
                size_t NUM_EDGES = ((shm_int[0] - 3) / 2);
                ofstream file("output3.txt");
                if (!file.is_open())
                {
                    cerr << "Failed to open output file "
                         << "output3.txt" << endl;
                    exit(1);
                }
                file << shm_int[0] << " " << shm_int[1] << " " << shm_int[2] << endl;
                vector<pair<int, int>> new_edges;
                for (int i = shm_int[2]; i < shm_int[0]; i += 2)
                {
                    g[shm_int[i]].push_back(shm_int[i + 1]);
                    g[shm_int[i + 1]].push_back(shm_int[i]);
                    if (i >= shm_int[2])
                        new_edges.push_back({shm_int[i], shm_int[i + 1]});
                }
                for (int i = 0; i < g.size(); i++)
                {
                    file << i << ": ";
                    for (int j = 0; j < g[i].size(); j++)
                    {
                        file << g[i][j] << " ";
                    }
                    file << endl;
                }
                shm_int[2] = shm_int[0] + 1;
                int n = NUM_NODES, m = NUM_EDGES;
                for (int i = 1; i <= 10; i++)
                {
                    if (fork() == 0)
                    {
                        string s = "./output/graph" + to_string(i) + ".txt";
                        for (int j = 0; j < n / 10; i++)
                        {
                            // vector<vector<int>> dist(1);
                            // dist[0] = djikstra(j + (i - 1) * (n / 10), n);
                            // print_dist_vec(dist, j + (i - 1) * (n / 10), s)
                            int N = 1;
                            vector<vector<int>> dist(N);
                            vector<vector<vector<int>>> path(N, vector<vector<int>>(n + 1));
                            print_dist_vec(dist[0], path[0], NUM_NODES, s);
                            dist[0] = djikstra_with_path(j + (i - 1) * (n / 10), n, path[0]);
                            print_dist_vec(dist[0], path[0], n, s);
                            add_more_edges(new_edges);
                            cout << "new_edges: " << new_edges.size() << endl;
                            update_dist_vec(dist, path, new_edges);
                        }
                    }
                    while (wait(NULL) > 0)
                    {
                        ;
                    }
                }
                shmdt(shm_ptr_p);
                exit(0);
            }
            while (wait(NULL) > 0)
            {
                ;
            }
            shmdt(shm_ptr_p);
        }
    }
    else
    {
        // Unoptimized Version
        g.clear();
        while (1)
        {
            sleep(5);
            void *shm_ptr_p = shmat(shm_id, NULL, 0);
            if (shm_ptr_p == (void *)-1)
            {
                cout << "Error: could not attach shared memory segment." << endl;
                return 1;
            }
            // cast the pointer to a pointer of Node pointers
            int *shm_int = static_cast<int *>(shm_ptr_p);
            if (fork() == 0)
            {
                size_t NUM_NODES = (shm_int[1]);
                size_t NUM_EDGES = ((shm_int[0] - 3) / 2);
                for (int i = 3; i < shm_int[0]; i += 2)
                {
                    g[shm_int[i]].push_back(shm_int[i + 1]);
                    g[shm_int[i + 1]].push_back(shm_int[i]);
                }
                shm_int[2] = shm_int[0] + 1;
                int n = NUM_NODES, m = NUM_EDGES;
                // vector<vector<int>> dist(1);
                // dist[0] = djikstra(0, n);
                // print_dist_vec(dist);
                for (int i = 1; i <= 10; i++)
                {
                    if (fork() == 0)
                    {
                        string s = "./output/graph" + to_string(i) + ".txt";
                        for (int j = 0; j < n / 10; i++)
                        {
                            // vector<vector<int>> dist(1);
                            // dist[0] = djikstra(j + (i - 1) * (n / 10), n);
                            // print_dist_vec(dist, j + (i - 1) * (n / 10), s);
                            int N = 1;
                            vector<vector<int>> dist(N);
                            vector<vector<vector<int>>> path(N, vector<vector<int>>(n + 1));
                            dist[0] = djikstra_with_path(j + (i - 1) * (n / 10), n, path[0]);
                            print_dist_vec(dist[0], path[0], n, s);
                        }
                    }
                    while (wait(NULL) > 0)
                    {
                        ;
                    }
                }
                shmdt(shm_ptr_p);
                exit(0);
            }
            while (wait(NULL) > 0)
            {
                ;
            }
            shmdt(shm_ptr_p);
        }
    }
    shmctl(shm_id, IPC_RMID, NULL);
    return 0;
}
