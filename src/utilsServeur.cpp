#include <iostream>
#include <string>
#include <vector>
#include <sstream>
#include <queue>
//#include "../include/Serveur.hpp"


std::string getNthWord(const std::string& input, int place) {
    std::istringstream iss(input);

    std::vector<std::string> words;

    std::string word;
    while (iss >> word) {
        words.push_back(word);
    }
    return words[place];
}

std::queue<std::string> splitToQueue(const std::string& input, char delimiter) {
    std::queue<std::string> tokens;
    std::istringstream tokenStream(input);
    std::string token;
    
    while (std::getline(tokenStream, token, delimiter)) {
        if (!tokenStream.eof()) {
            token.push_back(delimiter);
        }
        tokens.push(token);
    }
    return tokens;
}

void mergeQueues(std::queue<std::string>& q1, std::queue<std::string>& q2) {
    while (!q2.empty()) {
        q1.push(q2.front());
        if (!q2.empty())
            q2.pop();
    }
}