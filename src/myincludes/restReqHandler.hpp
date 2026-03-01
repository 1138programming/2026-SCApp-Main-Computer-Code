#ifndef RESTREQHANDLER_HPP
#define RESTREQHANDLER_HPP

#include "../include/curlIncludes/curl/curl.h"

#include <vector>
#include <format>
#include <iostream>
#include <string>
#include <fstream>
#include <functional>

// NOT thread-safe
class RestReqHandler {
    private:
        DatabaseMan* database = new DatabaseMan();
        std::string output; // TODO: causes errors when private?
        
        static size_t readHandler(char* ptr, size_t size, size_t numElements, void* ourPtr) {
            RestReqHandler* self = (RestReqHandler*)ourPtr;
            std::string temp = std::string(ptr, size * numElements);
            self->output.append(temp);
            
            return size * numElements;
        }
        
    public:
        /**
         * @brief appends the ```request``` string to after the /v3/ in the link, using the api key given in /src/resources/tbaKey.env
         */
        std::string makeTBAReq(std::string request) {
            CURL* handler = curl_easy_init();

            curl_slist* headerList = NULL;
            this->output.clear();
            if (handler) {
                std::string url = std::string("https://www.thebluealliance.com/api/v3/") + request;
                curl_easy_setopt(handler, CURLOPT_URL, url.c_str());
                curl_easy_setopt(handler, CURLOPT_SSL_VERIFYPEER, false);

                // read header file and set header
                std::ifstream headerFile("resources/tbaKey.env");
                if (!headerFile.good()) {
                    DebugConsole::println("ERROR: Unable to make TBA Request. Please add a TBA api key into /resources/tbaKey.env to use TBA.", DBGC_RED);
                    return std::string();
                }
                std::string tbaKey;
                    std::getline(headerFile, tbaKey);
                std::string header = std::string("X-TBA-Auth-Key: ") + tbaKey;
                headerList = curl_slist_append(headerList, header.c_str());
                curl_easy_setopt(handler, CURLOPT_HTTPHEADER, headerList);
                curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, readHandler);
                curl_easy_setopt(handler, CURLOPT_WRITEDATA, this);

                //perform query
                CURLcode res = curl_easy_perform(handler); 
                if (res != CURLcode::CURLE_OK) {
                    DebugConsole::print(std::string("Error Pulling from api Res: ") + std::to_string((int)res) + "\n", DBGC_YELLOW);
                }
            }
            curl_easy_cleanup(handler);

            return output;
        }
        void getteamdata(int page) {
            std::string pageData = makeTBAReq(std::string("teams/") + std::to_string(page));

            JsonParser parser(pageData); 
            std::vector<TEAM_DATAPOINT> teams = parser.parseTeams();

            database->setteamdat(teams);
            database->addTeams();        
        }
        void getteamsatcomphdata(std::string eventkey) {
            JsonParser teamsParser(makeTBAReq(std::string("event/") + eventkey + std::string("/teams")));

            std::vector<TEAM_DATAPOINT> teamsList = teamsParser.parseTeams();
            if (teamsList.size() >= 1) {
                std::ofstream compTeamsFile("resources/csv/teamCompList.csv");
                compTeamsFile << std::to_string(teamsList[0].teamNum);
                for (int i = 1; i < teamsList.size(); i++) {
                    compTeamsFile << "," << std::to_string(teamsList[i].teamNum);
                }
            }
            else {
                toastHandler::add(Toast("Invalid Comp ID", LENGTH_NORMAL));
            }       
        }

        void getMatchData(std::string eventKey) {
            int qualMatchCount = 0;

            JsonParser matchListParser(makeTBAReq(std::string("event/") + eventKey + std::string("/matches/simple")));

            std::vector<MATCHLIST_DATAPOINT> matchDatapoints = matchListParser.parseMatchList();
            for (MATCHLIST_DATAPOINT datapoint : matchDatapoints) {
                if (datapoint.compLevel == "qm") {
                    qualMatchCount++;
                }
            }

            if (qualMatchCount < 40) {
                DebugConsole::println("QUAL MATCHES NOT CORRECTLY COUNTED!!! DEFUALTING TO 150...", DBGC_YELLOW, DBGL_WARNING);
                qualMatchCount = 150;
            }

            std::ofstream compMatchNumFile("resources/csv/compMatchNums.csv");
            compMatchNumFile << qualMatchCount << ",13,3"; // always 13 playoffs & 3 finals...
            compMatchNumFile.close();
        }


        /*__SEND REQUESTS__*/
        std::string uploadToBackend(std::string url, std::string data) {
            CURL* handler = curl_easy_init();
            curl_slist* headerList = NULL;
            this->output.clear();
            if (handler) {
                curl_easy_setopt(handler, CURLOPT_URL, url.c_str());
                curl_easy_setopt(handler, CURLOPT_SSL_VERIFYPEER, false);

                std::ifstream authKey("resources/backendKey.env");
                std::string authStr;
                if (!authKey.good()) {
                    DebugConsole::println("ERROR: Unable to make Backend Request. Please add backend api key into /resources/backendKey.env to upload to backend.", DBGC_RED);
                    return std::string("");
                }
                std::getline(authKey, authStr);

                std::string fullHeader = std::string("Auth: ") + authStr;
                headerList = curl_slist_append(headerList, fullHeader.c_str());

                curl_easy_setopt(handler, CURLOPT_HTTPHEADER, headerList);
                curl_easy_setopt(handler, CURLOPT_WRITEFUNCTION, readHandler);
                curl_easy_setopt(handler, CURLOPT_WRITEDATA, this);
                curl_easy_setopt(handler, CURLOPT_POSTFIELDS, data.c_str());

                CURLcode result = curl_easy_perform(handler);
                if(result != CURLE_OK) {
                    DebugConsole::println(std::string("POST Error: ") + std::string(curl_easy_strerror(result)));
                    return std::string("");
                }
            
                curl_easy_cleanup(handler);
            }

            return this->output;
        }

        std::string getFromBackend(std::string url) {
            return uploadToBackend(url, std::string(""));
        }

        void deleteteams() {
            database->clearTeams();
        }

        ~RestReqHandler() {
            delete database;
        }
};

#endif