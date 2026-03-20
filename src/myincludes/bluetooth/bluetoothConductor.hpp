#ifndef BLUETOOTHCONDUCTOR_HPP
#define BLUETOOTHCONDUCTOR_HPP

#include "btTabObj.hpp"
#include "../jsonParser.hpp"
#include "../DatabaseMan.hpp"
#include "../murmurHash2Neutral.hpp"
#include "../database.hpp"

#include <vector>
#include <optional>
#include <fstream>
#include <sstream>

class BluetoothConductor {
    private:
        /*********************************************/
        /* PRIVATE COMMUNICATION PROTO FUNC(S) */
        /*********************************************/
        std::optional<std::vector<char>> returnEmptyVector() {
            return std::nullopt;
        }
        void handleWriteTransactions(bt::TABTRANSACTION* trans, std::launch policy) {
            switch (trans->transactionType) {
                case bt::TRANS_SEND_LOCAL_DB: {
                    std::ostringstream dbFileStrBuilder;
                        dbFileStrBuilder << readWholeFile("resources/csv/scouterList.csv")
                        << "\n"
                        << readWholeFile("resources/csv/teamCompList.csv")
                        << "\n"
                        << readWholeFile("resources/csv/compMatchNums.csv")
                        << "\n"
                        << readWholeFile("resources/csv/compID.csv");
                    std::string dbFileStr = dbFileStrBuilder.str();

                    std::vector<char> dbFileArr;
                    for(char i : dbFileStr) {
                        dbFileArr.push_back(i);
                    }
                    trans->batmanTrans = false;
                    trans->writeTransaction = true;

                    trans->data = std::async(policy, trans->parent->internalWrite, trans->parent, dbFileArr, std::ref(trans->success));
                    break;
                }
                case bt::TRANS_SEND_LOCAL_DB_HASH: {
                    std::ostringstream dbFileStrBuilder;
                        dbFileStrBuilder << readWholeFile("resources/csv/scouterList.csv")
                        << "\n"
                        << readWholeFile("resources/csv/teamCompList.csv")
                        << "\n"
                        << readWholeFile("resources/csv/compMatchNums.csv")
                        << "\n"
                        << readWholeFile("resources/csv/compID.csv");
                    std::string dbFileStr = dbFileStrBuilder.str();

                    std::vector<char> dbFileVec;
                    for (char i : dbFileStr) {
                        dbFileVec.push_back(i);
                    }
                    
                    int murmurRes = murmurHash(dbFileVec);
                    DebugConsole::println(std::string("Sending Hash: ") + std::to_string(murmurRes), DBGL_DEVEL);

                    std::vector<char> murmurHashData;
                    for (int i = 0; i < BT_HASH_SIZE; i++) {
                        murmurHashData.push_back(((char*)&murmurRes)[i]);
                    }
                    trans->batmanTrans = false;
                    trans->writeTransaction = true;

                    trans->data = std::async(policy, trans->parent->internalWrite, trans->parent, murmurHashData, std::ref(trans->success));
                    break;
                }
                case bt::TRANS_SEND_TAB_UNRECVD_MATCHES: {
                    std::vector<bt::MatchIdentifier>* matchesVec = trans->parent->getMatchIdentifierVecPtr();
                    std::vector<int> unrecvdMatches;

                    Database db;
                    for (int i = 0; i < matchesVec->size(); i++) {
                        if (db.query("SELCT * FROM matchtransaction WHERE CompID='?' AND TeamID=? AND MatchID='?';", matchesVec->at(i).comp.c_str(), matchesVec->at(i).team.c_str(), matchesVec->at(i).match.c_str()).at(0).size() < 1) {
                            unrecvdMatches.push_back(i);
                        }
                    }

                    std::ostringstream unrecvdMatchesTxt;
                    for (int i = 0; i < unrecvdMatches.size(); i++) {
                        unrecvdMatchesTxt << unrecvdMatches.at(i);
                        if (i != unrecvdMatches.size()-1) {
                            unrecvdMatchesTxt << ",";
                        }
                    }
                    std::string unrecvdMatchesStr = unrecvdMatchesTxt.str();
                    std::vector<char> unrecvdMatchesTxtVec(unrecvdMatchesStr.begin(), unrecvdMatchesStr.end());
                    
                    trans->batmanTrans = false;
                    trans->writeTransaction = true;
                    trans->data = std::async(policy, trans->parent->internalWrite, trans->parent, unrecvdMatchesTxtVec, std::ref(trans->success));

                    break;
                }
            }
        }
    public:
        bt::TABTRANSACTION* initReadyTransaction(bt::TABTRANSACTION* trans) {
            // get policy at the beginning, as it will need to be used later in most cases.
            std::launch policy = std::launch::async | std::launch::deferred;
            switch(trans->parent->getCallType()) {
                case bt::CALLTYPE_DEFAULT: {
                    policy = std::launch::async | std::launch::deferred;
                    break;
                }
                case bt::CALLTYPE_DEFERRED: {
                    policy = std::launch::deferred;
                    break;
                }
                case bt::CALLTYPE_ASYNCHRONOUS: {
                    policy = std::launch::async;
                    break;
                }
            }

            // error cases
            if (trans->transactionType == bt::TRANS_SOCKET_ERROR) {
                DebugConsole::println("TRANS_SOCKET_ERROR");
                trans->parent->sendNack(); // honestly could be a bad idea in some scenarios lol
                trans->success = false;
                trans->batmanTrans = true;
                trans->writeTransaction = false;
                
                trans->parent->sockSuicide();
                return trans;
            }
            if (trans->transactionType == bt::TRANS_CLOSE_SOCKET) {
                DebugConsole::println("TRANS_CLOSE_SOCKET");
                trans->success = false;
                trans->batmanTrans = true;
                trans->writeTransaction = false;

                trans->parent->sockSuicide();
                return trans;
            }
            DebugConsole::println(std::string("Got Transaction Code: ") + std::to_string(trans->transactionType), DBGC_BLUE, DBGL_DEVEL);
            // read/write handling
            if (trans->writeTransaction) {
                this->handleWriteTransactions(trans, policy);
            }
            else {
                trans->success = true;
                trans->batmanTrans = false;

                trans->data = std::async(policy, trans->parent->internalRead, trans->parent, std::ref(trans->success));
                return trans;
            }

            return trans;
        }

