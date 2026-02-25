#ifndef DATAPOINT_STRUCT_HPP
#define DATAPOINT_STRUCT_HPP

#include <string>

typedef struct {
    std::string ScouterID;
    std::string MatchID;
    std::string TeamID;
    std::string DatapointID;
    std::string DatapointValue;
    std::string DatapointTimestamp;
    std::string AllianceID;
    std::string CompID;
} MATCH_DATAPOINT;

typedef struct {
    int teamNum;
    std::string teamName;
    std::string teamDesc;

} TEAM_DATAPOINT;

typedef struct {
    std::string tbaMatchID;
    std::string compLevel;
    int matchNumber;

} MATCHLIST_DATAPOINT;



#endif