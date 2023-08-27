#include <algorithm>
#include <array>
#include <chrono>
#include <fstream>
#include <iostream>
#include <list>
#include <map>
#include <set>
#include <sstream>
#include <string>
#include <vector>

// #define __DEBUG_MODE__  true
#define __DEBUG_MODE__  false

using namespace std;

struct FPGAGraph
{
    enum type { nonViolate = 0, violate = 1 };
    vector<list<int>> edges;
    vector<array<vector<int>, 2>> SFPGA;

    void findDist(const int & numFPGAs)
    {
        SFPGA.resize(numFPGAs);
        for(int i = 0; i < numFPGAs; i++)
        {
            int now = 0;
            SFPGA[i][nonViolate].reserve(edges[i].size() + 1);
            SFPGA[i][violate].reserve(numFPGAs);
            for(const auto & j : edges[i])
            {
                while(now != j)
                {
                    if(now == i)
                        SFPGA[i][nonViolate].emplace_back(now++);
                    else
                        SFPGA[i][violate].emplace_back(now++);
                }
                SFPGA[i][nonViolate].emplace_back(now++);
            }
            while(now < numFPGAs)
            {
                if(now == i)
                    SFPGA[i][nonViolate].emplace_back(now++);
                else
                    SFPGA[i][violate].emplace_back(now++);
            }
            SFPGA[i][nonViolate].shrink_to_fit();
            SFPGA[i][violate].shrink_to_fit();
        }
        
        if(!__DEBUG_MODE__)
            return;

        for(int i = 0; i < SFPGA.size(); i++)
        {
            cout << i << ":\tNon-Violate:";
            for(const auto & j : SFPGA[i][nonViolate])
                cout << " " << j;
            cout << "\n\tViolate:";
            for(const auto & j : SFPGA[i][violate])
                cout << " " << j;
            cout << endl;
        }
    }

    void deleteCddt(const int & id, vector<vector<int>> & cddt)
    {
        for(auto & i : SFPGA)
        {
            auto iter = find(i[nonViolate].begin(), i[nonViolate].end(), id);
            if(iter != i[nonViolate].end())
                i[nonViolate].erase(iter);
            else
            {
                iter = find(i[violate].begin(), i[violate].end(), id);
                i[violate].erase(iter);
            }
        }

        for(auto & i : cddt)
        {
            if(i.empty())
                continue;
            auto iter = find(i.begin(), i.end(), id);
            if(iter != i.end())
                i.erase(iter);
        }
    }
};

struct circuitGraph
{
    enum type { fromMe = 0, toMe = 1 };
    vector<array<list<int>, 2>> hyperedges;
};

void readFile(const char * file,
              int & numFPGAs, int & numFPGAChannels, int & capacity, int & numNodes, int & numNets, int & numFixedNodes,
              FPGAGraph & FPGAGraph, circuitGraph & circuitGraph, vector<array<int, 2>> & fixedPairs)
{
    fstream input;
    input.open(file, ios::in);
    input >> numFPGAs >> numFPGAChannels >> capacity >> numNodes >> numNets >> numFixedNodes;

    FPGAGraph.edges.resize(numFPGAs);
    for(int i = 0; i < numFPGAChannels; i++)
    {
        int a, b;
        input >> a >> b;
        FPGAGraph.edges[a].emplace_back(b);
        FPGAGraph.edges[b].emplace_back(a);
    }

    circuitGraph.hyperedges.resize(numNodes);
    for(int i = -1; i < numNets; i++)
    {
        string tmp;
        getline(input, tmp);
        if(i == -1)
            continue;
        istringstream ss(tmp);
        int source, sink;
        ss >> source;
        while(ss >> sink)
        {
            circuitGraph.hyperedges[source][circuitGraph.fromMe].emplace_back(sink);
            circuitGraph.hyperedges[sink][circuitGraph.toMe].emplace_back(source);
        }
    }

    fixedPairs.resize(numFixedNodes);
    for(int i = 0; i < numFixedNodes; i++)
        input >> fixedPairs[i][0] >> fixedPairs[i][1];
    
    input.close();

    if(!__DEBUG_MODE__)
        return;

    for(int i = 0; i < FPGAGraph.edges.size(); i++)
    {
        cout << i << ": ";
        for(const auto & j : FPGAGraph.edges[i])
            cout << j << ' ';
        cout << endl;
    }

    for(int i = 0; i < circuitGraph.hyperedges.size(); i++)
    {
        cout << i << ": \tFrom Me: ";
        for(const auto & j : circuitGraph.hyperedges[i][circuitGraph.fromMe])
            cout << j << ' ';
        cout << "\n\tTo Me: ";
        for(const auto & j : circuitGraph.hyperedges[i][circuitGraph.toMe])
            cout << j << ' ';
        cout << endl;
    }
}

