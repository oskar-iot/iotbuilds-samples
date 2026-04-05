#include <Arduino.h>
#include <WioCellular.h>

#include <string>

// モジュール識別情報、SIM 状態、探索設定をまとめて表示。
void PrintInfo() {
  std::string imei;
  WioCellular.getIMEI(&imei);
  std::string revision;
  WioCellular.getModemInfo(&revision);
  int simInserted;
  WioCellular.getSimInsertionStatus(nullptr, &simInserted);
  int simInitStatus;
  WioCellular.getSimInitializationStatus(&simInitStatus);
  std::string simState;
  WioCellular.getSimState(&simState);
  std::string imsi;
  WioCellular.getIMSI(&imsi);
  std::string iccid;
  WioCellular.getSimCCID(&iccid);
  std::string phoneNumber;
  WioCellular.getPhoneNumber(&phoneNumber);
  int searchAct;
  WioCellular.getSearchAccessTechnology(&searchAct);
  std::string emtcBand;
  WioCellular.getSearchFrequencyBand(nullptr, &emtcBand, nullptr);

  Serial.printf("IMEI:                 %s\n", imei.c_str());
  Serial.printf("Revision:             %s\n", revision.c_str());
  Serial.printf("SIM Inserted:         %d(%s)\n", simInserted, simInserted == 0 ? "No" : simInserted == 1 ? "Yes"
                                                                                                          : "Unknown");
  Serial.printf("SIM Init:             %d(%s)\n", simInitStatus, simInitStatus == 0 ? "Initial" : simInitStatus == 1 ? "CPIN Ready"
                                                                                                : simInitStatus == 2 ? "SMS Done"
                                                                                                : simInitStatus == 3 ? "CPIN Ready & SMS Done"
                                                                                                                     : "Unknown");
  Serial.printf("SIM State:            %s\n", simState.c_str());
  Serial.printf("IMSI:                 %s\n", imsi.c_str());
  Serial.printf("ICCID:                %s\n", iccid.c_str());
  Serial.printf("Phone Number:         %s\n", phoneNumber.c_str());
  Serial.printf("Search ACT:           %d(%s)\n", searchAct, searchAct == 0 ? "eMTC" : "Unknown");
  Serial.printf("Search Band - eMTC:   %s\n", emtcBand.c_str());
}
