#ifndef DATABASEMAN_HPP
#define DATABASEMAN_HPP

#include "../include/mysql/mysql.h"
#include <stdio.h>
#include <stdlib.h>
#include "guiHandler/toastHandler.hpp"
#include "database.hpp"
#include "datapointStruct.hpp"
#include <iostream>
#include <format>


class DatabaseMan {
    private:
        MYSQL mysql; 
        MYSQL_ROW row;
        Database database;
        std::vector<MATCH_DATAPOINT> datapoints;
        std::vector<TEAM_DATAPOINT> teams;
        std::vector<MATCHLIST_DATAPOINT> matchs;

        MATCH_DATAPOINT temp;
        TEAM_DATAPOINT temp2;
        MATCHLIST_DATAPOINT temp3;

        std::string resultstr;
        
        
    public:

    void setmatchdata(std::vector<MATCH_DATAPOINT> datapointsn) {
        this->datapoints = datapointsn;
    }

    void setteamdat(std::vector<TEAM_DATAPOINT> teamsn) {
        this->teams = teamsn;
    }
    void setmatchlistdat(std::vector<MATCHLIST_DATAPOINT> matchsn) {
        this->matchs = matchsn;
    }


    void addMatchDatapoints() {
        if (!datapoints.empty()) {
            for (MATCH_DATAPOINT i : this->datapoints) {
                auto _notUsed = database.query("insert into matchtransaction (UploadID, CompID, MatchID, DatapointID, ScouterID, TeamID, AllianceID, DatapointValue, DatapointTimestamp) values(?,'?','?',?,?,?,?,'?',?)", "0", i.CompID.c_str(), i.MatchID.c_str(), i.DatapointID.c_str(), i.ScouterID.c_str(), i.TeamID.c_str(), i.AllianceID.c_str(), i.DatapointValue.c_str(), i.DatapointTimestamp.c_str());
            }
        }
        else {
            DebugConsole::println(std::string("Error inserting datapoints. Vector is null "), DBGC_RED);
        }
    }

    void clearTeams() {
        auto _deleteRes = database.execQuery("delete from team", 1);
    }
    
    void addTeams() {
        if (!teams.empty()) {
            for (auto i = teams.begin(); i != teams.end(); ++i) {
                if (i.base() != NULL) {
                    temp2 = *i.base();                    
                    std::string teamNum = std::to_string(temp2.teamNum);
                    auto insertRes = database.query("insert into team(TeamId, TeamNumber, TeamDesc) values(?, ?, '?')", teamNum.c_str(), teamNum.c_str(), temp2.teamName.c_str());
                }
                else {
                    DebugConsole::print(std::string("Error inserting teams. Base is null ") + "\n", DBGC_RED);
                }
            }
        }
        else {
            DebugConsole::print(std::string("Error inserting teams. Vector is null ") + "\n", DBGC_RED);
        }
    }
};
#endif