void propagation(const int & numFPGAs, const int & numNodes, FPGAGraph & FPGAGraph, circuitGraph & circuitGraph,
                 vector<array<int, 2>> & fixedPairs, vector<vector<int>> & cddt)
{
    FPGAGraph.findDist(numFPGAs);

    cddt.resize(numNodes);
    for(const auto & i : fixedPairs)
        cddt[i[0]].emplace_back(i[1]);

    if(!__DEBUG_MODE__)
        return;

    for(int i = 0; i < cddt.size(); i++)
    {
        if(!cddt[i].empty())
        {
            cout << i << ": ";
            for(const auto & j : cddt[i])
                cout << j << ' ';
            cout << "| " << cddt[i].size() << endl;
        }
    }
}

void partition(const int & numFPGAs, const int & numNodes, const int & capacity, vector<vector<int>> & cddt,
               FPGAGraph & FPGAGraph, const circuitGraph & circuitGraph,
               vector<vector<int>> & FPGAPart, vector<int> & circuitPart)
{
    vector<int> Q, fullFPGA, noPlace;
    auto comp = [&cddt](int a, int b){return (cddt[a].size() > cddt[b].size());};
    Q.reserve(numNodes);
    fullFPGA.reserve(numFPGAs);
    noPlace.reserve(numNodes);
    for(int i = 0; i < cddt.size(); i++)
        if(!cddt[i].empty())
            Q.emplace_back(i);
    make_heap(Q.begin(), Q.end(), comp);

    while(!Q.empty())
    {
        const int nowNode = Q.front();
        pop_heap(Q.begin(), Q.end(), comp);
        Q.pop_back();
        
        int destFPGA;
        vector<array<int, 2>> R;
        for(const auto & i : cddt[nowNode])
        {
            if(FPGAPart[i].empty())
                FPGAPart[i].reserve(capacity);

            if(cddt[nowNode].size() == 1)
            {
                destFPGA = i;
                if(FPGAPart[destFPGA].size() == capacity)
                {
                    noPlace.emplace_back(nowNode);
                    circuitPart[nowNode] = -2;

                    goto VIOLATE;
                }
                else
                {
                    for(const auto & type : circuitGraph.hyperedges[nowNode])
                    {
                        for(const auto & nbrNode : type)
                        {
                            if(circuitPart[nbrNode] == -1)
                            {
                                if(cddt[nbrNode].empty())
                                {
                                    cddt[nbrNode] = FPGAGraph.SFPGA[destFPGA][FPGAGraph.nonViolate];
                                    Q.emplace_back(nbrNode);
                                }
                                else
                                {
                                    vector<int> tmp (max(cddt[nbrNode].size(), FPGAGraph.SFPGA[destFPGA][FPGAGraph.nonViolate].size()));
                                    auto iter = set_intersection(cddt[nbrNode].begin(), cddt[nbrNode].end(),
                                                                 FPGAGraph.SFPGA[destFPGA][FPGAGraph.nonViolate].begin(), FPGAGraph.SFPGA[destFPGA][FPGAGraph.nonViolate].end(),
                                                                 tmp.begin());
                                    tmp.resize(iter - tmp.begin());
                                    cddt[nbrNode] = tmp;
                                }
                            }
                        }
                    }
                    goto PART;
                }
            }

            array<int, 2> tmp = {i, 0};
            set<int> tmp2;
            for(const auto & j : circuitGraph.hyperedges[nowNode][circuitGraph.fromMe])
            {
                if(circuitPart[j] != -1)
                    tmp2.emplace(circuitPart[j]);
            }
            if(!tmp2.empty())
            {
                if(tmp2.emplace(i).second)
                {
                    tmp[1]++;
                    if(tmp2.size() == 2)
                        tmp[1]++;
                }
                tmp2.clear();
            }

            for(const auto & j : circuitGraph.hyperedges[nowNode][circuitGraph.toMe])
            {
                if(circuitPart[j] != -1)
                    tmp2.emplace(circuitPart[j]);
                for(const auto & k : circuitGraph.hyperedges[j][circuitGraph.fromMe])
                {
                    if(circuitPart[k] != -1)
                        tmp2.emplace(circuitPart[k]);
                }
                if(!tmp2.empty())
                {
                    if(tmp2.emplace(i).second)
                    {
                        tmp[1]++;
                        if(tmp2.size() == 2)
                            tmp[1]++;
                    }
                    tmp2.clear();
                }
            }
            R.emplace_back(tmp);
        }
        sort(R.begin(), R.end(), [&FPGAPart](array<int, 2> a, array<int, 2> b){return (a[1] != b[1]) ? 
                                                                                      (a[1] < b[1]) : (FPGAPart[a[0]].size() < FPGAPart[b[0]].size());});
        if(R.empty())
        {
            noPlace.emplace_back(nowNode);
            circuitPart[nowNode] = -2;

            goto VIOLATE;
        }

        for(const auto & i : R)
        {
            const list<int> edges = FPGAGraph.edges[i[0]];
            const array<list<int>, 2> hyperedges = circuitGraph.hyperedges[nowNode];
            vector<bool> T (numFPGAs, false);
            vector<int> nbrs;
            vector<vector<int>> newCddt;
            set<int> newQ;
            nbrs.reserve(hyperedges[circuitGraph.fromMe].size() + hyperedges[circuitGraph.toMe].size());
            newCddt.reserve(hyperedges[circuitGraph.fromMe].size() + hyperedges[circuitGraph.toMe].size());
            T[i[0]] = true;
            for(const auto & j : edges)
                T[j] = true;
            for(const auto & type : hyperedges)
            {
                for(const auto & j : type)
                {
                    if(circuitPart[j] != -1)
                        continue;
                    vector<bool> tmp (numFPGAs, cddt[j].empty());
                    if(cddt[j].empty())
                    {
                        newQ.emplace(j);
                        for(const auto & k : fullFPGA)
                            tmp[k] = false;
                    }
                    else
                    {
                        for(const auto & k : cddt[j])
                            tmp[k] = true;
                    }
                    vector<int> tmp2;
                    tmp2.reserve(numFPGAs);
                    for(int k = 0; k < numFPGAs; k++)
                    {
                        if(T[k] && tmp[k])
                            tmp2.emplace_back(k);
                    }
                    tmp2.shrink_to_fit();

                    if(!tmp2.empty())
                    {
                        nbrs.emplace_back(j);
                        newCddt.emplace_back(tmp2);
                    }
                    else
                        goto NEXT;
                }
            }

            destFPGA = i[0];
            for(int j = 0; j < nbrs.size(); j++)
                cddt[nbrs[j]] = newCddt[j];
            if(!newQ.empty())
                Q.insert(Q.end(), newQ.begin(), newQ.end());
            goto PART;

            NEXT:
            continue;
        }
        goto VIOLATE;
        
        PART:
        FPGAPart[destFPGA].emplace_back(nowNode);
        circuitPart[nowNode] = destFPGA;
        if(FPGAPart[destFPGA].size() == capacity)
        {
            fullFPGA.emplace_back(destFPGA);
            FPGAGraph.deleteCddt(destFPGA, cddt);
        }

        VIOLATE:
        cddt[nowNode].clear();
        if(Q.empty())
        {
            vector<int> nonPart, newCddt;
            nonPart.reserve(numNodes);
            newCddt.reserve(numFPGAs);
            for(int i = 0; i < numNodes; i++)
            {
                if(circuitPart[i] == -1)
                    nonPart.emplace_back(i);
            }
            if(!nonPart.empty())
            {
                for(int i = 0; i < numFPGAs; i++)
                {
                    if(FPGAPart[i].size() < capacity)
                        newCddt.emplace_back(i);
                }
                newCddt.shrink_to_fit();
                for(const auto & i : nonPart)
                {
                    cddt[i] = newCddt;
                    Q.emplace_back(i);
                }
            }
        }
        make_heap(Q.begin(), Q.end(), comp);
    }

    if(!noPlace.empty())
    {
        vector<int> order;
        vector<vector<array<int, 2>>> cddtNumPairs (noPlace.size());
        order.reserve(noPlace.size());
        sort(fullFPGA.begin(), fullFPGA.end());
        for(int i = 0; i < noPlace.size(); i++)
        {
            order.emplace_back(i);
            const int & nowNode = noPlace[i];
            const list<int> & source = circuitGraph.hyperedges[nowNode][circuitGraph.fromMe];
            const list<int> & sink = circuitGraph.hyperedges[nowNode][circuitGraph.toMe];
            map<int, int> tmp;
            for(const auto & j : source)
            {
                if(circuitPart[j] != -2 && FPGAPart[circuitPart[j]].size() != capacity)
                    tmp.emplace(circuitPart[j], 1);
            }
            for(const auto & j : sink)
            {
                if(circuitPart[j] != -2 && FPGAPart[circuitPart[j]].size() != capacity && !tmp.emplace(circuitPart[j], 1).second)
                    tmp[circuitPart[j]]++;
            }
            
            vector<array<int, 2>> & nowPairs = cddtNumPairs[i];
            nowPairs.reserve(tmp.size());
            for(const auto & j : tmp)
            {
                array<int, 2> a = {j.first, j.second};
                nowPairs.push_back(a);
            }
            if(nowPairs.empty())
            {
                for(int j = 0, k = 0; j < numFPGAs; j++)
                {
                    if(k < fullFPGA.size() && j == fullFPGA[k])
                        k++;
                    else
                    {
                        array<int, 2> a = {j, 1};
                        nowPairs.emplace_back(a);
                    }
                }
            }
            sort(nowPairs.begin(), nowPairs.end(), [&FPGAPart](array<int, 2> a, array<int, 2> b){return (a[1] != b[1]) ?
                                                                                                        (a[1] < b[1]) : (FPGAPart[a[0]].size() > FPGAPart[b[0]].size());});
        }
        auto comp = [&cddtNumPairs](int a, int b){return (cddtNumPairs[a].size() != cddtNumPairs[b].size()) ?
                                                         (cddtNumPairs[a].size() < cddtNumPairs[b].size()) : (cddtNumPairs[a].back()[1] > cddtNumPairs[b].back()[1]);};
        sort(order.begin(), order.end(), comp);
        reverse(order.begin(), order.end());

        while(!order.empty())
        {
            const int nowNode = noPlace[order.back()];
            const int nowFPGA = cddtNumPairs[order.back()].back()[0];
            order.pop_back();
            FPGAPart[nowFPGA].emplace_back(nowNode);
            circuitPart[nowNode] = nowFPGA;

            for(const auto & i : order)
            {
                if(FPGAPart[nowFPGA].size() == capacity)
                {
                    auto iter = cddtNumPairs[i].begin();
                    while(iter != cddtNumPairs[i].end())
                    {
                        if((*iter)[0] != nowFPGA)
                            iter++;
                        else
                        {
                            cddtNumPairs[i].erase(iter);
                            break;
                        }
                    }
                }
                sort(cddtNumPairs[i].begin(), cddtNumPairs[i].end(), [&FPGAPart](array<int, 2> a, array<int, 2> b){return (a[1] != b[1]) ?
                                                                                                                          (a[1] < b[1]) : (FPGAPart[a[0]].size() > FPGAPart[b[0]].size());});
            }
            sort(order.begin(), order.end(), comp);
            reverse(order.begin(), order.end());
        }
    }
}

