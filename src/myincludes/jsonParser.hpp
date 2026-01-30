#ifndef JSON_PARSER_HPP
#define JSON_PARSER_HPP

#include "../include/json.hpp"
#include "guiHandler/toastHandler.hpp"
#include "datapointStruct.hpp"
#include <fstream>
#include <iostream>
#include <vector>

using json = nlohmann::json;

class JsonParser {
    private:
        json data;
    public:
        JsonParser(std::string datan) {
       
            try {
                this->data = json::parse(datan);
                toastHandler::add(Toast("JSON parsed", LENGTH_NORMAL));
            }
            catch(...) {
               toastHandler::add(Toast("Failed to parse JSON", LENGTH_NORMAL));
            }
            std::ofstream file;
            try {
                file = std::ofstream("JSONPARSER log.json");
                file << datan;
                file.close();
            }
            catch(...) {
                file.close();
                toastHandler::add(Toast("Shit hit the fan. Nothing submitted.", LENGTH_NORMAL));
            }
           
        }
        std::vector<MATCH_DATAPOINT> parseMatch() {                
            std::vector<MATCH_DATAPOINT> datapoints = std::vector<MATCH_DATAPOINT>();  
            try {
                for (auto it = this->data["scoutingData"].begin(); it != this->data["scoutingData"].end() ; ++it) { 
                        MATCH_DATAPOINT currentDatapoint;
                    
                        std::cout << *it << std::endl;
                        json element = *it;
                        currentDatapoint.ScouterID = element["ScouterID"];
                        currentDatapoint.DatapointID = element["DatapointID"];
                        currentDatapoint.TeamID = element["TeamID"];
                        currentDatapoint.MatchID = element["MatchID"];
                        currentDatapoint.DatapointValue = element["DatapointValue"];
                        currentDatapoint.DatapointTimestamp = element["DatapointTimestamp"];
                        currentDatapoint.AllianceID = element["allianceID"];    
                    
                    datapoints.push_back(currentDatapoint);
                }
            }
            catch (...) {
                DebugConsole::println(std::string("Error parsing match "), DBGC_YELLOW);
            }
           
            return datapoints;
        }
        std::vector<TEAM_DATAPOINT> parseTeams() {                
            std::vector<TEAM_DATAPOINT> Teamsdata = std::vector<TEAM_DATAPOINT>();  
            try {
                for (auto it = this->data.begin(); it != this->data.end() ; ++it) { 
                
                    TEAM_DATAPOINT currentDatapoint; 
                    
                    json element = *it;
                    (!element["nickname"].is_null()) ?  currentDatapoint.teamName = element["nickname"] : currentDatapoint.teamName = "NULL";
                    (!element["team_number"].is_null()) ?  currentDatapoint.teamNum = element["team_number"] : currentDatapoint.teamNum = 0;
                    (!element["website"].is_null()) ?  currentDatapoint.teamDesc = element["website"] : currentDatapoint.teamDesc = "NULL";  

                    Teamsdata.push_back(currentDatapoint);  
                }
            }
            catch (...) {
                DebugConsole::print(std::string("Error parsing teams ") +  "\n", DBGC_YELLOW);
            }             
            return Teamsdata;
        }
        
        std::vector<MATCHLIST_DATAPOINT> parseMatchList() {
            std::vector<MATCHLIST_DATAPOINT> MatchlistData = std::vector<MATCHLIST_DATAPOINT>();  
            try {
                for (auto it = this->data.begin(); it != this->data.end() ; ++it) { 
                
                    MATCHLIST_DATAPOINT currentDatapoint; 
                    
                    json element = *it;
                    (!element["key"].is_null()) ?  currentDatapoint.tbaMatchID = element["key"] : currentDatapoint.tbaMatchID = "NULL";
                    (!element["comp_level"].is_null()) ?  currentDatapoint.compLevel = element["comp_level"] : currentDatapoint.compLevel = "NULL";
                    (!element["match_number"].is_null()) ? currentDatapoint.matchNumber = element["match_number"] : currentDatapoint.matchNumber = -1;
                    
                    MatchlistData.push_back(currentDatapoint);  
                }
            }
            catch (...) {
                DebugConsole::print(std::string("Error parsing teams ") +  "\n", DBGC_YELLOW);
                return std::vector<MATCHLIST_DATAPOINT>(); // make sure returnee knows error occured
            }
            return MatchlistData;
        }
};

#endif