        void handleTransResult(bt::TABTRANSACTION* trans) {
            DebugConsole::println("Transaction result being handled", DBGC_BLUE, DBGL_DEVEL);
            if (trans->writeTransaction) {
                DebugConsole::println("Transaction is a write transaction, handled.", DBGC_GREEN, DBGL_DEVEL);
                return;
            }
            
            switch(trans->transactionType) {
                case bt::TRANS_RECV_MATCH: {
                    std::vector<char> readDataVec = trans->data.get().value();
                    if (trans->success) {
                        std::string data = std::string(readDataVec.begin(), readDataVec.end());
                        std::cerr << data << std::endl;
                        
                        // parse data and put it into database
                        JsonParser parser(data);
                        std::vector<MATCH_DATAPOINT> vectData = parser.parseMatch();
                        DatabaseMan databaseCall;
                        databaseCall.setmatchdata(vectData);
                        databaseCall.addMatchDatapoints();
                    }
                    break;
                }
                case bt::TRANS_RECV_TABLET_INFO: {
                    std::optional<std::vector<char>> tabletScoutingInfoVec = trans->data.get();
                    if (trans->success && tabletScoutingInfoVec.has_value()) {
                        std::string tabletScoutingInfo = std::string(tabletScoutingInfoVec.value().begin(), tabletScoutingInfoVec.value().end());
                        trans->parent->setScoutingName(tabletScoutingInfo);
                    }
                    break;
                }
                case bt::TRANS_RECV_TAB_MATCH_LIST: {
                    std::vector<bt::MatchIdentifier>* matchIdenVecPtr = trans->parent->getMatchIdentifierVecPtr();
                    std::vector<char> readDataVec = trans->data.get().value();
                    std::string matchesStr(readDataVec.begin(), readDataVec.end());
                    std::istringstream matchesStream(matchesStr);
                    
                    std::string currLine;
                    while (std::getline(matchesStream, currLine)) {
                        size_t firstSemicolon = currLine.find(';');
                        size_t secondSemicolon = currLine.find(';', firstSemicolon+1);
                        bt::MatchIdentifier currMatch;
                            currMatch.comp = currLine.substr(0, firstSemicolon);
                            currMatch.team = currLine.substr(firstSemicolon+1, secondSemicolon - 1);
                            currMatch.match = currLine.substr(secondSemicolon+1);
                        matchIdenVecPtr->push_back(currMatch);
                    }
                    break;
                }
            }
            DebugConsole::println("Read transaction handled(?)", DBGC_GREEN, DBGL_DEVEL);
        }
};

#endif