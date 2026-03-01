#include "include/raylib-cpp.hpp"
#include "../include/json.hpp"
#include "myincludes/DatabaseMan.hpp"
#include "myincludes/jsonParser.hpp"
#include "myincludes/scenes.hpp"
#include "../include/json_fwd.hpp"
#include "myincludes/pong.hpp"
#include "myincludes/database.hpp"
#include "myincludes/bluetooth/bluetooth.hpp"
#include "myincludes/winsockErrorDesc.hpp"
#include "myincludes/debugConsole.hpp"
#include "myincludes/restReqHandler.hpp"
#include "myincludes/bluetooth/bluetooth.hpp"
#include "myincludes/bluetooth/btTabObj.hpp"
#include "myincludes/bluetooth/bluetoothConductor.hpp"
#include "include/qrcodegen.hpp"

#include "myincludes/guiHandler/guiLib.hpp"

#include <iostream>
#include <fstream>
#include <stdio.h>
#include <time.h>

using namespace gui;
using namespace qrcodegen;

int main() {
    MYSQL_RES res;
    MYSQL_ROW row;
    std::string resultstr;
   
    DebugConsole::print("Welcome to the main computer!\n", DBGC_BLUE);
     
    // _____ Constant Things _____
    raylib::Window window(1280,720,"Scouting App Computer UI", FLAG_WINDOW_RESIZABLE);  
        window.SetConfigFlags(FLAG_VSYNC_HINT);
        window.SetConfigFlags(FLAG_MSAA_4X_HINT);
        window.SetIcon(raylib::Image("resources/eagleEngineeringLogoLowRes.png"));
        SetTraceLogLevel(LOG_WARNING);
    Pong pongame = Pong(&window);
    window.SetTargetFPS(60);

    raylib::Font spaceCadet(std::string("resources/SM.TTF"));
    raylib::Font comicSans(std::string("resources/ComicMono.ttf"));
    raylib::Font spaceMono(std::string("resources/SpaceMono-Bold.ttf"));
    Bluetooth btConn;
    btConn.initWinsock();
    btConn.initAccept();

    RestReqHandler handler;        

    SCENES currentScene = SCANNING;
    Database database;
    // set up display + constant scene stuff
    SizeScaling::init();
    // set up tabs at top of screen
    TabHandler tabs(raylib::Rectangle(0, 0, GetScreenWidth(), GetScreenHeight() * 0.15));    
    // tab buttons
    Button mainTab(0.0, 0.0, RAYWHITE, raylib::Color(255, 255, 255, 10), raylib::Color(255, 255, 255, 40), EzText(raylib::Text(spaceCadet, "Main Tab"), RAYWHITE, 18.0_spD, 0.0));
    Button databaseTab(0.0, 0.0, RAYWHITE, raylib::Color(255, 255, 255, 10), raylib::Color(255, 255, 255, 40), EzText(raylib::Text(spaceCadet, "Database"), RAYWHITE, 18.0_spD, 0.0));
    Button bluetoothTab(0.0,0.0, RAYWHITE, raylib::Color(255, 255, 255, 10), raylib::Color(255, 255, 255, 40), EzText(raylib::Text(spaceCadet, "Bluetooth"), RAYWHITE, 18.0_spD, 0.0));
    mainTab.disable();
    tabs.add(&mainTab)
        .add(&databaseTab)
        .add(&bluetoothTab);

    // _____ Setting up Scenes _____
    DrawableTexture texture(1280.0_spX, 720.0_spY, raylib::Image("resources/bg.png"), raylib::Color(100, 100, 100));
    // __ Scanner Scene __
        Empty scannerScreen(raylib::Rectangle(0,GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight() * 0.85));
        Empty matchConfigurationScreen(raylib::Rectangle(0,GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight() * 0.85));
        Empty pongscreen(raylib::Rectangle(0,GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight() * 0.85));
        TexturedButton goated(400.0_spX, 200.0_spY, raylib::Image("resources/submit-button.png"), raylib::Image("resources/submit-button-hover.png"));
        Button DB(200.0_spX,100.0_spY, RAYWHITE, BLACK, DARKGRAY, EzText(raylib::Text(spaceCadet, "BD"), RAYWHITE, 18.0_spD, 0.0));
        Button AmplifyBlue(300.0_spX,100.0_spY, RAYWHITE, BLUE, DARKGRAY, EzText(raylib::Text(spaceCadet, "AmplifyBlue"), RAYWHITE, 12.0_spD, 0.0));
        Button AmplifyRed(300.0_spX,100.0_spY, RAYWHITE, RED, DARKGRAY, EzText(raylib::Text(spaceCadet, "AmplifyRed"), RAYWHITE, 12.0_spD, 0.0));
        Button pong(10.0, 10.0, raylib::Color(0,0,0,0), raylib::Color(0,0,0,0), GRAY, EzText(raylib::Text(spaceCadet, "_"), RAYWHITE, 5.0_spD, 0.0));
        Button pongback(100.0, 100.0, RAYWHITE, BLACK, DARKGRAY, EzText(raylib::Text(spaceCadet, "Back"), RAYWHITE, 10.0_spD, 0.0));
        Button pongreset(100.0, 100.0, RAYWHITE, BLACK, DARKGRAY, EzText(raylib::Text(spaceCadet, "Reset"), RAYWHITE, 10.0_spD, 0.0));

        Button rest(100.0_spX, 100.0_spY, RAYWHITE, BLACK, DARKGRAY, EzText(raylib::Text(spaceCadet, "rest"), RAYWHITE, 10.0_spD, 0.0));

        goated.setDisplayPos(BOTTOMCENTERED);
        DB.setDisplayPos(BOTTOMLEFT);
        pong.setDisplayPos(TOPRIGHT);
        pongback.setDisplayPos(BOTTOMRIGHT);
        pongreset.setDisplayPos(TOPRIGHT);
        AmplifyBlue.setDisplayPos(CENTERLEFT);
        AmplifyRed.setDisplayPos(CENTERRIGHT);
        TextBox MatchBoxMain(100.0_spX, 50.0_spY, 10, 0.0, 15.0_spD, &spaceMono, WHITE, WHITE);
        MatchBoxMain.setDisplayPos(TOPCENTERED);
        rest.setDisplayPos(TOPCENTERED);

        scannerScreen.add(&goated);
        scannerScreen.add(&MatchBoxMain);
        scannerScreen.add(&AmplifyBlue);
        scannerScreen.add(&AmplifyRed);
    
        pongscreen.add(&pongback);
        pongscreen.add(&pongreset);

    // __  Database Scene __
        Empty dataVisualizationScreen(raylib::Rectangle(0, GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight()));

        EzText uploadBatchTitle(raylib::Text(comicSans, "Upload Batch (Leave blank for un-uploaded data)"), RAYWHITE, 24.0_spD, 0.0);
        TextBox uploadBatchNum(200.0_spX, 40.0_spY, 10, 0.0, 30.0_spD, &spaceMono, WHITE, WHITE);

        Button uploadBatchButton(100.0_spX,50.0_spY, RAYWHITE, BLACK, DARKGRAY, EzText(raylib::Text(spaceCadet, "Upload"), RAYWHITE, 10.0_spD, 0.0));
        DrawableList uploadBatchList(VERTICAL, 10);  
            uploadBatchList.add(&uploadBatchTitle);    
            uploadBatchList.add(&uploadBatchNum);
            uploadBatchList.add(&uploadBatchButton);
            uploadBatchList.setDisplayPos(CENTERLEFT);
        
        // TextBox tournamentMatch(250.0_spX, 30.0_spY, 15, 0.0, 25.0_spD, &spaceMono, WHITE, WHITE);
        Button scouterUpdate(250.0_spX, 40.0_spY, RAYWHITE, BLACK, DARKGRAY, EzText(raylib::Text(spaceCadet, "Update Scouter List"), RAYWHITE, 10.0_spD, 0.0));
        DrawableList getMatchList(VERTICAL, 10);
            // getMatchList.add(&tournamentMatch);
            getMatchList.add(&scouterUpdate);
            getMatchList.setDisplayPos(CENTERED);
        
        TextBox tbaDataEvent(250.0_spX, 30.0_spY, 15, 0.0, 25.0_spD, &spaceMono, WHITE, WHITE);
        Button tbaDataUpdate(250.0_spX, 50.0_spY, RAYWHITE, BLACK, DARKGRAY, EzText(raylib::Text(spaceCadet, "Update TBA Data"), RAYWHITE, 10.0_spD, 0.0));
        DrawableList tbaDataUpdateList(VERTICAL, 10);
            tbaDataUpdateList.add(&tbaDataEvent)
            .add(&tbaDataUpdate);
            tbaDataUpdateList.setDisplayPos(CENTERRIGHT);

        dataVisualizationScreen.add(&DB)
            .add(&uploadBatchList)
            .add(&getMatchList)
            .add(&rest)
            .add(&tbaDataUpdateList);
            
    // __ BT Scene __
        Empty btTestingScene(raylib::Rectangle(0, GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight()));       
        btTestingScene.add(&pong);
        Button disconnectAllTabs(200.0_spX, 100.0_spY, RAYWHITE, raylib::Color(0,0,0,0), raylib::Color(255,255,255,20), EzText(raylib::Text(spaceCadet, "Reset Tabs"), RAYWHITE, 12.0_spD, 0.0));
            disconnectAllTabs.setDisplayPos(BOTTOMRIGHT);
        EzText macAddress(raylib::Text(comicSans, "Mac: " + btConn.getLocalMacStr()), RAYWHITE, 24.0_spD, 0.0);
        EzText port(raylib::Text(comicSans, "Port: " + std::to_string(btConn.getLocalPort())), RAYWHITE, 24.0_spD, 0.0);
        EzText activeConnections(raylib::Text(comicSans, "Connections: " + std::to_string(btConn.getNumConnections())), RAYWHITE, 24.0_spD, 0.0);
        DrawableList serverData(VERTICAL, 0);
            serverData.add(&macAddress);
            serverData.add(&port);
            serverData.add(&activeConnections);
            serverData.setDisplayPos(BOTTOMLEFT);
        btTestingScene.add(&serverData);
        btTestingScene.add(&disconnectAllTabs);
        btConn.getNameList()->setDisplayPos(BOTTOMCENTERED);
        VerticalScrollable* nameList = btConn.getNameList();
        btTestingScene.add(nameList);

        QrCodeHandler qrCode(btConn.getLocalMacStr() + std::string(";") + std::to_string(btConn.getLocalPort()), qrcodegen::QrCode::Ecc::QUARTILE, ShouldScale(400.0, true, YDEPENDENT));
            qrCode.setDisplayPos(TOPCENTERED);
        btTestingScene.add(&qrCode);


    bool windowHighFPS = true;
    while(!window.ShouldClose()) {
        if (window.IsFocused() && !windowHighFPS) {
            window.SetTargetFPS(60);
            windowHighFPS = true;
        }
        else if (!window.IsFocused() && windowHighFPS){
            window.SetTargetFPS(10);
            windowHighFPS = false;
        }

        // make application fullscreen on f11 press (and set resolution)
        if (IsKeyPressed(KEY_F11)) {
            if (!window.IsFullscreen()) {
                window.SetSize(GetMonitorWidth(GetCurrentMonitor()), GetMonitorHeight(GetCurrentMonitor()));
                window.SetFullscreen(true);
            }
            else {
                window.SetFullscreen(false);
                window.SetSize(1280,720);
            }
        }
        if (IsKeyPressed(KEY_E)) {
            toastHandler::add(Toast(WinsockErrorDesc::get(6).errorNameDesc,TOASTLENGTHS::LENGTH_LONG));
        }

        if (mainTab.isPressed()) {
            currentScene = SCANNING;

            mainTab.disable();
            databaseTab.enable();
            bluetoothTab.enable();
        }
        else if (databaseTab.isPressed()) {
            currentScene = DATABASE;
            mainTab.enable();
            databaseTab.disable();
            bluetoothTab.enable();
        }
        else if (bluetoothTab.isPressed()) {
            currentScene = BLUETOOTH;

            mainTab.enable();
            databaseTab.enable();
            bluetoothTab.disable();
        }

        activeConnections.setText("Connections: " + std::to_string(btConn.getNumConnections()));
        btConn.updateAllBt();
        activeConnections.setText("Connections: " + std::to_string(btConn.getNumConnections()));

        switch(currentScene) {
            case SCANNING:
                if (AmplifyBlue.isPressed()) {
                    try {
                        auto res = database.execQuery("insert into matchtransaction ( MatchId, ScouterID, DataPointID,  DCValue, TeamID,AllianceID) values ( 4,-1,11,'true', -1, 'Blue');", 0);  
                        toastHandler::add(Toast("Amplify Blue Started",LENGTH_NORMAL));
                    }
                    catch (...) {
                        toastHandler::add(Toast("error",LENGTH_NORMAL));
                    }
                   
                }
                if (AmplifyRed.isPressed()) {
                    try {
                        auto res = database.execQuery("insert into matchtransaction ( MatchId, ScouterID, DataPointID,  DCValue, TeamID,AllianceID) values ("+ MatchBoxMain.getText() + ",-1,11,'true', -1, 'Red');", 0);  
                        toastHandler::add(Toast("Amplify Red Started",LENGTH_NORMAL));
                    }
                    catch (...) {
                        toastHandler::add(Toast("error",LENGTH_NORMAL));
                    }
                }
               
                // drawing
                window.BeginDrawing();
                    window.ClearBackground(BLACK);
                    texture.draw(0,0);
                    scannerScreen.updateAndDraw(raylib::Rectangle(0, GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight() * 0.85));
                // calling endDrawing later
            break;

            case DATABASE:
                if (rest.isPressed()) {
                    handler.deleteteams();
                    for (int i = 0; i < 23; i++) {
                        handler.getteamdata(i);
                    }              
                }
                 if(DB.isPressed()) {
                    for (int i =1; i<7; i++) {
                        for  (int j =1; j<46; j++) {
                            database.execQuery("insert into matchtransaction ( MatchId, ScouterID, DatapointID,  DCValue, TeamID, AllianceID) values (" + std::to_string(i) + "," + std::to_string(-1) + "," + std::to_string(j) + ",'" + "event" + "'," +  std::to_string(1) + ",'1');", 0);
                            database.execQuery("insert into matchtransaction ( MatchId, ScouterID, DatapointID,  DCValue, TeamID, AllianceID) values (" + std::to_string(i) + "," + std::to_string(-1) + "," + std::to_string(j) + ",'" + "event" + "'," +  std::to_string(4) + ",'1');", 0);
                            database.execQuery("insert into matchtransaction ( MatchId, ScouterID, DatapointID,  DCValue, TeamID, AllianceID) values (" + std::to_string(i) + "," + std::to_string(-1) + "," + std::to_string(j) + ",'" + "event" + "'," +  std::to_string(5) + ",'1');", 0);
                            database.execQuery("insert into matchtransaction ( MatchId, ScouterID, DatapointID,  DCValue, TeamID, AllianceID) values (" + std::to_string(i) + "," + std::to_string(-1) + "," + std::to_string(j) + ",'" + "event" + "'," +  std::to_string(6) + ",'0');", 0);
                            database.execQuery("insert into matchtransaction ( MatchId, ScouterID, DatapointID,  DCValue, TeamID, AllianceID) values (" + std::to_string(i) + "," + std::to_string(-1) + "," + std::to_string(j) + ",'" + "event" + "'," +  std::to_string(7) + ",'0');", 0);
                            database.execQuery("insert into matchtransaction ( MatchId, ScouterID, DatapointID,  DCValue, TeamID, AllianceID) values (" + std::to_string(i) + "," + std::to_string(-1) + "," + std::to_string(j) + ",'" + "event" + "'," +  std::to_string(8) + ",'0');", 0);
                        }
                    }                  
                }
                if (scouterUpdate.isPressed()) {
                    auto result = database.query("select scouterfirstname, scouterlastname, scouterid from scouter where scouterid>=0");
                    if (result[0].size() == 3) {
                        std::cout << result.size() << ", " << result[0].size();
                        std::ofstream scouterFile("resources/csv/scouterList.csv");
                        scouterFile << result[0][0] << " " << result[0][1] << ":" << result[0][2];
                        for (int i = 1; i < result.size(); i++) {
                            scouterFile << "," << result[i][0] << " " << result[i][1] << ":" << result[i][2];
                        }
                    }
                    else {
                        toastHandler::add(Toast("Scouter DB Error", LENGTH_NORMAL));
                    }            
                }

                if(uploadBatchButton.isPressed()) {
                    bool success = true;
                    std::string batch = uploadBatchNum.getText();
                    if (batch == "") {
                        batch = std::string("0");
                    }
                    
                    Database batchDBInst;

                    
                    std::vector<std::vector<std::string>> dbResp = batchDBInst.query("select * from matchtransaction where UploadID=?", batch.c_str());
                    if (batch == "0") {
                        std::string backendNum = handler.getFromBackend("http://localhost/requests/getNextUploadID.php");
                        if(backendNum == "") {
                            DebugConsole::println(std::string("BADBADBAD NOT UPLOADED SUCCESSFULLY"), DBGC_RED, DBGL_ERROR);
                            continue; // pray ts doesn't happen 🙏🙏
                        }
                        
                        DebugConsole::println(std::string("Uploading batch (assigned) #") + backendNum);
                        batch = backendNum;
                        for (std::vector<std::string> i : dbResp) {
                            auto _ = batchDBInst.query("INSERT INTO matchtransaction(UploadID, CompID, MatchID, DatapointID, ScouterID, TeamID, AllianceID, DatapointValue, DatapointTimestamp) VALUES(?,'?','?',?,?,?,?,'?',?)", backendNum.c_str(), i.at(2).c_str(), i.at(3).c_str(), i.at(4).c_str(), i.at(5).c_str(), i.at(6).c_str(), i.at(7).c_str(), i.at(8).c_str(), i.at(9).c_str());
                            batchDBInst.query("DELETE FROM matchtransaction WHERE UploadID=0");
                        }
                    }
                    else {
                        DebugConsole::println(std::string("Uploading batch #") + batch);
                    }
                    
                    nlohmann::json jsonToBeSent;
                    std::vector<std::string> columnNames = batchDBInst.getHeaders("matchtransaction");
                    for (int i = 0; i < dbResp.size(); i++) {
                        for (int j = 0; j < dbResp.at(i).size(); j++) {
                            if(columnNames.at(j) == std::string("UploadID")) {
                                jsonToBeSent[i][columnNames.at(j)] = batch;
                            }
                            else {
                                jsonToBeSent[i][columnNames.at(j)] = dbResp.at(i).at(j);
                            }
                        }
                    }
                    DebugConsole::println("Got Data! :3");
                    DebugConsole::println(handler.uploadToBackend("http://localhost/requests/uploadMatchData.php", jsonToBeSent.dump()));
                }
              
                if (tbaDataUpdate.isPressed()) {
                    std::string validateString = handler.makeTBAReq(std::string("event/") + tbaDataEvent.getText() + std::string("/simple"));
                    if (!(validateString.length() < 7 || validateString.substr(5, 5) == "Error")) {
                        handler.getMatchData(tbaDataEvent.getText());
                        handler.getteamsatcomphdata(tbaDataEvent.getText());
                        std::ofstream compIDFile("resources/csv/compID.csv");
                        compIDFile << tbaDataEvent.getText();
                    }
                    else {
                        toastHandler::add(Toast(std::string("EVENT NAME INVALID"), LENGTH_NORMAL));
                    }
                }

                window.BeginDrawing();
                    window.ClearBackground(BLACK);
                    texture.draw(0,0);
                    dataVisualizationScreen.updateAndDraw(raylib::Rectangle(0, GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight() * 0.85));
                
            break;

            case BLUETOOTH:
                if (disconnectAllTabs.isPressed()) {
                    btConn.killAllSockets();
                }
                if (pong.isPressed()) {
                    std::cout << "hello" << std::endl;
                    currentScene = PONG;
                }
                window.BeginDrawing();
                window.ClearBackground(BLACK);
                nameList = btConn.getNameList();
                btTestingScene.updateAndDraw(raylib::Rectangle(0, GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight() * 0.85));        
            break;
            
            case PONG:
                window.BeginDrawing();
                window.ClearBackground(BEIGE);
                pongame.update();
                DrawCircle(pongame.Ballpos.x,pongame.Ballpos.y,20.0f,BLACK);
                DrawRectangle(pongame.Paddle1pos.x,pongame.Paddle1pos.y,20.0f,80.0f,BLACK);
                DrawRectangle(pongame.Paddle2pos.x,pongame.Paddle2pos.y,20.0f,80.0f,BLACK);
                mainTab.disable();
                databaseTab.disable();
                bluetoothTab.disable();
                if (pongback.isPressed()) {
                    currentScene = BLUETOOTH;

                    mainTab.enable();
                    databaseTab.enable();
                    bluetoothTab.disable();
                }
                if (pongreset.isPressed()) {
                   pongame.reset();
                }
                
                pongscreen.updateAndDraw(raylib::Rectangle(0, GetScreenHeight() * 0.15, GetScreenWidth(), GetScreenHeight() * 0.85));
            break;
        }
        if (currentScene != PONG) {
            tabs.updateAndDraw(raylib::Rectangle(0, 0, GetScreenWidth(), GetScreenHeight() * 0.15));
        }

        window.DrawFPS();
        toastHandler::update();
        window.EndDrawing();   
    }
    btConn.killAllSockets();
    WinsockErrorDesc::destroy();
    return 0;
}