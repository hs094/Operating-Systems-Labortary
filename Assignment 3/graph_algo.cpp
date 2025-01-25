#include <bits/stdc++.h>
using namespace std;
#define ll long long

vector<vector<int> > g;

void print_dist_vec(vector<int> &dist, vector<vector<int> > &path, int n, ){
    for(int i=0; i<=n; i++){
        cout<<i<<":"<<dist[i]<<": ";
        for(int j=0; j<path[i].size(); j++)
            cout<<path[i][j]<<" ";
        cout<<endl;
    }
}

// djikstra algorithm to find shortest path from source to all other nodes in a graph with non-negative edge weights using adjacency list representation of graph
vector<int> djikstra(int src, int n){
    vector<int> dist(n+1, INT_MAX);
    dist[src] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;
    pq.push({0, src});

    while(!pq.empty()){
        int u = pq.top().second;
        pq.pop();

        for(int v: g[u]){
            if(dist[v] > dist[u] + 1){
                dist[v] = dist[u] + 1;
                pq.push({dist[v], v});
            }
        }
    }
    for(int i=0; i<=n; i++)
        if(dist[i] == INT_MAX)
            dist[i] = -1;
    return dist;
}

// djikstra algorithm to find shortest path from source to all other nodes in a graph with non-negative edge weights using adjacency list representation of graph while saving the path in parameter vector of vectors
vector<int> djikstra_with_path(int src, int n, vector<vector<int> > &path){
    vector<int> dist(n+1, INT_MAX);
    path.clear();
    dist[src] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;
    pq.push({0, src});

    while(!pq.empty()){
        int u = pq.top().second;
        pq.pop();

        for(int v: g[u]){
            if(dist[v] > dist[u] + 1){
                dist[v] = dist[u] + 1;
                pq.push({dist[v], v});

                path[v].clear();
                path[v] = path[u];
                path[v].push_back(u);
            }
        }
    }
    for(int i=0; i<=n; i++)
        if(dist[i] == INT_MAX)
            dist[i] = -1;
        else
            path[i].push_back(i);
    return dist;
}

// get path from source to destination using dfs in a graph with non-negative edge weights using adjacency list representation of graph
vector<int> get_path(int src, int dest, int n){
    vector<int> dist(n+1, INT_MAX);
    vector<vector<int> > path(n+1);
    dist[src] = 0;

    priority_queue<pair<int, int>, vector<pair<int, int> >, greater<pair<int, int> > > pq;
    pq.push({0, src});

    while(!pq.empty()){
        int u = pq.top().second;
        pq.pop();

        for(int v: g[u]){
            if(dist[v] > dist[u] + 1){
                dist[v] = dist[u] + 1;
                pq.push({dist[v], v});

                path[v].clear();
                path[v] = path[u];
                path[v].push_back(u);
            }
        }
    }
    for(int i=0; i<=n; i++)
        if(dist[i] == INT_MAX)
            dist[i] = -1;
        else
            path[i].push_back(i);
    return path[dest];
}


void update_distance(vector<int> &dist, vector<vector<int> > &path, int u,int v){
    int src = path[u][0];
    queue<pair<int,int> > q, temp;
    q.push({v,u});
    dist[v] = dist[u] + 1;
    int val=2;
    while(!q.empty()){
        int x = q.front().first;
        int h = q.front().second;

        path[x].clear();
        path[x] = path[h];
        path[x].push_back(h);
        path[x].push_back(x);
        path[x] = get_path(src, x, g.size()-1);

        q.pop();
        for(auto y: g[x]){
            if(dist[y] == -1){
                dist[y] = dist[u] + val;
                path[y].clear();    path[y] = path[x];
                path[y].push_back(y);
                q.push({y,x});
            }
            else if(dist[y] > dist[u] + val){
                dist[y] = dist[u] + val;
                path[y].clear();    path[y] = path[x];
                path[y].push_back(y);
                temp.push({y,x});
            }
        }
        if(q.empty()){
            q = temp;
            temp = queue<pair<int,int> >();
            val++;
        }
    }
}

void get_initial_graph(int &n, int &m){
    int la;
    fstream file;
    string word, t, q, filename;

    filename = "file.txt";
    file.open(filename.c_str());
    int lno=0;

    file >> word;
    m = stoi(word);

    file >> word;
    n = stoi(word);

    file >> word;
    la = stoi(word);

    g.clear();
    g.resize(n+1);

    int e=0, u, v;
    while(e<=m){
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

void add_more_edges(vector<pair<int, int> > &new_edges){
    fstream file;
    string word, t, q, filename;
    int u, v;

    new_edges.clear();
    filename = "new_nodes.txt";
    file.open(filename.c_str());
    while(file >> word){
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

void update_dist_vec(vector<vector<int> > &dist, vector< vector<vector<int> > > &path, vector<pair<int, int> > &new_edges){
    for(int i=0; i<dist.size(); i++){
        for(auto x: new_edges){
            if(dist[i][x.first] >=0 || dist[i][x.second] >= 0){
                int a,b;
                if(dist[i][x.first]<0){
                    a=x.second;
                    b=x.first;
                }
                else if(dist[i][x.second]<0){
                    a=x.first;
                    b=x.second;
                }
                else if(dist[i][x.first]>=0 && ((dist[i][x.first]+1) < dist[i][x.second])){
                    a=x.first;
                    b=x.second;
                }
                else if(dist[i][x.second]>=0 && ((dist[i][x.second]+1) < dist[i][x.first])){
                    a=x.second;
                    b=x.first;
                }
                else
                    continue;

                cout<<"i:"<<i<<" a:"<<a<<" b:"<<b<<endl;
                update_distance(dist[i], path[i], a, b);
            }
        }
    }
}

int main(){
    int n, m, la;
    get_initial_graph(n,m);

    int N = 1;
    vector<vector<int> > dist(N);
    vector< vector<vector<int> > > path(N, vector<vector<int> > (n+1));
    dist[0] = djikstra_with_path(0, n, path[0]);

    print_dist_vec(dist[0], path[0], n);

    vector<pair<int, int> > new_edges;
    add_more_edges(new_edges);

    cout<< "new_edges: "<<new_edges.size()<<endl;
    update_dist_vec(dist, path, new_edges);

    print_dist_vec(dist[0], path[0], n);    
    return 0;
}
