#pragma once
#include <list>
#include <string>

#include "computer.h"
#include "history.h"

class UCI {
   public:
    UCI();
    void writeTokenizedCommand(std::string line);
    void consumeOutput();

   private:
    History hist;

    // https://www.wbec-ridderkerk.nl/html/UCIProtocol.html
    void handleUci(std::list<std::string>& params);
    void handleIsReady(std::list<std::string>& params);
    void handleUciNewGame(std::list<std::string>& params);
    void handleGo(std::list<std::string>& params);
    void handlePosition(std::list<std::string>& params);
    void handleDisplay(std::list<std::string>& params);
    void handleStop(std::list<std::string>& params);
    void handleQuit(std::list<std::string>& params);
    void handleMovelist(std::list<std::string>& params);
};
