#include <bits/stdc++.h>
using namespace std;

int main(){
    ifstream in("daa.txt");
    ofstream f("data2.txt");
    vector<string> results;
    while(!in.eof()){
        string text;
        getline(in,text);
        results.push_back(text);
    }
    for(auto val:results){
        if(val.size()>0&&val[0]=='6'){
        string s;
        int current = 0;
        while(val.substr(current,4)!="time")current++;
        current+=5;
        while(val[current]!=' '){
            s.push_back(val[current]);
            current++;
        }
        f<<s<<endl;
        }
    }
    return 0;
}