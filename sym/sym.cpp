#include <fstream>
#include <string>
#include <iostream>
#include <vector>
#include <unordered_map>
#include <unordered_set>
using namespace std;
vector<string> removeDupWord(string str)
{
    vector<string> words;
    string word = "";
    for (auto x : str) 
    {
        if (x == ' ' || x == ',')
        {
            if (word.size() > 0)
               words.push_back(word);
            word = "";
        }
        else {
            word = word + x;
        }
    }
    words.push_back(word);
    return words;
}
bool getRegValue(unordered_map<string, bool>& regToValueMap, string reg) {
    bool regValue = false;
    if (regToValueMap.find(reg) != regToValueMap.end())
       regValue = regToValueMap[reg];
    return regValue;
}
int main(int argc, char *argv[]) 
{ 
    if (argc < 3) return 0;
    
    unordered_map<string, bool> regToValueMap;
    unordered_map<string, bool> varToValueMap;
    std::ifstream outputFile(argv[1]);
    std::ifstream loadFile(argv[2]);
    std::string str; 
    bool rd = false;
    while (std::getline(loadFile, str))
    {
       vector<string> words = removeDupWord(str);
       if (varToValueMap.find(words[0]) == varToValueMap.end()) {
          bool varValue;
          cout<<words[0]<<" = ";
          cin>>varValue;
          varToValueMap[words[0]] = varValue;
       }
       regToValueMap[words[1]] = varToValueMap[words[0]];



    }
    while (std::getline(outputFile, str)) // reading and executiong the output file
    {
        vector<string> words = removeDupWord(str);  
        //rd rs0 nand rs1
        bool rs0 = getRegValue(regToValueMap, words[1]); // get the value of rs0
        bool rs1 = getRegValue(regToValueMap, words[3]); // get the  value of rs1
        rd = !(rs0 && rs1);
        cout<<"rd="<<rd<<" "<<words[1]<<"="<<rs0<<" "<<words[3]<<"="<<rs1<<endl;
        regToValueMap[words[0]] = rd;
        
        
    }
    cout<<"The Reulst is :"<<rd<<endl;
    return 0;
}
