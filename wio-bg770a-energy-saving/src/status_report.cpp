#include <Arduino.h>
#include <WioCellular.h>

#include <string>

// 3GPP の RSSI コード値を人間可読な dBm 表記へ変換。
static std::string RssiCodeToStr(int rssi) {
  if (rssi == 0) {
    return "~-113dBm";
  } else if (rssi == 1) {
    return "-111dBm";
  } else if (rssi <= 30) {
    const auto value = map(rssi, 2, 30, -109, -53);
    return std::to_string(value) + "dBm";
  } else if (rssi == 31) {
    return "-51~dBm";
  } else {
    return "Unknown";
  }
}

// 3GPP の BER コード値をおおよその誤り率レンジへ変換。
static std::string BerCodeToStr(int ber) {
  switch (ber) {
    case 0:
      return "0~0.2%";
    case 1:
      return "0.2~0.4%";
    case 2:
      return "0.4~0.8%";
    case 3:
      return "0.8~1.6%";
    case 4:
      return "1.6~3.2%";
    case 5:
      return "3.2~6.4%";
    case 6:
      return "6.4~12.8%";
    case 7:
      return "12.8~%";
    default:
      return "Unknown";
  }
}

struct OperatorInfo {
  std::string name;
  std::string plmn;
  int act;
};

// 現在の事業者名と、数値形式(PLMN: MCC/MNC)を両方取得する。
static OperatorInfo GetOperatorInfo() {
  OperatorInfo info{"-", "-", -1};

  int mode = -1;
  int format = -1;
  std::string oper;
  int act = -1;
  if (WioCellular.getOperator(&mode, &format, &oper, &act) != WioCellularResult::Ok) {
    return info;
  }

  if (!oper.empty()) {
    if (format == 2) {
      info.plmn = oper;
    } else {
      info.name = oper;
    }
  }
  info.act = act;

  // COPS表示を数値(PLMN)へ一時切り替えして取得。
  if (WioCellular.setOperator(3, 2, "", -1) == WioCellularResult::Ok) {
    int numMode = -1;
    int numFormat = -1;
    std::string numOper;
    int numAct = -1;
    if (WioCellular.getOperator(&numMode, &numFormat, &numOper, &numAct) == WioCellularResult::Ok &&
        numFormat == 2 && !numOper.empty()) {
      info.plmn = numOper;
      if (info.act == -1) {
        info.act = numAct;
      }
    }
  }

  // 元の表示形式へ戻し、次回以降の表示名取得を安定させる。
  if (format >= 0 && format <= 2) {
    WioCellular.setOperator(3, format, "", -1);
  }

  return info;
}

// 運用中ステータス（電波品質、網登録、事業者、PS接続）を 1 行で表示。
void PrintStatus() {
  const auto uptime = millis() / 1000;
  int rssi;
  int ber;
  WioCellular.getSignalQuality(&rssi, &ber);
  int state;
  WioCellular.getEpsNetworkRegistrationState(&state);
  const OperatorInfo operatorInfo = GetOperatorInfo();
  int psState;
  WioCellular.getPacketDomainState(&psState);

  Serial.printf("%u\t", uptime);
  Serial.printf("Status\t");
  Serial.printf("%d(%s)\t", rssi, RssiCodeToStr(rssi).c_str());
  Serial.printf("%d(%s)\t", ber, BerCodeToStr(ber).c_str());
  Serial.printf("%d(%s)\t", state,
                state == 0   ? "Not Registered"
                : state == 1 ? "Registered, Home Network"
                : state == 2 ? "Searching"
                : state == 3 ? "Denied"
                : state == 4 ? "Unknown"
                : state == 5 ? "Registered, Roaming"
                             : "Unknown");
  Serial.printf("%s(PLMN:%s), %d(%s)\t",
                operatorInfo.name.c_str(),
                operatorInfo.plmn.c_str(),
                operatorInfo.act,
                operatorInfo.act == 7 ? "eMTC" : operatorInfo.act == 9 ? "NB-IoT"
                                                                    : "Unknown");
  Serial.printf("%d(%s)\n", psState, psState == 0 ? "Detached" : psState == 1 ? "Attached"
                                                                              : "Unknown");
}