void writeFile(const char * file, const vector<int> & circuitPart)
{
    fstream output;
    output.open(file, ios::out);
    for(int i = 0; i < circuitPart.size(); i++)
        output << i << ' ' << circuitPart[i] << endl;
    output.close();
}

int main(int argc, char * argv[])
{
    auto inputStart = chrono::steady_clock::now();

    cout << "Reading File..." << endl; cout.flush();
    int numFPGAs, numFPGAChannels, capacity, numNodes, numNets, numFixedNodes;
    FPGAGraph FPGAGraph;
    circuitGraph circuitGraph;
    vector<array<int, 2>> fixedPairs;
    readFile(argv[1],
             numFPGAs, numFPGAChannels, capacity, numNodes, numNets, numFixedNodes,
             FPGAGraph, circuitGraph, fixedPairs);

    auto inputEnd = chrono::steady_clock::now();
    auto propagationStart = chrono::steady_clock::now();

    cout << "Propagation..." << endl; cout.flush();
    vector<vector<int>> cddt;
    propagation(numFPGAs, numNodes, FPGAGraph, circuitGraph, fixedPairs, cddt);

    auto propagationEnd = chrono::steady_clock::now();
    auto partitionStart = chrono::steady_clock::now();

    cout << "Partition..." << endl; cout.flush();
    vector<vector<int>> FPGAPart (numFPGAs);
    vector<int> circuitPart (numNodes, -1);
    partition(numFPGAs, numNodes, capacity, cddt, FPGAGraph, circuitGraph, FPGAPart, circuitPart);

    auto partitionEnd = chrono::steady_clock::now();
    auto outputStart = chrono::steady_clock::now();

    cout << "Writing File..." << endl; cout.flush();
    writeFile(argv[2], circuitPart);

    auto outputEnd = chrono::steady_clock::now();
    if(__DEBUG_MODE__)
        return 0;
    cout << "----- Timing Result -----\n"
         << "  Input Time:\t\t" << chrono::duration<float>(inputEnd - inputStart).count() << "\tsec." << endl
         << "+ Propagation Time:\t" << chrono::duration<float>(propagationEnd - propagationStart).count() << "\tsec." << endl
         << "+ Partition Time:\t" << chrono::duration<float>(partitionEnd - partitionStart).count() << "\tsec." << endl
         << "+ Output Time:\t\t" << chrono::duration<float>(outputEnd - outputStart).count() << "\tsec." << endl
         << "= Total Runtime:\t" << chrono::duration<float>(outputEnd - inputStart).count() << "\tsec." << endl;
}